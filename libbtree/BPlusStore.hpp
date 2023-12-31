#pragma once
#include <memory>
#include <iostream>
#include <stack>
#include <optional>
#include <mutex>
#include <shared_mutex>
#include <syncstream>
#include <thread>
#include <cmath>
#include <exception>
#include <variant>
#include <unordered_map>
#include "CacheErrorCodes.h"
#include "ErrorCodes.h"
#include "VariadicNthType.h"
#include <tuple>

#include <iostream>
#include <fstream>
#include <assert.h>
//#define __CONCURRENT__
#define __TREE_AWARE_CACHE__

#ifdef __TREE_AWARE_CACHE__
template <typename ICallback, typename KeyType, typename ValueType, typename CacheType>
class BPlusStore : public ICallback
#else // !__TREE_AWARE_CACHE__
template <typename KeyType, typename ValueType, typename CacheType>
class BPlusStore
#endif __TREE_AWARE_CACHE__
{
    typedef CacheType::ObjectUIDType ObjectUIDType;
    typedef CacheType::ObjectType ObjectType;
    typedef CacheType::ObjectTypePtr ObjectTypePtr;

    using DataNodeType = typename std::tuple_element<0, typename ObjectType::ObjectCoreTypes>::type;
    using IndexNodeType = typename std::tuple_element<1, typename ObjectType::ObjectCoreTypes>::type;

private:
    uint32_t m_nDegree;
    std::shared_ptr<CacheType> m_ptrCache;
    std::optional<ObjectUIDType> m_uidRootNode;

#ifdef __CONCURRENT__
    mutable std::shared_mutex m_mutex;
#endif __CONCURRENT__

public:
    ~BPlusStore()
    {
    }

    template<typename... CacheArgs>
    BPlusStore(uint32_t nDegree, CacheArgs... args)
        : m_nDegree(nDegree)
        , m_uidRootNode(std::nullopt)
    {
        m_ptrCache = std::make_shared<CacheType>(args...);
    }

    template <typename DefaultNodeType>
    void init()
    {
#ifdef __TREE_AWARE_CACHE__
        m_ptrCache->init(this);
#endif __TREE_AWARE_CACHE__

        m_ptrCache->template createObjectOfType<DefaultNodeType>(m_uidRootNode);
    }

    ErrorCode insert(const KeyType& key, const ValueType& value)
    {
        std::vector<std::pair<ObjectUIDType, ObjectTypePtr>> vtAccessedNodes;

#ifdef __CONCURRENT__
        std::vector<std::unique_lock<std::shared_mutex>> vtLocks;
#endif __CONCURRENT__

        ObjectUIDType uidLastNode, uidCurrentNode;  // TODO: make Optional!
        ObjectTypePtr ptrLastNode = nullptr, ptrCurrentNode = nullptr;

        std::vector<std::pair<ObjectUIDType, ObjectTypePtr>> vtNodes;

#ifdef __CONCURRENT__
        vtLocks.push_back(std::unique_lock<std::shared_mutex>(m_mutex));
#endif __CONCURRENT__

        uidCurrentNode = m_uidRootNode.value();

        do
        {
            std::optional<ObjectUIDType> uidUpdated = std::nullopt;
            m_ptrCache->getObject(uidCurrentNode, ptrCurrentNode, uidUpdated);    //TODO: lock

            if (uidUpdated != std::nullopt)
            {
                if (ptrLastNode != nullptr)
                {
                    if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*ptrLastNode->data))
                    {
                        std::shared_ptr<IndexNodeType> ptrIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*ptrLastNode->data);
                        ptrIndexNode->updateChildUID(uidCurrentNode, *uidUpdated);
                    }
                    else //if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*ptrLastNode->data))
                    {
                        throw new std::exception("should not occur!");
                    }
                }
                else
                {
                    assert(uidCurrentNode == *m_uidRootNode);
                    m_uidRootNode = uidUpdated;
                }

                uidCurrentNode = *uidUpdated;
            }

#ifdef __CONCURRENT__
            vtLocks.push_back(std::unique_lock<std::shared_mutex>(ptrCurrentNode->mutex));
