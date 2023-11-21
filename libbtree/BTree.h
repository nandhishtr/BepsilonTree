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

template <typename KeyType, typename ValueType, typename CacheType>
class BTree
{
    typedef CacheType::KeyType CacheKeyType;

    typedef std::shared_ptr<CacheType> CacheTypePtr;
    typedef std::shared_ptr<INode<KeyType, ValueType, CacheType>> INodePtr;
    typedef std::shared_ptr<LeafNode<KeyType, ValueType, CacheType>> LeafNodePtr;
    typedef std::shared_ptr<InternalNode<KeyType, ValueType, CacheType>> InternalNodePtr;

private:
    uint32_t m_nDegree;
    std::shared_ptr<CacheType> m_ptrCache;
    std::optional<CacheKeyType> m_cktRootNodeKey;

public:
    ~BTree()
    {
    }

    template<typename... Args>
    BTree(uint32_t nDegree, Args... CacheArgs)
        : m_nDegree(nDegree)
    {    
        m_ptrCache = std::make_shared<CacheType>(CacheArgs...);
        m_cktRootNodeKey = m_ptrCache->template createObjectOfType<LeafNode<KeyType, ValueType, CacheType>>(m_nDegree);


        InternalNode<KeyType, ValueType, CacheType>* t0 = new InternalNode<KeyType, ValueType, CacheType>(1);
        LeafNode<KeyType, ValueType, CacheType>* t1 = new LeafNode<KeyType, ValueType, CacheType>(1);

        std::cout << std::boolalpha;
        std::cout << "->: " << std::is_standard_layout<InternalNode<KeyType, ValueType, CacheType>>::value << std::endl;
        std::cout << "->: " << std::is_standard_layout<LeafNode<KeyType, ValueType, CacheType>>::value << std::endl;

        LOG(INFO) << "BTree::BTree.";
    }

    ErrorCode insert(const KeyType& key, const ValueType& value)
    {
        LOG(INFO) << "BTree::insert(" << key << "," << value << ").";

        INodePtr ptrRootNode = m_ptrCache->template getObjectOfType<INode<KeyType, ValueType, CacheType>>(*m_cktRootNodeKey);
        if (ptrRootNode == NULL)
        {
            LOG(ERROR) << "Failed to get object.";

            return ErrorCode::Error;
        }

        int nPivot = 0;
        std::optional<CacheKeyType> ptrSibling;
        ErrorCode nRes = ptrRootNode->insert(key, value, m_ptrCache, ptrSibling, nPivot);

        if (nRes == ErrorCode::Error)
        {
            LOG(ERROR) << "Failed to insert object.";

            return ErrorCode::Error;
        }

        if (ptrSibling)
        {
            LOG(INFO) << "Update the root.";

            m_cktRootNodeKey = m_ptrCache->template createObjectOfType<InternalNode<KeyType, ValueType, CacheType>>(m_nDegree, nPivot, *m_cktRootNodeKey, *ptrSibling);

            if (!m_cktRootNodeKey)
            {
                LOG(INFO) << "Failed to create object.";
                return ErrorCode::Error;
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
        INodePtr ptrRootNode = m_ptrCache->template getObjectOfType<INode<KeyType, ValueType, CacheType>>(*m_cktRootNodeKey);
        if (ptrRootNode == NULL)
            return ErrorCode::Error;

        return ptrRootNode->search(m_ptrCache, key, value);
    }


    void print()
    {
        INodePtr ptrRootNode = m_ptrCache->template getObjectOfType<INode<KeyType, ValueType, CacheType>>(*m_cktRootNodeKey);
        if (ptrRootNode == NULL)
            return;

        ptrRootNode->print(m_ptrCache, 0);
    }
};