#pragma once

#include <vector>
#include <string>
#include <map>

#include "INode.h"
#include "InternalNode.h"
#include "DRAMLRUCache.h"
#include "DRAMCacheObject.h"
#include "DRAMCacheObjectKey.h"

using namespace std; 

template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
class LeafNode : public INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>
{
	typedef std::shared_ptr<CacheType<CacheKeyType, CacheValueType>> CacheTypePtr;
	typedef std::shared_ptr<LeafNode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>> LeafNodePtr;
private:
	uint32_t m_nDegree;
	
	std::vector<KeyType> m_vtKeys;
	std::vector<ValueType> m_vtValues;

public:
	~LeafNode();

	LeafNode(uint32_t nDegree);
	LeafNode(uint32_t nDegree, LeafNode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>* ptrNodeSource, size_t nOffset);

	ErrorCode insert(const KeyType& key, const ValueType& value, CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot);
	ErrorCode insert(CacheTypePtr ptrCache, CacheKeyType ptrChildNode, int nChildPivot, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot);

	ErrorCode remove(const KeyType& key);

	ErrorCode search(CacheTypePtr ptrCache, const KeyType& key, ValueType& value);

	size_t getChildCount();
	ErrorCode moveAnEntityFromLHSSibling(LeafNodePtr ptrLHSSibling, KeyType& oKey);
	ErrorCode moveAnEntityFromRHSSibling(LeafNodePtr ptrRHSSibling, KeyType& oKey);

	ErrorCode mergeNodes(LeafNodePtr ptrSiblingToMerge);
private:
	bool requireSplit();
	ErrorCode split(CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot);

public:
	void print(CacheTypePtr ptrCache, int nLevel);

	void wieHiestDu() {
		printf("ich heisse LeafNode.\n");
	}
};

template class LeafNode<int, int, DRAMLRUCache, uintptr_t, DRAMCacheObject>;
template class LeafNode<int, std::string, DRAMLRUCache, uintptr_t, DRAMCacheObject>;


template class LeafNode<int, int, DRAMLRUCache, DRAMCacheObjectKey, DRAMCacheObject>;
template class LeafNode<int, std::string, DRAMLRUCache, DRAMCacheObjectKey, DRAMCacheObject>;