#endif __CONCURRENT__

            if (ptrCurrentNode == nullptr)
            {
                throw new std::exception("should not occur!");   // TODO: critical log.
            }

            vtAccessedNodes.push_back(std::make_pair(uidCurrentNode, ptrCurrentNode));

            if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*ptrCurrentNode->data))
            {
                std::shared_ptr<IndexNodeType> ptrIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*ptrCurrentNode->data);

                if (ptrIndexNode->canTriggerSplit(m_nDegree))
                {
                    vtNodes.push_back(std::pair<ObjectUIDType, ObjectTypePtr>(uidLastNode, ptrLastNode));
                }
                else
                {
#ifdef __CONCURRENT__
                    vtLocks.erase(vtLocks.begin(), vtLocks.begin() + vtLocks.size() - 1);
#endif __CONCURRENT__
                    vtNodes.clear(); //TODO: release locks
                }

                uidLastNode = uidCurrentNode;
                ptrLastNode = ptrCurrentNode;

                uidCurrentNode = ptrIndexNode->getChild(key);
            }
            else if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*ptrCurrentNode->data))
            {
                std::shared_ptr<DataNodeType> ptrDataNode = std::get<std::shared_ptr<DataNodeType>>(*ptrCurrentNode->data);

                if (ptrDataNode->insert(key, value) != ErrorCode::Success)
                {
                    vtNodes.clear();

#ifdef __CONCURRENT__
                    vtLocks.clear();
#endif __CONCURRENT__
                    return ErrorCode::InsertFailed;
                }

                if (ptrDataNode->requireSplit(m_nDegree))
                {
                    vtNodes.push_back(std::pair<ObjectUIDType, ObjectTypePtr>(uidLastNode, ptrLastNode));
                    vtNodes.push_back(std::pair<ObjectUIDType, ObjectTypePtr>(uidCurrentNode, ptrCurrentNode));
                }
                else
                {
#ifdef __CONCURRENT__
                    vtLocks.clear();
#endif __CONCURRENT__
                    vtNodes.clear(); //TODO: release locks
                }

                break;
            }
        } while (true);

        KeyType pivotKey;
        ObjectUIDType uidLHSNode;
        std::optional<ObjectUIDType> uidRHSNode;

        while (vtNodes.size() > 0)
        {
            std::pair<ObjectUIDType, ObjectTypePtr> prNodeDetails = vtNodes.back();

            if (prNodeDetails.second == nullptr)
            {
                if (vtNodes.size() != 1)
                {
                    throw new std::exception("should not occur!");
                }

                m_ptrCache->template createObjectOfType<IndexNodeType>(m_uidRootNode, pivotKey, uidLHSNode, *uidRHSNode);
                break;
            }

            if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*prNodeDetails.second->data))
            {
                std::shared_ptr<IndexNodeType> ptrIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*prNodeDetails.second->data);

                if (ptrIndexNode->insert(pivotKey, *uidRHSNode) != ErrorCode::Success)
                {
                    // TODO: Should update be performed on cloned objects first?
                    throw new std::exception("should not occur!"); // for the time being!
                }

                uidRHSNode = std::nullopt;

                if (ptrIndexNode->requireSplit(m_nDegree))
                {
                    ErrorCode errCode = ptrIndexNode->template split<std::shared_ptr<CacheType>>(m_ptrCache, uidRHSNode, pivotKey);

                    if (errCode != ErrorCode::Success)
                    {
                        // TODO: Should update be performed on cloned objects first?
                        throw new std::exception("should not occur!"); // for the time being!
                    }
                }
            }
            else if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*prNodeDetails.second->data))
            {
                std::shared_ptr<DataNodeType> ptrDataNode = std::get<std::shared_ptr<DataNodeType>>(*prNodeDetails.second->data);

                ErrorCode errCode = ptrDataNode->template split<std::shared_ptr<CacheType>, ObjectUIDType>(m_ptrCache, uidRHSNode, pivotKey);

                if (errCode != ErrorCode::Success)
                {
                    // TODO: Should update be performed on cloned objects first?
                    throw new std::exception("should not occur!"); // for the time being!
                }
            }

            uidLHSNode = prNodeDetails.first;

            vtNodes.pop_back();
        }

        m_ptrCache->reorder(vtAccessedNodes);
        vtAccessedNodes.clear();

        return ErrorCode::Success;
    }

    ErrorCode search(const KeyType& key, ValueType& value)
    {
        ErrorCode errCode = ErrorCode::Error;

        std::vector<std::pair<ObjectUIDType, ObjectTypePtr>> vtAccessedNodes;

#ifdef __CONCURRENT__
        std::vector<std::shared_lock<std::shared_mutex>> vtLocks;
        vtLocks.push_back(std::shared_lock<std::shared_mutex>(m_mutex));
#endif __CONCURRENT__

        ObjectUIDType uidCurrentNode = *m_uidRootNode;
        do
        {
            ObjectTypePtr prNodeDetails = nullptr;

            std::optional<ObjectUIDType> uidUpdated = std::nullopt;
            m_ptrCache->getObject(uidCurrentNode, prNodeDetails, uidUpdated);    //TODO: lock

            if (uidUpdated != std::nullopt)
            {
                ObjectTypePtr ptrLastNode = vtAccessedNodes.size() > 0 ? vtAccessedNodes[vtAccessedNodes.size() - 1].second : nullptr;
                if (ptrLastNode != nullptr)
                {
                    if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*ptrLastNode->data))
                    {
                        std::shared_ptr<IndexNodeType> ptrIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*ptrLastNode->data);
                        ptrIndexNode->updateChildUID(uidCurrentNode, *uidUpdated);
                    }
                    else //if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*ptrLastNode->data))
                    {
                        throw new std::exception("should not occur!");
                    }
                }
                else
                {
                    assert(uidCurrentNode == *m_uidRootNode);
                    m_uidRootNode = uidUpdated;
                }

                uidCurrentNode = *uidUpdated;
            }



