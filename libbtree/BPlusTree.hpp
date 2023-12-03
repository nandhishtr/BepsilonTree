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

private:
    uint32_t m_nDegree;
    std::shared_ptr<CacheType> m_ptrCache;
    std::optional<CacheKeyType> m_cktRootNodeKey;

public:
    ~BPlusTree()
    {
    }

    template<typename... CacheArgs>
    BPlusTree(uint32_t nDegree, CacheArgs... args)
        : m_nDegree(nDegree)
    {    
        m_ptrCache = std::make_shared<CacheType>(args...);
    }

    template <typename DefaultNodeType>
    void init()
    {
        m_cktRootNodeKey = m_ptrCache->template createObjectOfType<DefaultNodeType>();
    }

    template <typename InternalNodeType, typename LeadNodeType>
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

                ptrCurrentNodeKey = ptrNodeData->getChild(key); // fid it.. there are two kinds of methods..
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

    template <typename InternalNodeType, typename LeadNodeType>
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

    template <typename InternalNodeType, typename LeadNodeType>
    ErrorCode remove(const KeyType& key)
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

                if (ptrNodeData->canTriggerMerge(m_nDegree))
                {
                    vtNodes.push_back(std::pair<CacheKeyType, CacheValueType>(ptrLastNodeKey, ptrLastNode));
                }
                else
                {
                    vtNodes.clear(); //TODO: release locks
                }

                ptrLastNodeKey = ptrCurrentNodeKey;
                ptrLastNode = ptrCurrentNode;

                ptrCurrentNodeKey = ptrNodeData->getChildNodeIndex(key); // fid it.. there are two kinds of methods..
            }
            else if (std::holds_alternative<shared_ptr<LeadNodeType>>(*ptrCurrentNode))
            {
                shared_ptr<LeadNodeType> ptrNodeData = std::get<shared_ptr<LeadNodeType>>(*ptrCurrentNode);

                ptrNodeData->remove(key);

                if (ptrNodeData->requireMerge(m_nDegree))
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

        CacheKeyType childKey;
        CacheValueType ptrChildValue = nullptr;

        while (vtNodes.size() > 0)
        {
            std::pair<CacheKeyType, CacheValueType> ptrCurrentNode = vtNodes.back();

            if (ptrCurrentNode.second == nullptr)
            {
                if (vtNodes.size() != 1)
                {
                    throw new exception("should not occur!");
                }

                //m_cktRootNodeKey = m_ptrCache->template createObjectOfType<InternalNodeType>(pivotKey, ptrLHSValue, *ptrRHSNodeKey);
                break;
            }

            if (std::holds_alternative<shared_ptr<InternalNodeType>>(*ptrCurrentNode.second))
            {
                shared_ptr<InternalNodeType> ptrParentNodeData = std::get<shared_ptr<InternalNodeType>>(*ptrCurrentNode.second);

                if (std::holds_alternative<shared_ptr<InternalNodeType>>(*ptrChildValue))
                {
                    std::optional<CacheKeyType> dlt = std::nullopt;

                    shared_ptr<InternalNodeType> ptrNodeData = std::get<shared_ptr<InternalNodeType>>(*ptrChildValue);

                    if (ptrNodeData->requireMerge(m_nDegree))
                    {
                        ptrParentNodeData->template rebalance<std::shared_ptr<CacheType>, shared_ptr<InternalNodeType>>(m_ptrCache, ptrNodeData, childKey, key, m_nDegree, dlt);

                        if (dlt)
                        {
                            m_ptrCache->remove(*dlt);
                        }
                    }
                }
                else if (std::holds_alternative<shared_ptr<LeadNodeType>>(*ptrChildValue))
                {
                    std::optional<CacheKeyType> dlt = std::nullopt;

                    shared_ptr<LeadNodeType> ptrNodeData = std::get<shared_ptr<LeadNodeType>>(*ptrChildValue);
                    ptrParentNodeData->template rebalance<std::shared_ptr<CacheType>, shared_ptr<LeadNodeType>>(m_ptrCache, ptrNodeData, childKey, key, m_nDegree, dlt);

                    if (dlt)
                    {
                        m_ptrCache->remove(*dlt);
                    }
                }

              /*  ptrNodeData->insert(pivotKey, *ptrRHSNodeKey);

                ptrRHSNodeKey = std::nullopt;

                if (ptrNodeData->requireSplit(m_nDegree))
                {
                    ErrorCode ret = ptrNodeData->template split<std::shared_ptr<CacheType>>(m_ptrCache, ptrRHSNodeKey, pivotKey);

                    if (ret != ErrorCode::Success)
                    {
                        throw new exception("should not occur!");
                    }
                }*/
            }

            childKey = ptrCurrentNode.first;
            ptrChildValue = ptrCurrentNode.second;

            vtNodes.pop_back();
        }

        return ErrorCode::Success;
    }
};