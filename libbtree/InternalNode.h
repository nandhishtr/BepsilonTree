#pragma once

#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <iterator>
#include <iostream>

#include "INode.h"
#include "ICacheObjectKey.h"
#include "DRAMLRUCache.h"
#include "DRAMCacheObject.h"
#include "DRAMCacheObjectKey.h"

#include "NVRAMLRUCache.h"
#include "NVRAMCacheObject.h"
#include "NVRAMCacheObjectKey.h"

using namespace std;

template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
class InternalNode : public INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>
{
	typedef std::shared_ptr<CacheType<CacheKeyType, CacheValueType>> CacheTypePtr;
	typedef std::shared_ptr<INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>> INodePtr;
	typedef std::shared_ptr<InternalNode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>> InternalNodePtr;

private:
	uint32_t m_nDegree;

	std::vector<KeyType> m_vtPivots;
	//std::vector<ValueType> m_vtValues;
	std::vector<CacheKeyType> m_vtChildren;

public:
	~InternalNode()
	{

	}

	InternalNode(uint32_t nDegree)
		: m_nDegree(nDegree)
	{
		std::cout << "InternalNode::InternalNode()" << std::endl;
	}

	InternalNode(uint32_t nDegree, int nPivot, CacheKeyType ptrLHSNode, CacheKeyType ptrRHSNode)
		: m_nDegree(nDegree)
	{
		std::cout << "InternalNode::InternalNode(pvt, lhs, rhs)" << std::endl;

		m_vtPivots.push_back(nPivot);
		m_vtChildren.push_back(ptrLHSNode);
		m_vtChildren.push_back(ptrRHSNode);
	}

	InternalNode(uint32_t nDegree, InternalNode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>* ptrNodeSource, size_t nOffset)
		: m_nDegree(nDegree)
	{
		std::cout << "InternalNode::InternalNode(..due to split)" << std::endl;

		this->m_vtPivots.assign(ptrNodeSource->m_vtPivots.begin() + nOffset, ptrNodeSource->m_vtPivots.end());
		this->m_vtChildren.assign(ptrNodeSource->m_vtChildren.begin() + nOffset, ptrNodeSource->m_vtChildren.end());

		std::ostringstream oss;
		std::copy(m_vtPivots.begin(), m_vtPivots.end(), std::ostream_iterator<int>(oss, ","));
		std::cout << "InternalNode::split - after - new node - pivots: " << oss.str() << std::endl;

	}

	void wieHiestDu() {
		printf("ich heisse InternalNode.\n");
	}