#ifdef __CONCURRENT__
            vtLocks.push_back(std::shared_lock<std::shared_mutex>(prNodeDetails->mutex));
            vtLocks.erase(vtLocks.begin());
#endif __CONCURRENT__

            if (prNodeDetails == nullptr)
            {
                throw new std::exception("should not occur!");
            }

            vtAccessedNodes.push_back(std::make_pair(uidCurrentNode, prNodeDetails));

            if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*prNodeDetails->data))
            {
                std::shared_ptr<IndexNodeType> ptrIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*prNodeDetails->data);

                uidCurrentNode = ptrIndexNode->getChild(key);
            }
            else if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*prNodeDetails->data))
            {
                std::shared_ptr<DataNodeType> ptrDataNode = std::get<std::shared_ptr<DataNodeType>>(*prNodeDetails->data);

                errCode = ptrDataNode->getValue(key, value);

                break;
            }


        } while (true);

        m_ptrCache->reorder(vtAccessedNodes);
        vtAccessedNodes.clear();

        return errCode;
    }

    ErrorCode remove(const KeyType& key)
    {   
        std::vector<std::pair<ObjectUIDType, ObjectTypePtr>> vtAccessedNodes;

#ifdef __CONCURRENT__
        std::vector<std::unique_lock<std::shared_mutex>> vtLocks;
#endif __CONCURRENT__

        ObjectUIDType uidLastNode, uidCurrentNode;
        ObjectTypePtr ptrLastNode = nullptr, ptrCurrentNode = nullptr;

        std::vector<std::pair<ObjectUIDType, ObjectTypePtr>> vtNodes;

#ifdef __CONCURRENT__
        vtLocks.push_back(std::unique_lock<std::shared_mutex>(m_mutex));
#endif __CONCURRENT__

        uidCurrentNode = m_uidRootNode.value();

        do
        {
            std::optional<ObjectUIDType> uidUpdated = std::nullopt;
            m_ptrCache->getObject(uidCurrentNode, ptrCurrentNode, uidUpdated);    //TODO: lock
            

            if (uidUpdated != std::nullopt)
            {
                if (ptrLastNode != nullptr)
                {
                    if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*ptrLastNode->data))
                    {
                        std::shared_ptr<IndexNodeType> ptrIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*ptrLastNode->data);
                        ptrIndexNode->updateChildUID(uidCurrentNode, *uidUpdated);
                    }
                    else //if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*ptrLastNode->data))
                    {
                        throw new std::exception("should not occur!");
                    }
                }
                else
                {
                    assert(uidCurrentNode == *m_uidRootNode);
                    m_uidRootNode = uidUpdated;
                }

                uidCurrentNode = *uidUpdated;
            }



#ifdef __CONCURRENT__
            vtLocks.push_back(std::unique_lock<std::shared_mutex>(ptrCurrentNode->mutex));
