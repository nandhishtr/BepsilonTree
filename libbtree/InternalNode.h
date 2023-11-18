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

template <typename KeyType, typename ValueType, typename CacheType>
class InternalNode : public INode<KeyType, ValueType, CacheType>
{
	typedef CacheType::KeyType CacheKeyType;
	typedef std::shared_ptr<CacheType> CacheTypePtr;
	typedef std::shared_ptr<INode<KeyType, ValueType, CacheType>> INodePtr;
	typedef std::shared_ptr<InternalNode<KeyType, ValueType, CacheType>> InternalNodePtr;

private:
	uint32_t m_nDegree;

	std::vector<KeyType> m_vtPivots;
	std::vector<CacheKeyType> m_vtChildren;

public:
	~InternalNode()
	{
	}

	InternalNode(uint32_t nDegree, int nPivot, CacheKeyType ptrLHSNode, CacheKeyType ptrRHSNode)
		: m_nDegree(nDegree)
	{
		LOG(INFO) << "Creating InternalNode .. nPivot=" << nPivot << ".";

		m_vtPivots.push_back(nPivot);
		m_vtChildren.push_back(ptrLHSNode);
		m_vtChildren.push_back(ptrRHSNode);
	}

	InternalNode(uint32_t nDegree, InternalNode<KeyType, ValueType, CacheType>* ptrNodeSource, size_t nOffset)
		: m_nDegree(nDegree)
	{
		LOG(INFO) << "Creating InternalNode .. due to split .., Offset=" << nOffset << ".";

		this->m_vtPivots.assign(ptrNodeSource->m_vtPivots.begin() + nOffset, ptrNodeSource->m_vtPivots.end());
		this->m_vtChildren.assign(ptrNodeSource->m_vtChildren.begin() + nOffset, ptrNodeSource->m_vtChildren.end());

		std::ostringstream oss;
		std::copy(m_vtPivots.begin(), m_vtPivots.end(), std::ostream_iterator<int>(oss, ","));
		LOG(INFO) << "Newly created node details (" << oss.str() << ").";

	}

	void wieHiestDu() {
		printf("ich heisse InternalNode.\n");
	}

	ErrorCode insert(const KeyType& key, const ValueType& value, CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
	{
		int nChildIdx = 0;
		while (nChildIdx < m_vtPivots.size() && key > m_vtPivots[nChildIdx]) 
		{
			nChildIdx++;
		}

		LOG(INFO) << "Insert (Key=" << key << ", Value=" << value << ") Idx=" << nChildIdx << ".";

		INodePtr ptrParentNode = ptrCache->template getObjectOfType<INode<KeyType, ValueType, CacheType>>(m_vtChildren[nChildIdx]);
		if (ptrParentNode == NULL)
		{
			LOG(ERROR) << "Failed to get object.";

			return ErrorCode::Error;
		}

		int nChildPivot = 0;
		std::optional<CacheKeyType> ptrChildNode;
		ErrorCode retCode = ptrParentNode->insert(key, value, ptrCache, ptrChildNode, nChildPivot);

		if (retCode == ErrorCode::Error)
		{
			LOG(ERROR) << "Failed to insert object.";

			return ErrorCode::Error;
		}

		if (ptrChildNode)
		{
			LOG(INFO) << "Insert object due to split at the previous level.";

			return insert(ptrCache, *ptrChildNode, nChildPivot, ptrSiblingNode, nSiblingPivot);
		}

		return ErrorCode::Success;
	}

	ErrorCode insert(CacheTypePtr ptrCache, CacheKeyType ptrChildNode, int nChildPivot, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
	{
		int nChildIdx = m_vtPivots.size();
		for (int nIdx = 0; nIdx < m_vtPivots.size(); ++nIdx) 
		{
			if (nChildPivot < m_vtPivots[nIdx]) 
			{
				nChildIdx = nIdx;
				break;
			}
		}

		LOG(INFO) << "Insert object nChildIdx=" << nChildIdx << ".";

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

		INodePtr ptrChildNode = ptrCache->template getObjectOfType<INode<KeyType, ValueType, CacheType>>(m_vtChildren[nIdx]);
		if (ptrChildNode == NULL)
		{
			LOG(INFO) << "Failed to get object (nIdx=" << nIdx << ").";

			return ErrorCode::Error;
		}

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
		for (size_t nIdx = 0; nIdx < m_vtPivots.size(); ++nIdx) 
		{
			if (key < m_vtPivots[nIdx]) 
			{
				return nIdx;
			}
		}
		return m_vtPivots.size();
	}

	//ErrorCode rebalance(size_t nChildNodeIdx);
	ErrorCode split(CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
	{
		LOG(INFO) << "Trying to split object.";

		int nOffset = m_vtPivots.size() / 2;

		nSiblingPivot = this->m_vtPivots[nOffset];
		ptrSiblingNode = ptrCache->template createObjectOfType<InternalNode<KeyType, ValueType, CacheType>>(m_nDegree, this, nOffset);

		//m_vtPivots.erase(m_vtPivots.begin() + nOffset, m_vtPivots.end());
		//m_vtChildren.erase(m_vtChildren.begin() + nOffset, m_vtChildren.end());

		m_vtPivots.resize(nOffset);
		m_vtChildren.resize(nOffset + 1);

		std::ostringstream oss;
		std::copy(m_vtPivots.begin(), m_vtPivots.end(), std::ostream_iterator<int>(oss, ","));
		LOG(INFO) << "Current object after split (" << oss.str() << ") child count=" << m_vtChildren.size() << ".";

		LOG(INFO) << "Pivot=" << nSiblingPivot << ").";
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


			INodePtr ptrChildNode = ptrCache->template getObjectOfType<INode<KeyType, ValueType, CacheType>>(m_vtChildren[i]);
			if (ptrChildNode == NULL)
				return;

			ptrChildNode->print(ptrCache, nLevel + 1);
		}
	}
};