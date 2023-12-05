#pragma once

#include <iostream>
#include <stack>

#include "ErrorCodes.h"

template <typename KeyType, typename ValueType, typename CacheType>
class BPlusStore
{
    typedef CacheType::KeyType CacheKeyType;
    typedef CacheType::CacheValueType CacheValueType;

private:
    uint32_t m_nDegree;
    std::shared_ptr<CacheType> m_ptrCache;
    std::optional<CacheKeyType> m_cktRootNodeKey;

public:
    ~BPlusStore()
    {
    }

    template<typename... CacheArgs>
    BPlusStore(uint32_t nDegree, CacheArgs... args)
        : m_nDegree(nDegree)
    {    
        m_ptrCache = std::make_shared<CacheType>(args...);
    }

    template <typename DefaultNodeType>
    void init()
    {
        m_cktRootNodeKey = m_ptrCache->template createObjectOfType<DefaultNodeType>();
    }

    template <typename IndexNodeType, typename DataNodeType>
    ErrorCode insert(const KeyType& key, const ValueType& value)
    {
        std::vector<std::pair<CacheKeyType, CacheValueType>> vtNodes;
        CacheValueType ptrLastNode = nullptr, prNodeDetails = nullptr;
        CacheKeyType ckLastNode, ckCurrentNode = m_cktRootNodeKey.value();

        do
        {
            prNodeDetails = m_ptrCache->getObjectOfType(ckCurrentNode);    //TODO: lock

            if (prNodeDetails == nullptr)
            {
                throw new exception("should not occur!");   // TODO: critical log.
            }

            if (std::holds_alternative<shared_ptr<IndexNodeType>>(*prNodeDetails))
            {
                shared_ptr<IndexNodeType> ptrIndexNode = std::get<shared_ptr<IndexNodeType>>(*prNodeDetails);

                if (ptrIndexNode->canTriggerSplit(m_nDegree))
                {
                    vtNodes.push_back(std::pair<CacheKeyType, CacheValueType>(ckLastNode, ptrLastNode));
                }
                else
                {
                    vtNodes.clear(); //TODO: release locks
                }

                ckLastNode = ckCurrentNode;
                ptrLastNode = prNodeDetails;

                ckCurrentNode = ptrIndexNode->getChild(key); // fid it.. there are two kinds of methods..
            }
            else if (std::holds_alternative<shared_ptr<DataNodeType>>(*prNodeDetails))
            {
                shared_ptr<DataNodeType> ptrDataNode = std::get<shared_ptr<DataNodeType>>(*prNodeDetails);

                ptrDataNode->insert(key, value);

                if (ptrDataNode->requireSplit(m_nDegree))
                {
                    vtNodes.push_back(std::pair<CacheKeyType, CacheValueType>(ckLastNode, ptrLastNode));
                    vtNodes.push_back(std::pair<CacheKeyType, CacheValueType>(ckCurrentNode, prNodeDetails));
                }
                else
                {
                    vtNodes.clear(); //TODO: release locks
                }

                break;
            }
        } while (true);

        KeyType pivotKey;
        CacheKeyType ckLHSNode;
        std::optional<CacheKeyType> ckRHSNode;

        while (vtNodes.size() > 0) 
        {
            std::pair<CacheKeyType, CacheValueType> prNodeDetails = vtNodes.back();

            if (prNodeDetails.second == nullptr)
            {
                if (vtNodes.size() != 1)
                {
                    throw new exception("should not occur!");
                }

                m_cktRootNodeKey = m_ptrCache->template createObjectOfType<IndexNodeType>(pivotKey, ckLHSNode, *ckRHSNode);
                break;
            }

            if (std::holds_alternative<shared_ptr<IndexNodeType>>(*prNodeDetails.second))
            {
                shared_ptr<IndexNodeType> ptrIndexNode = std::get<shared_ptr<IndexNodeType>>(*prNodeDetails.second);

                ptrIndexNode->insert(pivotKey, *ckRHSNode);

                ckRHSNode = std::nullopt;

                if (ptrIndexNode->requireSplit(m_nDegree))
                {
                    ErrorCode errCode = ptrIndexNode->template split<std::shared_ptr<CacheType>>(m_ptrCache, ckRHSNode, pivotKey);

                    if (errCode != ErrorCode::Success)
                    {
                        throw new exception("should not occur!");
                    }
                }
            }
            else if (std::holds_alternative<shared_ptr<DataNodeType>>(*prNodeDetails.second))
            {
                shared_ptr<DataNodeType> ptrDataNode = std::get<shared_ptr<DataNodeType>>(*prNodeDetails.second);

                ErrorCode errCode = ptrDataNode->template split<std::shared_ptr<CacheType>, CacheKeyType>(m_ptrCache, ckRHSNode, pivotKey);

                if (errCode != ErrorCode::Success)
                {
                    throw new exception("should not occur!");
                }                
            }

            ckLHSNode = prNodeDetails.first;

            vtNodes.pop_back();
        }

        return ErrorCode::Success;
    }

