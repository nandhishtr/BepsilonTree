#pragma once

#include <vector>
#include <string>
#include <map>

#include "INode.h"
#include "InternalNode.h"
#include "DRAMLRUCache.h"
#include "DRAMCacheObject.h"
#include "DRAMCacheObjectKey.h"

#include "NVRAMLRUCache.h"
#include "NVRAMCacheObject.h"
#include "NVRAMCacheObjectKey.h"

using namespace std; 

//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
template <typename KeyType, typename ValueType, typename CacheType>
class LeafNode : public INode<KeyType, ValueType, CacheType>
{
	typedef CacheType::KeyType CacheKeyType;

	typedef std::shared_ptr<CacheType> CacheTypePtr;
	typedef std::shared_ptr<LeafNode<KeyType, ValueType, CacheType>> LeafNodePtr;
private:
	uint32_t m_nDegree;
	
	std::vector<KeyType> m_vtKeys;
	std::vector<ValueType> m_vtValues;

public:
	~LeafNode() 
	{
	}

	LeafNode(uint32_t nDegree) : m_nDegree(nDegree)	
	{
	}

	LeafNode(uint32_t nDegree, LeafNode<KeyType, ValueType, CacheType>* ptrNodeSource, size_t nOffset)
		: m_nDegree(nDegree)
	{
		this->m_vtKeys.assign(ptrNodeSource->m_vtKeys.begin() + nOffset, ptrNodeSource->m_vtKeys.end());
		this->m_vtValues.assign(ptrNodeSource->m_vtValues.begin() + nOffset, ptrNodeSource->m_vtValues.end());

		std::ostringstream oss;
		std::copy(m_vtKeys.begin(), m_vtKeys.end(), std::ostream_iterator<int>(oss, ","));
		std::cout << "LeafNode::split - after - new node - pivots: " << oss.str() << std::endl;
	}

