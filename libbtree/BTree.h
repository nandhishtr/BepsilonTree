#pragma once

#include <iostream>
#include "ErrorCodes.h"
#include "INode.h"
#include "LeafNode.h"

#include "DRAMLRUCache.h"
#include "DRAMCacheObject.h"
#include "DRAMCacheObjectKey.h"

#include "NVRAMLRUCache.h"
#include "NVRAMCacheObject.h"
#include "NVRAMCacheObjectKey.h"

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
    ~BTree()
    {

    }

    BTree()
        //    : m_nDegree(nDegree),
    {
        m_ptrCache = std::make_shared<CacheType<CacheKeyType, CacheValueType>>(10000);
        m_cktRootNodeKey = m_ptrCache->createObjectOfType<LeafNode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>>(5);

        std::cout << "BTree::BTree()" << std::endl;
    }

    ErrorCode init()
    {
        std::cout << "BTree::init()" << std::endl;


        return ErrorCode::Success;
    }

    ErrorCode insert(const KeyType& key, const ValueType& value)
    {
        std::cout << "BTree::insert(" << key << "," << value << ")" << std::endl;

        INodePtr ptrRootNode = m_ptrCache->getObjectOfType<INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>>(*m_cktRootNodeKey);
        if (ptrRootNode == NULL)
            return ErrorCode::Error;

        int nPivot = 0;
        std::optional<CacheKeyType> ptrSibling;
        ErrorCode nRes = ptrRootNode->insert(key, value, m_ptrCache, ptrSibling, nPivot);

        if (nRes == ErrorCode::Success)
        {
            if (ptrSibling)
            {
                m_cktRootNodeKey = m_ptrCache->createObjectOfType<InternalNode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>>(5, nPivot, *m_cktRootNodeKey, *ptrSibling);
            }
        }

        return nRes;
    }

    ErrorCode remove(const KeyType& key)
    {
        LeafNodePtr ptrRootNode = NULL;
        //if (m_ptrCache->get(m_cktRootNodeKey, reinterpret_cast<IObject*&>(ptrRootNode)) != CacheErrorCode::Success)
        //    return ErrorCode::Error;

        return ptrRootNode->remove(key);
    }

    ErrorCode search(const KeyType& key, ValueType& value)
    {
        INodePtr ptrRootNode = m_ptrCache->getObjectOfType<INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>>(*m_cktRootNodeKey);
        if (ptrRootNode == NULL)
            return ErrorCode::Error;

        return ptrRootNode->search(m_ptrCache, key, value);
    }


    void print()
    {
        INodePtr ptrRootNode = m_ptrCache->getObjectOfType<INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>>(*m_cktRootNodeKey);
        if (ptrRootNode == NULL)
            return;

        ptrRootNode->print(m_ptrCache, 0);
    }
};

//template class BTree<int, int, DRAMLRUCache, uintptr_t, DRAMCacheObject>;
//template class BTree<int, std::string, DRAMLRUCache, uintptr_t, DRAMCacheObject>;
//template class BTree<int, int, DRAMLRUCache, DRAMCacheObjectKey, DRAMCacheObject>;
//template class BTree<int, std::string, DRAMLRUCache, DRAMCacheObjectKey, DRAMCacheObject>;


//template class BTree<int, int, NVRAMLRUCache, uintptr_t, NVRAMCacheObject>;
//template class BTree<int, std::string, NVRAMLRUCache, uintptr_t, NVRAMCacheObject>;
//template class BTree<int, int, NVRAMLRUCache, NVRAMCacheObjectKey, NVRAMCacheObject>;
//template class BTree<int, std::string, NVRAMLRUCache, NVRAMCacheObjectKey, NVRAMCacheObject>;
