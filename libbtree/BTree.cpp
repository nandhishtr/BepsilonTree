#include "pch.h"
#include "BTree.h"
#include <optional>

//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//BTree<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>::~BTree()
//{
//
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//BTree<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>::BTree()
////    : m_nDegree(nDegree),
//{
//    m_ptrCache = std::make_shared<CacheType<CacheKeyType, CacheValueType>>(10000);
//    m_cktRootNodeKey = m_ptrCache->createObjectOfType<LeafNode<KeyType, ValueType, CacheType>>(5);
//
//    std::cout << "BTree::BTree()" << std::endl;
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//ErrorCode BTree<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>::init()
//{
//    std::cout << "BTree::init()" << std::endl;
//
//
//    return ErrorCode::Success;
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//ErrorCode BTree<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>::insert(const KeyType& key, const ValueType& value)
//{
//    std::cout << "BTree::insert(" << key << "," << value << ")" << std::endl;
//
//    INodePtr ptrRootNode = m_ptrCache->getObjectOfType<INode<KeyType, ValueType, CacheType>>(*m_cktRootNodeKey);
//    if (ptrRootNode == NULL)
//        return ErrorCode::Error;
//
//    int nPivot = 0;
//    std::optional<CacheKeyType> ptrSibling;
//    ErrorCode nRes = ptrRootNode->insert(key, value, m_ptrCache, ptrSibling, nPivot);
//    
//    if (nRes == ErrorCode::Success)
//    {
//        if (ptrSibling)
//        {
//            m_cktRootNodeKey = m_ptrCache->createObjectOfType<InternalNode<KeyType, ValueType, CacheType>>(5, nPivot, *m_cktRootNodeKey, *ptrSibling);
//        }
//    }
//
//    return nRes;
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//ErrorCode BTree<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>::remove(const KeyType& key)
//{
//    LeafNodePtr ptrRootNode = NULL;
//    //if (m_ptrCache->get(m_cktRootNodeKey, reinterpret_cast<IObject*&>(ptrRootNode)) != CacheErrorCode::Success)
//    //    return ErrorCode::Error;
//
//    return ptrRootNode->remove(key);
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//ErrorCode BTree<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>::search(const KeyType& key, ValueType& value)
//{
//    INodePtr ptrRootNode = m_ptrCache->getObjectOfType<INode<KeyType, ValueType, CacheType>>(*m_cktRootNodeKey);
//    if (ptrRootNode == NULL)
//        return ErrorCode::Error;
//
//    return ptrRootNode->search(m_ptrCache, key, value);
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//void BTree<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>::print()
//{
//    INodePtr ptrRootNode = m_ptrCache->getObjectOfType<INode<KeyType, ValueType, CacheType>>(*m_cktRootNodeKey);
//    if (ptrRootNode == NULL)
//        return;
//
//    ptrRootNode->print(m_ptrCache, 0);
//}