#endif __CONCURRENT__

            if (ptrCurrentNode == nullptr)
            {
                throw new std::exception("should not occur!");
            }

            vtAccessedNodes.push_back(std::make_pair(uidCurrentNode, ptrCurrentNode));

            if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*ptrCurrentNode->data))
            {
                std::shared_ptr<IndexNodeType> ptrIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*ptrCurrentNode->data);

                if (ptrIndexNode->canTriggerMerge(m_nDegree))
                {
                    vtNodes.push_back(std::pair<ObjectUIDType, ObjectTypePtr>(uidLastNode, ptrLastNode));
                }
                else
                {
#ifdef __CONCURRENT__
                    if (vtLocks.size() > 3) //TODO: 3 seems to be working.. but how and why.. investiage....!
                    {
                        vtLocks.erase(vtLocks.begin());
                    }
#endif __CONCURRENT__
                    vtNodes.clear(); //TODO: release locks
                }

                uidLastNode = uidCurrentNode;
                ptrLastNode = ptrCurrentNode;

                uidCurrentNode = ptrIndexNode->getChild(key); // fid it.. there are two kinds of methods..
            }
            else if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*ptrCurrentNode->data))
            {
                std::shared_ptr<DataNodeType> ptrDataNode = std::get<std::shared_ptr<DataNodeType>>(*ptrCurrentNode->data);

                if (ptrDataNode->remove(key) == ErrorCode::KeyDoesNotExist)
                {
                    throw new std::exception("should not occur!");
                }

                if (ptrDataNode->requireMerge(m_nDegree))
                {
                    vtNodes.push_back(std::pair<ObjectUIDType, ObjectTypePtr>(uidLastNode, ptrLastNode));
                    vtNodes.push_back(std::pair<ObjectUIDType, ObjectTypePtr>(uidCurrentNode, ptrCurrentNode));
                }
                else
                {
#ifdef __CONCURRENT__
                    vtLocks.clear();
#endif __CONCURRENT__
                    vtNodes.clear(); //TODO: release locks
                }

                break;
            }
        } while (true);

        ObjectUIDType uidChildNode;
        ObjectTypePtr ptrChildNode = nullptr;

        while (vtNodes.size() > 0)
        {
            std::pair<ObjectUIDType, ObjectTypePtr> prNodeDetails = vtNodes.back();

            if (prNodeDetails.second == nullptr)
            {
                if (vtNodes.size() != 1)
                {
                    throw new std::exception("should not occur!");
                }

                if (*m_uidRootNode != uidChildNode)
                {
                    throw new std::exception("should not occur!");
                }

                ObjectTypePtr ptrCurrentRoot;
                std::optional<ObjectUIDType> uidUpdated = std::nullopt;
                m_ptrCache->getObject(*m_uidRootNode, ptrCurrentRoot, uidUpdated);

                assert(uidUpdated == std::nullopt);

                if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*ptrCurrentRoot->data))
                {
                    std::shared_ptr<IndexNodeType> ptrInnerNode = std::get<std::shared_ptr<IndexNodeType>>(*ptrCurrentRoot->data);
                    if (ptrInnerNode->getKeysCount() == 0) {
                        ObjectUIDType _tmp = ptrInnerNode->getChildAt(0);
                        m_ptrCache->remove(*m_uidRootNode);
                        m_uidRootNode = _tmp;

                    }
                }
                else if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*ptrCurrentRoot->data))
                {
                    int i = 0;
                    /*std::shared_ptr<DataNodeType> ptrDataNode = std::get<std::shared_ptr<DataNodeType>>(*ptrCurrentRoot->data);
                    if (ptrDataNode->getKeysCount() == 0) {
                        m_ptrCache->remove(*m_uidRootNode);
                    }*/
                }

                break;
            }

            if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*prNodeDetails.second->data))
            {
                std::shared_ptr<IndexNodeType> ptrParentIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*prNodeDetails.second->data);

                if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*ptrChildNode->data))
                {
                    std::optional<ObjectUIDType> uidToDelete = std::nullopt;

                    std::shared_ptr<IndexNodeType> ptrChildIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*ptrChildNode->data);

                    if (ptrChildIndexNode->requireMerge(m_nDegree))
                    {
                        ptrParentIndexNode->template rebalanceIndexNode<std::shared_ptr<CacheType>, shared_ptr<IndexNodeType>>(m_ptrCache, uidChildNode, ptrChildIndexNode, key, m_nDegree, uidToDelete);

                        if (uidToDelete)
                        {
#ifdef __CONCURRENT__
                            if (*uidToDelete == uidChildNode)
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
#endif __CONCURRENT__

                            m_ptrCache->remove(*uidToDelete);
                        }
                    }
                }
                else if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*ptrChildNode->data))
                {
                    std::optional<ObjectUIDType> uidToDelete = std::nullopt;

                    std::shared_ptr<DataNodeType> ptrChildDataNode = std::get<std::shared_ptr<DataNodeType>>(*ptrChildNode->data);

                    ptrParentIndexNode->template rebalanceDataNode<std::shared_ptr<CacheType>, shared_ptr<DataNodeType>>(m_ptrCache, uidChildNode, ptrChildDataNode, key, m_nDegree, uidToDelete);

                    if (uidToDelete)
                    {
#ifdef __CONCURRENT__
                        if (*uidToDelete == uidChildNode)
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
#endif __CONCURRENT__

                        m_ptrCache->remove(*uidToDelete);
                    }
                }
            }

            uidChildNode = prNodeDetails.first;
            ptrChildNode = prNodeDetails.second; 
            vtNodes.pop_back();
        }
        
        m_ptrCache->reorder(vtAccessedNodes, false);
        vtAccessedNodes.clear();

        return ErrorCode::Success;
    }

    void print(std::ofstream & out)
    {
        int nSpace = 7;

        std::string prefix;

        out << prefix << "|" << std::endl;
        out << prefix << "|" << std::string(nSpace, '-').c_str() << "(root)";

        out << std::endl;

        ObjectTypePtr ptrRootNode = nullptr;
        m_ptrCache->getObject(m_uidRootNode.value(), ptrRootNode);

        if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*ptrRootNode->data))
        {
            std::shared_ptr<IndexNodeType> ptrIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*ptrRootNode->data);

            ptrIndexNode->template print<std::shared_ptr<CacheType>, ObjectTypePtr, DataNodeType>(out, m_ptrCache, 0, prefix);
        }
        else if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*ptrRootNode->data))
        {
            std::shared_ptr<DataNodeType> ptrDataNode = std::get<std::shared_ptr<DataNodeType>>(*ptrRootNode->data);

            ptrDataNode->print(out, 0, prefix);
        }
    }

    void getCacheState(size_t& lru, size_t& map)
    {
        return m_ptrCache->getCacheState(lru, map);
    }

