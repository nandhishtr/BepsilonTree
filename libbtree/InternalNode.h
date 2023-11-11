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
	~InternalNode();
	InternalNode(uint32_t nDegree);
	InternalNode(uint32_t nDegree, int nPivot, CacheKeyType ptrLHSNode, CacheKeyType ptrRHSNode);
	InternalNode(uint32_t nDegree, InternalNode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>* ptrNodeSource, size_t nOffset);

	void wieHiestDu() {
		printf("ich heisse InternalNode.\n");
	}

	ErrorCode insert(const KeyType& key, const ValueType& value, CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot);
	ErrorCode insert(CacheTypePtr ptrCache, CacheKeyType ptrChildNode, int nChildPivot, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot);

	//ErrorCode remove(const KeyType& key);

	ErrorCode search(CacheTypePtr ptrCache, const KeyType& key, ValueType& value);

	size_t getChildCount();
	//ErrorCode moveAnEntityFromLHSSibling(INodePtr ptrLHSSibling, KeyType& oKey);
	//ErrorCode moveAnEntityFromRHSSibling(INodePtr ptrRHSSibling, KeyType& oKey);

	//ErrorCode mergeNodes(INodePtr ptrSiblingToMerge);
private:
	bool requireSplit();
	size_t getChildNodeIndex(const KeyType& key);

	//ErrorCode rebalance(size_t nChildNodeIdx);
	ErrorCode split(CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot);

	void print(CacheTypePtr ptrCache, int nLevel);
};

//template class InternalNode<int, int, DRAMLRUCache, uintptr_t, DRAMCacheObject>;
//template class InternalNode<int, std::string, DRAMLRUCache, uintptr_t, DRAMCacheObject>;
//template class InternalNode<int, int, DRAMLRUCache, DRAMCacheObjectKey, DRAMCacheObject>;
//template class InternalNode<int, std::string, DRAMLRUCache, DRAMCacheObjectKey, DRAMCacheObject>;

template class InternalNode<int, int, NVRAMLRUCache, uintptr_t, NVRAMCacheObject>;
template class InternalNode<int, std::string, NVRAMLRUCache, uintptr_t, NVRAMCacheObject>;
template class InternalNode<int, int, NVRAMLRUCache, NVRAMCacheObjectKey, NVRAMCacheObject>;
template class InternalNode<int, std::string, NVRAMLRUCache, NVRAMCacheObjectKey, NVRAMCacheObject>;