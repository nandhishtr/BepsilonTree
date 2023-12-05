#pragma once

#include <iostream>
#include <stack>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <syncstream>
#include <thread>
#include <cmath>

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
    mutable std::shared_mutex mutex;
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
        std::vector<std::unique_lock<std::shared_mutex>> vtLocks;
        std::vector<std::pair<CacheKeyType, CacheValueType>> vtNodes;
        CacheValueType ptrLastNode = nullptr, ptrCurrentNode = nullptr;
        CacheKeyType ckLastNode = NULL, ckCurrentNode = NULL;

        vtLocks.push_back(std::unique_lock<std::shared_mutex>(mutex));

        ckCurrentNode = m_cktRootNodeKey.value();

        do
        {
            ptrCurrentNode = m_ptrCache->getObjectOfType(ckCurrentNode);    //TODO: lock

            vtLocks.push_back(std::unique_lock<std::shared_mutex>(ptrCurrentNode->mutex));

            if (ptrCurrentNode == nullptr)
            {
                throw new exception("should not occur!");   // TODO: critical log.
            }

            if (std::holds_alternative<shared_ptr<IndexNodeType>>(*ptrCurrentNode->data))
            {
                shared_ptr<IndexNodeType> ptrIndexNode = std::get<shared_ptr<IndexNodeType>>(*ptrCurrentNode->data);

                if (ptrIndexNode->canTriggerSplit(m_nDegree))
                {
                    vtNodes.push_back(std::pair<CacheKeyType, CacheValueType>(ckLastNode, ptrLastNode));
                }
                else
                {
                    vtLocks.erase(vtLocks.begin(), vtLocks.begin() + vtLocks.size() - 1);
                    vtNodes.clear(); //TODO: release locks
                }

                ckLastNode = ckCurrentNode;
                ptrLastNode = ptrCurrentNode;

                ckCurrentNode = ptrIndexNode->getChild(key); // fid it.. there are two kinds of methods..
            }
            else if (std::holds_alternative<shared_ptr<DataNodeType>>(*ptrCurrentNode->data))
            {
                shared_ptr<DataNodeType> ptrDataNode = std::get<shared_ptr<DataNodeType>>(*ptrCurrentNode->data);

                ptrDataNode->insert(key, value);

                if (ptrDataNode->requireSplit(m_nDegree))
                {
                    //vtLocks.push_back(std::move(wlock));

                    vtNodes.push_back(std::pair<CacheKeyType, CacheValueType>(ckLastNode, ptrLastNode));
                    vtNodes.push_back(std::pair<CacheKeyType, CacheValueType>(ckCurrentNode, ptrCurrentNode));
                }
                else
                {
                    vtLocks.clear();
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

            if (std::holds_alternative<shared_ptr<IndexNodeType>>(*prNodeDetails.second->data))
            {
                shared_ptr<IndexNodeType> ptrIndexNode = std::get<shared_ptr<IndexNodeType>>(*prNodeDetails.second->data);

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
            else if (std::holds_alternative<shared_ptr<DataNodeType>>(*prNodeDetails.second->data))
            {
                shared_ptr<DataNodeType> ptrDataNode = std::get<shared_ptr<DataNodeType>>(*prNodeDetails.second->data);

                ErrorCode errCode = ptrDataNode->template split<std::shared_ptr<CacheType>, CacheKeyType>(m_ptrCache, ckRHSNode, pivotKey);

                if (errCode != ErrorCode::Success)
                {
                    throw new exception("should not occur!");
                }                
            }

            ckLHSNode = prNodeDetails.first;

            vtNodes.pop_back();
        }

        vtLocks.clear();

        return ErrorCode::Success;
    }

    template <typename IndexNodeType, typename DataNodeType>
    ErrorCode search(const KeyType& key, ValueType& value)
    {
        ErrorCode errCode = ErrorCode::Error;

        std::vector<std::shared_lock<std::shared_mutex>> vtLocks;
        vtLocks.push_back(std::shared_lock<std::shared_mutex>(mutex));

        CacheKeyType ckCurrentNode = m_cktRootNodeKey.value();
        do
        {
            CacheValueType prNodeDetails = m_ptrCache->getObjectOfType(ckCurrentNode);    //TODO: lock
            vtLocks.push_back(std::shared_lock<std::shared_mutex>(prNodeDetails->mutex));
            vtLocks.erase(vtLocks.begin());

            if (prNodeDetails == nullptr)
            {
                throw new exception("should not occur!");
            }

            if (std::holds_alternative<shared_ptr<IndexNodeType>>(*prNodeDetails->data))
            {
                shared_ptr<IndexNodeType> ptrIndexNode = std::get<shared_ptr<IndexNodeType>>(*prNodeDetails->data);

                ckCurrentNode = ptrIndexNode->getChild(key);
            }
            else if (std::holds_alternative<shared_ptr<DataNodeType>>(*prNodeDetails->data))
            {
                shared_ptr<DataNodeType> ptrDataNode = std::get<shared_ptr<DataNodeType>>(*prNodeDetails->data);

                errCode = ptrDataNode->getValue(key, value);

                break;
            }
        } while (true);

        vtLocks.clear();

        return errCode;
    }

    template <typename IndexNodeType, typename DataNodeType>
    ErrorCode remove(const KeyType& key)
    {
        std::vector<std::unique_lock<std::shared_mutex>> vtLocks;
        std::vector<std::pair<CacheKeyType, CacheValueType>> vtNodes;
        CacheValueType ptrLastNode = nullptr, ptrCurrentNode = nullptr;
        CacheKeyType ckLastNode = NULL, ckCurrentNode = NULL;

        vtLocks.push_back(std::unique_lock<std::shared_mutex>(mutex));

        ckCurrentNode = m_cktRootNodeKey.value();

        do
        {
            ptrCurrentNode = m_ptrCache->getObjectOfType(ckCurrentNode);    //TODO: lock
            
            vtLocks.push_back(std::unique_lock<std::shared_mutex>(ptrCurrentNode->mutex));

            if (ptrCurrentNode == nullptr)
            {
                throw new exception("should not occur!");
            }

            if (std::holds_alternative<shared_ptr<IndexNodeType>>(*ptrCurrentNode->data))
            {
                shared_ptr<IndexNodeType> ptrIndexNode = std::get<shared_ptr<IndexNodeType>>(*ptrCurrentNode->data);

                if (ptrIndexNode->canTriggerMerge(m_nDegree))
                {
                    vtNodes.push_back(std::pair<CacheKeyType, CacheValueType>(ckLastNode, ptrLastNode));
                }
                else
                {
                    if (vtLocks.size() > 3) //TODO: 3 seems to be working.. but how and why.. investiage when have time!
                    {
                        vtLocks.erase(vtLocks.begin());
                    }
                    vtNodes.clear(); //TODO: release locks
                }

                ckLastNode = ckCurrentNode;
                ptrLastNode = ptrCurrentNode;

                ckCurrentNode = ptrIndexNode->getChild(key); // fid it.. there are two kinds of methods..
            }
            else if (std::holds_alternative<shared_ptr<DataNodeType>>(*ptrCurrentNode->data))
            {
                shared_ptr<DataNodeType> ptrDataNode = std::get<shared_ptr<DataNodeType>>(*ptrCurrentNode->data);

                ptrDataNode->remove(key);

                if (ptrDataNode->requireMerge(m_nDegree))
                {
                    vtNodes.push_back(std::pair<CacheKeyType, CacheValueType>(ckLastNode, ptrLastNode));
                    vtNodes.push_back(std::pair<CacheKeyType, CacheValueType>(ckCurrentNode, ptrCurrentNode));
                }
                else
                {
                    vtLocks.clear();
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

            if (std::holds_alternative<shared_ptr<IndexNodeType>>(*prNodeDetails.second->data))
            {
                shared_ptr<IndexNodeType> ptrParentIndexNode = std::get<shared_ptr<IndexNodeType>>(*prNodeDetails.second->data);

                if (std::holds_alternative<shared_ptr<IndexNodeType>>(*ptrChildNode->data))
                {
                    std::optional<CacheKeyType> dlt = std::nullopt;

                    shared_ptr<IndexNodeType> ptrChildIndexNode = std::get<shared_ptr<IndexNodeType>>(*ptrChildNode->data);

                    if (ptrChildIndexNode->requireMerge(m_nDegree))
                    {
                        ptrParentIndexNode->template rebalanceIndexNode<std::shared_ptr<CacheType>, shared_ptr<IndexNodeType>>(m_ptrCache, ptrChildIndexNode, key, m_nDegree, ckChildNode, dlt);

                        if (dlt)
                        {
                            if (*dlt == ckChildNode)
                            {
                                auto it = vtLocks.begin();
                                while (it != vtLocks.end()) {
                                    if ((*it).mutex() == &ptrChildNode->mutex)
                                    {
                                        break;
                                    }
                                    it++;
                                }

                                if (it != vtLocks.end())
                                    vtLocks.erase(it);
                            }

                            m_ptrCache->remove(*dlt);
                        }

                        if (ptrParentIndexNode->getKeysCount() == 0)
                        {
                            m_cktRootNodeKey = ptrParentIndexNode->getChildAt(0);
                            //throw new exception("should not occur!");
                        }
                    }
                }
                else if (std::holds_alternative<shared_ptr<DataNodeType>>(*ptrChildNode->data))
                {
                    std::optional<CacheKeyType> dlt = std::nullopt;

                    shared_ptr<DataNodeType> ptrChildDataNode = std::get<shared_ptr<DataNodeType>>(*ptrChildNode->data);
                    ptrParentIndexNode->template rebalanceDataNode<std::shared_ptr<CacheType>, shared_ptr<DataNodeType>>(m_ptrCache, ptrChildDataNode, key, m_nDegree, ckChildNode, dlt);

                    if (dlt)
                    {
                        if (*dlt == ckChildNode)
                        {
                            auto it = vtLocks.begin();
                            while (it != vtLocks.end()) {
                                if ((*it).mutex() == &ptrChildNode->mutex)
                                {
                                    break;
                                }
                                it++;
                            }

                            if (it != vtLocks.end())
                                vtLocks.erase(it);
                        }

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

        //vtLocks.clear();
        return ErrorCode::Success;
    }

    template <typename IndexNodeType, typename DataNodeType>
    void print()
    {
        CacheValueType ptrRootNode = m_ptrCache->getObjectOfType(m_cktRootNodeKey.value());
        if (std::holds_alternative<shared_ptr<IndexNodeType>>(*ptrRootNode->data))
        {
            shared_ptr<IndexNodeType> ptrIndexNode = std::get<shared_ptr<IndexNodeType>>(*ptrRootNode->data);

            ptrIndexNode->template print<std::shared_ptr<CacheType>, CacheValueType, DataNodeType>(m_ptrCache, 0);
        }
        else if (std::holds_alternative<shared_ptr<DataNodeType>>(*ptrRootNode->data))
        {
            shared_ptr<DataNodeType> ptrDataNode = std::get<shared_ptr<DataNodeType>>(*ptrRootNode->data);

            ptrDataNode->print(0);
        }
    }
};