#ifdef __TREE_AWARE_CACHE__
public:
    CacheErrorCode updateChildUID(const std::optional<ObjectUIDType>& uidObject, const ObjectUIDType& uidChildOld, const ObjectUIDType& uidChildNew)
    {
        return CacheErrorCode::Success;
    }

    CacheErrorCode updateChildUID(std::vector<std::pair<ObjectUIDType, std::pair<ObjectUIDType, ObjectUIDType>>> vtUpdatedUIDs)
    {
        throw new std::exception("no implementation!");
    }

    void applyExistingUpdates(std::vector<std::pair<ObjectUIDType, std::pair<ObjectUIDType, std::shared_ptr<ObjectType>>>>& vtNodes
        , std::unordered_map<ObjectUIDType, std::optional<ObjectUIDType>>& mpUpdatedUIDs)
    {
        /*auto it = vtNodes.begin();
        while (it != vtNodes.end())
        {
            if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*(*it).second.second->data))
            {
                std::shared_ptr<IndexNodeType> ptrIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*(*it).second.second->data);

                auto it_children = ptrIndexNode->m_ptrData->m_vtChildren.begin();
                while (it_children != ptrIndexNode->m_ptrData->m_vtChildren.end())
                {
                    if (mpUpdatedUIDs.find(*it_children) != mpUIDUpdates.end())
                    {
                        *it_children = *mpUpdatedUIDs.find[*it_children];
                        mpUpdatedUIDs.erase(*it_children);  // Update applied!
                    }
                    it_children++;
                }
            }
            else //if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*(*it).second.second->data))
            {
                throw new std::exception("should not occur!");   // TODO: critical log.
            }
            it++;
        }*/
    }

#endif __TREE_AWARE_CACHE__
};