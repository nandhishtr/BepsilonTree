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

#include "CacheManager.h"

template <
    typename KeyType,
    typename ValueType,
    template <typename, template <typename> class CacheValueType, typename CacheValueCoreType> class CacheType, typename CacheKeyType,
    template <typename> class CacheValueType, typename CacheValueCoreType
>
class BTree
{
    typedef std::shared_ptr<CacheType<CacheKeyType, CacheValueType, CacheValueCoreType>> CacheTypePtr;
    typedef std::shared_ptr<INode<KeyType, ValueType, CacheType<CacheKeyType, CacheValueType, CacheValueCoreType>, CacheKeyType>> INodePtr;
    typedef std::shared_ptr<LeafNode<KeyType, ValueType, CacheType<CacheKeyType, CacheValueType, CacheValueCoreType>, CacheKeyType>> LeafNodePtr;
    typedef std::shared_ptr<InternalNode<KeyType, ValueType, CacheType<CacheKeyType, CacheValueType, CacheValueCoreType>, CacheKeyType>> InternalNodePtr;

private:
    uint32_t m_nDegree;
    std::shared_ptr<CacheType<CacheKeyType, CacheValueType, CacheValueCoreType>> m_ptrCache;
    std::optional<CacheKeyType> m_cktRootNodeKey;
    //LeafNode<KeyType, ValueType, CacheType, CacheKeyType>* m_ptrRootNode;

public:
    ~BTree()
    {

    }

    BTree()
        //    : m_nDegree(nDegree),
    {
        m_ptrCache = std::make_shared<CacheType<CacheKeyType, CacheValueType, CacheValueCoreType>>(10000);
        m_cktRootNodeKey = m_ptrCache->createObjectOfType<LeafNode<KeyType, ValueType, CacheType<CacheKeyType, CacheValueType, CacheValueCoreType>, CacheKeyType>>(5);

        std::cout << "BTree::BTree()" << std::endl;


        //CacheManager<BTree<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>, CacheType, CacheKeyType, CacheValueType>* mgr = new CacheManager<BTree<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>, CacheType, CacheKeyType, CacheValueType>(this);
        //m_cktRootNodeKey = mgr->createObjectOfType<CacheKeyType, LeafNode<KeyType, ValueType, CacheType, CacheKeyType>>(5);
        //INodePtr ptrParentNode = mgr->getObjectOfType<CacheKeyType, INode<KeyType, ValueType, CacheType, CacheKeyType>>(*m_cktRootNodeKey);
    }

    ErrorCode init()
    {
        std::cout << "BTree::init()" << std::endl;


        return ErrorCode::Success;
    }

    ErrorCode insert(const KeyType& key, const ValueType& value)
    {
        std::cout << "BTree::insert(" << key << "," << value << ")" << std::endl;

        INodePtr ptrRootNode = m_ptrCache->getObjectOfType<INode<KeyType, ValueType, CacheType<CacheKeyType, CacheValueType, CacheValueCoreType>, CacheKeyType>>(*m_cktRootNodeKey);
        if (ptrRootNode == NULL)
            return ErrorCode::Error;

        int nPivot = 0;
        std::optional<CacheKeyType> ptrSibling;
        ErrorCode nRes = ptrRootNode->insert(key, value, m_ptrCache, ptrSibling, nPivot);

        if (nRes == ErrorCode::Success)
        {
            if (ptrSibling)
            {
                m_cktRootNodeKey = m_ptrCache->createObjectOfType<InternalNode<KeyType, ValueType, CacheType<CacheKeyType, CacheValueType, CacheValueCoreType>, CacheKeyType>>(5, nPivot, *m_cktRootNodeKey, *ptrSibling);
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
        INodePtr ptrRootNode = m_ptrCache->getObjectOfType<INode<KeyType, ValueType, CacheType<CacheKeyType, CacheValueType, CacheValueCoreType>, CacheKeyType>>(*m_cktRootNodeKey);
        if (ptrRootNode == NULL)
            return ErrorCode::Error;

        return ptrRootNode->search(m_ptrCache, key, value);
    }


    void print()
    {
        INodePtr ptrRootNode = m_ptrCache->getObjectOfType<INode<KeyType, ValueType, CacheType<CacheKeyType, CacheValueType, CacheValueCoreType>, CacheKeyType>>(*m_cktRootNodeKey);
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