    template <typename IndexNodeType, typename DataNodeType>
    ErrorCode search(const KeyType& key, ValueType& value)
    {
        CacheKeyType ckCurrentNode = m_cktRootNodeKey.value();
        do
        {
            CacheValueType prNodeDetails = m_ptrCache->getObjectOfType(ckCurrentNode);    //TODO: lock

            if (prNodeDetails == nullptr)
            {
                throw new exception("should not occur!");
            }

            if (std::holds_alternative<shared_ptr<IndexNodeType>>(*prNodeDetails))
            {
                shared_ptr<IndexNodeType> ptrIndexNode = std::get<shared_ptr<IndexNodeType>>(*prNodeDetails);

                ckCurrentNode = ptrIndexNode->getChild(key);
            }
            else if (std::holds_alternative<shared_ptr<DataNodeType>>(*prNodeDetails))
            {
                shared_ptr<DataNodeType> ptrDataNode = std::get<shared_ptr<DataNodeType>>(*prNodeDetails);

                ErrorCode errCode = ptrDataNode->getValue(key, value);

                break;
            }
        } while (true);
    }

    template <typename IndexNodeType, typename DataNodeType>
    ErrorCode remove(const KeyType& key)
    {
        std::vector<std::pair<CacheKeyType, CacheValueType>> vtNodes;
        CacheValueType ptrLastNode = nullptr, ptrCurrentNode = nullptr;
        CacheKeyType ckLastNode = NULL, ckCurrentNode = m_cktRootNodeKey.value();

        do
        {
            ptrCurrentNode = m_ptrCache->getObjectOfType(ckCurrentNode);    //TODO: lock

            if (ptrCurrentNode == nullptr)
            {
                throw new exception("should not occur!");
            }

            if (std::holds_alternative<shared_ptr<IndexNodeType>>(*ptrCurrentNode))
            {
                shared_ptr<IndexNodeType> ptrIndexNode = std::get<shared_ptr<IndexNodeType>>(*ptrCurrentNode);

                if (ptrIndexNode->canTriggerMerge(m_nDegree))
                {
                    vtNodes.push_back(std::pair<CacheKeyType, CacheValueType>(ckLastNode, ptrLastNode));
                }
                else
                {
                    vtNodes.clear(); //TODO: release locks
                }

                ckLastNode = ckCurrentNode;
                ptrLastNode = ptrCurrentNode;

                ckCurrentNode = ptrIndexNode->getChild(key); // fid it.. there are two kinds of methods..
            }
            else if (std::holds_alternative<shared_ptr<DataNodeType>>(*ptrCurrentNode))
            {
                shared_ptr<DataNodeType> ptrDataNode = std::get<shared_ptr<DataNodeType>>(*ptrCurrentNode);

                ptrDataNode->remove(key);

                if (ptrDataNode->requireMerge(m_nDegree))
                {
                    vtNodes.push_back(std::pair<CacheKeyType, CacheValueType>(ckLastNode, ptrLastNode));
                    vtNodes.push_back(std::pair<CacheKeyType, CacheValueType>(ckCurrentNode, ptrCurrentNode));
                }
                else
                {
                    vtNodes.clear(); //TODO: release locks
                }

                break;
            }
        } while (true);

        CacheKeyType ckChildNode;
        CacheValueType ptrChildNode = nullptr;

        while (vtNodes.size() > 0)
        {
            std::pair<CacheKeyType, CacheValueType> prNodeDetails = vtNodes.back();

            if (prNodeDetails.second == nullptr)
            {
                if (vtNodes.size() != 1)
                {
                    throw new exception("should not occur!");
                }

                break;
            }

            if (std::holds_alternative<shared_ptr<IndexNodeType>>(*prNodeDetails.second))
            {
                shared_ptr<IndexNodeType> ptrParentIndexNode = std::get<shared_ptr<IndexNodeType>>(*prNodeDetails.second);

                if (std::holds_alternative<shared_ptr<IndexNodeType>>(*ptrChildNode))
                {
                    std::optional<CacheKeyType> dlt = std::nullopt;

                    shared_ptr<IndexNodeType> ptrChildIndexNode = std::get<shared_ptr<IndexNodeType>>(*ptrChildNode);

                    if (ptrChildIndexNode->requireMerge(m_nDegree))
                    {
                        ptrParentIndexNode->template rebalanceIndexNode<std::shared_ptr<CacheType>, shared_ptr<IndexNodeType>>(m_ptrCache, ptrChildIndexNode, key, m_nDegree, ckChildNode, dlt);

                        if (dlt)
                        {
                            m_ptrCache->remove(*dlt);
                        }

                        if (ptrParentIndexNode->getKeysCount() == 0)
                        {
                            m_cktRootNodeKey = ptrParentIndexNode->getChildAt(0);
                            //throw new exception("should not occur!");
                        }
                    }
                }
                else if (std::holds_alternative<shared_ptr<DataNodeType>>(*ptrChildNode))
                {
                    std::optional<CacheKeyType> dlt = std::nullopt;

                    shared_ptr<DataNodeType> ptrChildDataNode = std::get<shared_ptr<DataNodeType>>(*ptrChildNode);
                    ptrParentIndexNode->template rebalanceDataNode<std::shared_ptr<CacheType>, shared_ptr<DataNodeType>>(m_ptrCache, ptrChildDataNode, key, m_nDegree, ckChildNode, dlt);

                    if (dlt)
                    {
                        m_ptrCache->remove(*dlt);
                    }

                    if (ptrParentIndexNode->getKeysCount() == 0)
                    {
                        m_cktRootNodeKey = ptrParentIndexNode->getChildAt(0);
                        //throw new exception("should not occur!");
                    }
                }
            }

            ckChildNode = prNodeDetails.first;
            ptrChildNode = prNodeDetails.second;

            vtNodes.pop_back();
        }

        return ErrorCode::Success;
    }

    template <typename IndexNodeType, typename DataNodeType>
    void print()
    {
        CacheValueType ptrRootNode = m_ptrCache->getObjectOfType(m_cktRootNodeKey.value());
        if (std::holds_alternative<shared_ptr<IndexNodeType>>(*ptrRootNode))
        {
            shared_ptr<IndexNodeType> ptrIndexNode = std::get<shared_ptr<IndexNodeType>>(*ptrRootNode);

            ptrIndexNode->template print<std::shared_ptr<CacheType>, CacheValueType, DataNodeType>(m_ptrCache, 0);
        }
        else if (std::holds_alternative<shared_ptr<DataNodeType>>(*ptrRootNode))
        {
            shared_ptr<DataNodeType> ptrDataNode = std::get<shared_ptr<DataNodeType>>(*ptrRootNode);

            ptrDataNode->print(0);
        }
    }
};