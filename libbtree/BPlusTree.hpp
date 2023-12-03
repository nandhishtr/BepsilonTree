#pragma once

#include <iostream>
#include <stack>

#include "ErrorCodes.h"
#include "LeafNode.h"

template <typename KeyType, typename ValueType, typename CacheType>
class BPlusTree
{
    typedef CacheType::KeyType CacheKeyType;
    typedef CacheType::CacheValueType CacheValueType;

    typedef LeafNodeEx<KeyType, ValueType> LeadNodeType;
    typedef InternalNodeEx<KeyType, ValueType, CacheKeyType> InternalNodeType;

    //typedef std::shared_ptr<CacheType> CacheTypePtr;
    //typedef std::shared_ptr<INode<KeyType, ValueType, CacheType>> INodePtr;
    //typedef std::shared_ptr<LeafNode<KeyType, ValueType, CacheType>> LeafNodePtr;
    //typedef std::shared_ptr<InternalNode<KeyType, ValueType, CacheType>> InternalNodePtr;

private:
    uint32_t m_nDegree;
    std::shared_ptr<CacheType> m_ptrCache;
    std::optional<CacheKeyType> m_cktRootNodeKey;

public:
    ~BPlusTree()
    {
    }

    template<typename... Args>
    BPlusTree(uint32_t nDegree, Args... CacheArgs)
        : m_nDegree(nDegree)
    {    
        m_ptrCache = std::make_shared<CacheType>(CacheArgs...);
        m_cktRootNodeKey = m_ptrCache->template createObjectOfType<LeafNodeEx<KeyType, ValueType>>();
    }

    ErrorCode insert(const KeyType& key, const ValueType& value)
    {
        std::vector<std::pair<CacheKeyType, CacheValueType>> vtNodes;
        CacheValueType ptrLastNode = nullptr, ptrCurrentNode = nullptr;
        CacheKeyType ptrLastNodeKey, ptrCurrentNodeKey = m_cktRootNodeKey.value();

        do
        {
            ptrCurrentNode = m_ptrCache->getObjectOfType(ptrCurrentNodeKey);    //TODO: lock

            if (ptrCurrentNode == nullptr)
            {
                throw new exception("should not occur!");
            }

            if (std::holds_alternative<shared_ptr<InternalNodeType>>(*ptrCurrentNode))
            {
                shared_ptr<InternalNodeType> ptrNodeData = std::get<shared_ptr<InternalNodeType>>(*ptrCurrentNode);

                if (ptrNodeData->canTriggerSplit(m_nDegree))
                {
                    vtNodes.push_back(std::pair<CacheKeyType, CacheValueType>(ptrLastNodeKey, ptrLastNode));
                }
                else
                {
                    vtNodes.clear(); //TODO: release locks
                }

                ptrLastNodeKey = ptrCurrentNodeKey;
                ptrLastNode = ptrCurrentNode;

                ptrCurrentNodeKey = ptrNodeData->getChild(key);
            }
            else if (std::holds_alternative<shared_ptr<LeadNodeType>>(*ptrCurrentNode))
            {
                shared_ptr<LeadNodeType> ptrNodeData = std::get<shared_ptr<LeadNodeType>>(*ptrCurrentNode);

                ptrNodeData->insert(key, value);

                if (ptrNodeData->requireSplit(m_nDegree))
                {
                    vtNodes.push_back(std::pair<CacheKeyType, CacheValueType>(ptrLastNodeKey, ptrLastNode));
                    vtNodes.push_back(std::pair<CacheKeyType, CacheValueType>(ptrCurrentNodeKey, ptrCurrentNode));
                }
                else
                {
                    vtNodes.clear(); //TODO: release locks
                }

                break;
            }
        } while (true);

        KeyType pivotKey;
        CacheKeyType ptrLHSValue;
        std::optional<CacheKeyType> ptrRHSNodeKey;

        while (vtNodes.size() > 0) 
        {
            std::pair<CacheKeyType, CacheValueType> ptrNode = vtNodes.back();

            if (ptrNode.second == nullptr)
            {
                if (vtNodes.size() != 1)
                {
                    throw new exception("should not occur!");
                }

                m_cktRootNodeKey = m_ptrCache->template createObjectOfType<InternalNodeType>(pivotKey, ptrLHSValue, *ptrRHSNodeKey);
                break;
            }

            if (std::holds_alternative<shared_ptr<InternalNodeType>>(*ptrNode.second))
            {
                shared_ptr<InternalNodeType> ptrNodeData = std::get<shared_ptr<InternalNodeType>>(*ptrNode.second);

                ptrNodeData->insert(pivotKey, *ptrRHSNodeKey);

                ptrRHSNodeKey = std::nullopt;

                if (ptrNodeData->requireSplit(m_nDegree))
                {
                    ErrorCode ret = ptrNodeData->template split<std::shared_ptr<CacheType>>(m_ptrCache, ptrRHSNodeKey, pivotKey);

                    if (ret != ErrorCode::Success)
                    {
                        throw new exception("should not occur!");
                    }
                }
            }
            else if (std::holds_alternative<shared_ptr<LeadNodeType>>(*ptrNode.second))
            {
                shared_ptr<LeadNodeType> ptrNodeData = std::get<shared_ptr<LeadNodeType>>(*ptrNode.second);

                ErrorCode ret = ptrNodeData->template split<std::shared_ptr<CacheType>, CacheKeyType>(m_ptrCache, ptrRHSNodeKey, pivotKey);

                if (ret != ErrorCode::Success)
                {
                    throw new exception("should not occur!");
                }                
            }

            ptrLHSValue = ptrNode.first;

            vtNodes.pop_back();
        }

        return ErrorCode::Success;
    }

    ErrorCode search(const KeyType& key, ValueType& value)
    {
        CacheKeyType ptrNodeKey = m_cktRootNodeKey.value();
        do
        {
            CacheValueType ptrNode = m_ptrCache->getObjectOfType(ptrNodeKey);    //TODO: lock

            if (ptrNode == nullptr)
            {
                throw new exception("should not occur!");
            }

            if (std::holds_alternative<shared_ptr<InternalNodeType>>(*ptrNode))
            {
                shared_ptr<InternalNodeType> ptrNodeData = std::get<shared_ptr<InternalNodeType>>(*ptrNode);

                ptrNodeKey = ptrNodeData->getChildNodeIndex(key);
            }
            else if (std::holds_alternative<shared_ptr<LeadNodeType>>(*ptrNode))
            {
                shared_ptr<LeadNodeType> ptrNodeData = std::get<shared_ptr<LeadNodeType>>(*ptrNode);

                ErrorCode ret = ptrNodeData->getValue(key, value);

                break;
            }
        } while (true);
    }
};