	ErrorCode insert(const KeyType& key, const ValueType& value, CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
	{
		std::cout << "LeafNode::insert(" << key << "," << value << ")" << std::endl;

		m_vtKeys.push_back(key);
		m_vtValues.push_back(value);

		if (requireSplit())
		{
			return split(ptrCache, ptrSiblingNode, nSiblingPivot);
		}

		return ErrorCode::Success;
	}

	ErrorCode insert(CacheTypePtr ptrCache, CacheKeyType ptrChildNode, int nChildPivot, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
	{
		throw new exception("should occur!");
		return ErrorCode::ChildSplitCalledOnLeafNode;
	}

	ErrorCode remove(const KeyType& key)
	{
		auto it = std::lower_bound(m_vtKeys.begin(), m_vtKeys.end(), key);

		if (it != m_vtKeys.end() && *it == key)
		{
			int index = it - m_vtKeys.begin();
			m_vtKeys.erase(it);
			m_vtValues.erase(m_vtValues.begin() + index);

			return ErrorCode::Success;
		}

		return ErrorCode::KeyDoesNotExist;
	}

	ErrorCode search(CacheTypePtr ptrCache, const KeyType& key, ValueType& value)
	{
		auto it = std::lower_bound(m_vtKeys.begin(), m_vtKeys.end(), key);
		if (it != m_vtKeys.end() && *it == key) {
			int index = it - m_vtKeys.begin();
			value = m_vtValues[index];

			return ErrorCode::Success;
		}

		return ErrorCode::KeyDoesNotExist;
	}

	size_t getChildCount()
	{
		return m_vtKeys.size();
	}

	ErrorCode moveAnEntityFromLHSSibling(LeafNodePtr ptrLHSSibling, KeyType& oKey)
	{
		LeafNodePtr lhsibling = static_cast<LeafNodePtr>(ptrLHSSibling);

		KeyType key = lhsibling->m_vtKeys.back();
		ValueType value = lhsibling->m_vtValues.back();

		lhsibling->m_vtKeys.pop_back();
		lhsibling->m_vtValues.pop_back();

		m_vtKeys.insert(m_vtKeys.begin(), key);
		m_vtValues.insert(m_vtValues.begin(), value);

		oKey = key;

		return ErrorCode::Success;
	}

	ErrorCode moveAnEntityFromRHSSibling(LeafNodePtr ptrRHSSibling, KeyType& oKey)
	{
		LeafNodePtr rhssibling = static_cast<LeafNodePtr>(ptrRHSSibling);

		KeyType key = rhssibling->m_vtKeys.front();
		ValueType value = rhssibling->m_vtValues.front();

		rhssibling->m_vtKeys.erase(rhssibling->m_vtKeys.begin());
		rhssibling->m_vtValues.erase(rhssibling->m_vtValues.begin());

		m_vtKeys.push_back(key);
		m_vtValues.push_back(value);

		oKey = rhssibling->m_vtKeys.front();

		return ErrorCode::Success;
	}

	ErrorCode mergeNodes(LeafNodePtr ptrSiblingToMerge)
	{
		LeafNodePtr ptrNode = static_cast<LeafNodePtr>(ptrSiblingToMerge);

		m_vtKeys.insert(m_vtKeys.end(), ptrNode->m_vtKeys.begin(), ptrNode->m_vtKeys.end());
		m_vtValues.insert(m_vtValues.end(), ptrNode->m_vtValues.begin(), ptrNode->m_vtValues.end());

		//delete ptrNode; todo..

		return ErrorCode::Success;
	}

private:
	bool requireSplit() 
	{
		if (m_vtKeys.size() > m_nDegree)
			return true;

		return false;
	}

	ErrorCode split(CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
	{
		std::cout << "LeafNode::split" << std::endl;

		int nOffset = m_vtKeys.size() / 2;

		ptrSiblingNode = ptrCache->createObjectOfType<LeafNode<KeyType, ValueType, CacheType>>(m_nDegree, this, nOffset);
		if (!ptrSiblingNode)
		{
			return ErrorCode::Error;
		}

		nSiblingPivot = this->m_vtKeys[nOffset];

		m_vtKeys.resize(nOffset);
		m_vtValues.resize(nOffset);

		std::ostringstream oss;
		std::copy(m_vtKeys.begin(), m_vtKeys.end(), std::ostream_iterator<int>(oss, ","));
		std::cout << "LeafNode::split - after - current node - pivots: " << oss.str() << std::endl;

		std::cout << "LeafNode::split pivot " << nSiblingPivot << std::endl;

		return ErrorCode::Success;
	}

public:
	void print(CacheTypePtr ptrCache, int nLevel)
	{
		for (int i = 0; i < m_vtKeys.size(); i++)
		{
			printf("%s> level: %d, key: %d, value: %d\n", std::string(nLevel, '.').c_str(), nLevel, m_vtKeys[i], m_vtValues[i]);
		}
	}

	void wieHiestDu() {
		printf("ich heisse LeafNode.\n");
	}
};

//template class LeafNode<int, int, DRAMLRUCache, uintptr_t, DRAMCacheObject>;
//template class LeafNode<int, std::string, DRAMLRUCache, uintptr_t, DRAMCacheObject>;
//template class LeafNode<int, int, DRAMLRUCache, DRAMCacheObjectKey, DRAMCacheObject>;
//template class LeafNode<int, std::string, DRAMLRUCache, DRAMCacheObjectKey, DRAMCacheObject>;


//template class LeafNode<int, int, NVRAMLRUCache, uintptr_t, NVRAMCacheObject>;
//template class LeafNode<int, std::string, NVRAMLRUCache, uintptr_t, NVRAMCacheObject>;
//template class LeafNode<int, int, NVRAMLRUCache, NVRAMCacheObjectKey, NVRAMCacheObject>;
//template class LeafNode<int, std::string, NVRAMLRUCache, NVRAMCacheObjectKey, NVRAMCacheObject>;