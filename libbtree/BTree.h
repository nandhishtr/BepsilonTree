#pragma once

#include <iostream>
#include "ErrorCodes.h"
#include "INode.h"
#include "LeafNode.h"

#include "DRAMLRUCache.h"
#include "DRAMCacheObject.h"
#include "DRAMCacheObjectKey.h"

template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
class BTree
{
    typedef std::shared_ptr<CacheType<CacheKeyType, CacheValueType>> CacheTypePtr;
    typedef std::shared_ptr<INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>> INodePtr;
    typedef std::shared_ptr<LeafNode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>> LeafNodePtr;
    typedef std::shared_ptr<InternalNode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>> InternalNodePtr;

private:
    uint32_t m_nDegree;
    std::shared_ptr<CacheType<CacheKeyType, CacheValueType>> m_ptrCache;
    std::optional<CacheKeyType> m_cktRootNodeKey;
    //LeafNode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>* m_ptrRootNode;

public:
    BTree();
    ~BTree();

    ErrorCode init();
    ErrorCode insert(const KeyType& key, const ValueType& value);
    ErrorCode remove(const KeyType& key);
    ErrorCode search(const KeyType& key, ValueType& value);

    void print();
};

template class BTree<int, int, DRAMLRUCache, uintptr_t, DRAMCacheObject>;
template class BTree<int, std::string, DRAMLRUCache, uintptr_t, DRAMCacheObject>;

template class BTree<int, int, DRAMLRUCache, DRAMCacheObjectKey, DRAMCacheObject>;
template class BTree<int, std::string, DRAMLRUCache, DRAMCacheObjectKey, DRAMCacheObject>;