	ErrorCode insert(const KeyType& key, const ValueType& value, CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
	{
		int nChildIdx = 0;
		while (nChildIdx < m_vtPivots.size() && key > m_vtPivots[nChildIdx]) {
			nChildIdx++;
		}

		std::cout << "InternalNode::insert(" << key << "," << value << ") Idx: " << nChildIdx << std::endl;

		INodePtr ptrParentNode = ptrCache->getObjectOfType<INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>>(m_vtChildren[nChildIdx]);
		if (ptrParentNode == NULL)
			return ErrorCode::Error;

		int nChildPivot = 0;
		std::optional<CacheKeyType> ptrChildNode;
		ErrorCode retCode = ptrParentNode->insert(key, value, ptrCache, ptrChildNode, nChildPivot);

		if (ptrChildNode)
		{
			return insert(ptrCache, *ptrChildNode, nChildPivot, ptrSiblingNode, nSiblingPivot);
		}

		return ErrorCode::Success;
	}

	ErrorCode insert(CacheTypePtr ptrCache, CacheKeyType ptrChildNode, int nChildPivot, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
	{
		int nChildIdx = m_vtPivots.size();
		for (int nIdx = 0; nIdx < m_vtPivots.size(); ++nIdx) {
			if (nChildPivot < m_vtPivots[nIdx]) {
				nChildIdx = nIdx;
				break;
			}
		}

		std::cout << "InternalNode::insert_nodes at " << nChildIdx << std::endl;

		m_vtPivots.insert(m_vtPivots.begin() + nChildIdx, nChildPivot);
		m_vtChildren.insert(m_vtChildren.begin() + nChildIdx + 1, ptrChildNode);

		if (requireSplit())
		{
			return split(ptrCache, ptrSiblingNode, nSiblingPivot);
		}

		return ErrorCode::Success;
	}


	//ErrorCode remove(const KeyType& key);

	ErrorCode search(CacheTypePtr ptrCache, const KeyType& key, ValueType& value)
	{
		int nIdx = getChildNodeIndex(key);

		INodePtr ptrChildNode = ptrCache->getObjectOfType<INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>>(m_vtChildren[nIdx]);
		if (ptrChildNode == NULL)
			return ErrorCode::Error;

		return ptrChildNode->search(ptrCache, key, value);
	}

	size_t getChildCount()
	{
		return m_vtPivots.size();
	}

	//ErrorCode moveAnEntityFromLHSSibling(INodePtr ptrLHSSibling, KeyType& oKey);
	//ErrorCode moveAnEntityFromRHSSibling(INodePtr ptrRHSSibling, KeyType& oKey);

	//ErrorCode mergeNodes(INodePtr ptrSiblingToMerge);
private:
	bool requireSplit()
	{
		if (m_vtPivots.size() > m_nDegree)
			return true;

		return false;
	}

	size_t getChildNodeIndex(const KeyType& key)
	{
		for (size_t nIdx = 0; nIdx < m_vtPivots.size(); ++nIdx) {
			if (key < m_vtPivots[nIdx]) {
				return nIdx;
			}
		}
		return m_vtPivots.size();
	}

	//ErrorCode rebalance(size_t nChildNodeIdx);
	ErrorCode split(CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
	{
		std::cout << "InternalNode::split" << std::endl;

		int nOffset = m_vtPivots.size() / 2;

		nSiblingPivot = this->m_vtPivots[nOffset];
		ptrSiblingNode = ptrCache->createObjectOfType<InternalNode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>>(5, this, nOffset);

		//m_vtPivots.erase(m_vtPivots.begin() + nOffset, m_vtPivots.end());
		//m_vtChildren.erase(m_vtChildren.begin() + nOffset, m_vtChildren.end());

		m_vtPivots.resize(nOffset);
		m_vtChildren.resize(nOffset + 1);


		std::ostringstream oss;
		std::copy(m_vtPivots.begin(), m_vtPivots.end(), std::ostream_iterator<int>(oss, ","));
		std::cout << "InternalNode::split - after - current node - pivots: " << oss.str() << "child count: " << m_vtChildren.size() << std::endl;

		std::cout << "InternalNode::split pivot " << nSiblingPivot << std::endl;
		return ErrorCode::Success;
	}

	void print(CacheTypePtr ptrCache, int nLevel)
	{
		std::ostringstream oss;

		if (!m_vtPivots.empty())
		{
			// Convert all but the last element to avoid a trailing ","
			std::copy(m_vtPivots.begin(), m_vtPivots.end() - 1,
				std::ostream_iterator<int>(oss, ","));

			// Now add the last element with no delimiter
			oss << m_vtPivots.back();
		}

		printf("%s> level: %d\n", std::string(nLevel, '.').c_str(), nLevel);
		printf("%s  pivots: %s\n", std::string(nLevel, ' ').c_str(), oss.str().c_str());

		for (int i = 0; i < m_vtChildren.size(); i++)
		{
			printf("%s  child: %d\n", std::string(nLevel, ' ').c_str(), i);


			INodePtr ptrChildNode = ptrCache->getObjectOfType<INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>>(m_vtChildren[i]);
			if (ptrChildNode == NULL)
				return;

			ptrChildNode->print(ptrCache, nLevel + 1);
		}
	}
};

//template class InternalNode<int, int, DRAMLRUCache, uintptr_t, DRAMCacheObject>;
//template class InternalNode<int, std::string, DRAMLRUCache, uintptr_t, DRAMCacheObject>;
//template class InternalNode<int, int, DRAMLRUCache, DRAMCacheObjectKey, DRAMCacheObject>;
//template class InternalNode<int, std::string, DRAMLRUCache, DRAMCacheObjectKey, DRAMCacheObject>;

//template class InternalNode<int, int, NVRAMLRUCache, uintptr_t, NVRAMCacheObject>;
//template class InternalNode<int, std::string, NVRAMLRUCache, uintptr_t, NVRAMCacheObject>;
//template class InternalNode<int, int, NVRAMLRUCache, NVRAMCacheObjectKey, NVRAMCacheObject>;
//template class InternalNode<int, std::string, NVRAMLRUCache, NVRAMCacheObjectKey, NVRAMCacheObject>;