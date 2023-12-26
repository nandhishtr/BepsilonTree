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

//#define __CONCURRENT__
#define __POSITION_AWARE_ITEMS__

#ifdef __POSITION_AWARE_ITEMS__
template <typename ICallback, typename KeyType, typename ValueType, typename CacheType>
class BPlusStore : public ICallback
#else //__POSITION_AWARE_ITEMS__
template <typename KeyType, typename ValueType, typename CacheType>
class BPlusStore
#endif __POSITION_AWARE_ITEMS__
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
#ifdef __POSITION_AWARE_ITEMS__
        m_ptrCache->init(this);

        std::optional<ObjectUIDType> uidParent = std::nullopt;
        m_ptrCache->template createObjectOfType<DefaultNodeType>(m_uidRootNode, uidParent);
#else
        m_ptrCache->template createObjectOfType<DefaultNodeType>(m_uidRootNode);
#endif __POSITION_AWARE_ITEMS__
    }

    ErrorCode insert(const KeyType& key, const ValueType& value)
    {
#ifdef __CONCURRENT__
        std::vector<std::unique_lock<std::shared_mutex>> vtLocks;
#endif // __CONCURRENT__

        ObjectUIDType uidLastNode, uidCurrentNode;  // TODO: make Optional!
        ObjectTypePtr ptrLastNode = nullptr, ptrCurrentNode = nullptr;

#ifdef __POSITION_AWARE_ITEMS__
        std::optional<ObjectUIDType> uidCurrentNodeParent, uidLastNodeParent;
        std::vector<std::pair<ObjectUIDType, std::pair<std::optional<ObjectUIDType>, ObjectTypePtr>>> vtNodes;
#else
        std::vector<std::pair<ObjectUIDType, ObjectTypePtr>> vtNodes;
#endif __POSITION_AWARE_ITEMS__

#ifdef __CONCURRENT__
        vtLocks.push_back(std::unique_lock<std::shared_mutex>(m_mutex));
#endif // __CONCURRENT__

        uidCurrentNode = m_uidRootNode.value();

        do
        {
#ifdef __POSITION_AWARE_ITEMS__
            uidCurrentNodeParent = uidLastNode;
            m_ptrCache->getObject(uidCurrentNode, ptrCurrentNode, uidCurrentNodeParent);    //TODO: lock


            if (ptrLastNode != nullptr && !uidCurrentNodeParent)
            {
                throw new std::exception("should not occur!");   // TODO: critical log.
            }
            if (ptrLastNode != nullptr && uidCurrentNodeParent != uidLastNode)
            {
                throw new std::exception("should not occur!");   // TODO: critical log.
            }

#else
            m_ptrCache->getObject(uidCurrentNode, ptrCurrentNode);    //TODO: lock
#endif __POSITION_AWARE_ITEMS__

#ifdef __CONCURRENT__
            vtLocks.push_back(std::unique_lock<std::shared_mutex>(ptrCurrentNode->mutex));
#endif __CONCURRENT__

            if (ptrCurrentNode == nullptr)
            {
                throw new std::exception("should not occur!");   // TODO: critical log.
            }

            if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*ptrCurrentNode->data))
            {
                std::shared_ptr<IndexNodeType> ptrIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*ptrCurrentNode->data);

                if (ptrIndexNode->canTriggerSplit(m_nDegree))
                {
#ifdef __POSITION_AWARE_ITEMS__
                    vtNodes.push_back(std::pair<ObjectUIDType, std::pair<std::optional<ObjectUIDType>, ObjectTypePtr>>(uidLastNode, std::pair<std::optional<ObjectUIDType>, ObjectTypePtr>(uidLastNodeParent, ptrLastNode)));
#else
                    vtNodes.push_back(std::pair<ObjectUIDType, ObjectTypePtr>(uidLastNode, ptrLastNode));
#endif __POSITION_AWARE_ITEMS__
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

#ifdef __POSITION_AWARE_ITEMS__
                uidLastNodeParent = uidCurrentNodeParent;
#endif __POSITION_AWARE_ITEMS__

                uidCurrentNode = ptrIndexNode->getChild(key); // fid it.. there are two kinds of methods..
            }
            else if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*ptrCurrentNode->data))
            {
                std::shared_ptr<DataNodeType> ptrDataNode = std::get<std::shared_ptr<DataNodeType>>(*ptrCurrentNode->data);

                ptrCurrentNode->dirty = true;
                ptrDataNode->insert(key, value);

                if (ptrDataNode->requireSplit(m_nDegree))
                {
#ifdef __POSITION_AWARE_ITEMS__
                    vtNodes.push_back(std::pair<ObjectUIDType, std::pair<std::optional<ObjectUIDType>, ObjectTypePtr>>(uidLastNode, std::pair<std::optional<ObjectUIDType>, ObjectTypePtr>(uidLastNodeParent, ptrLastNode)));
                    vtNodes.push_back(std::pair<ObjectUIDType, std::pair<std::optional<ObjectUIDType>, ObjectTypePtr>>(uidCurrentNode, std::pair<std::optional<ObjectUIDType>, ObjectTypePtr>(uidCurrentNodeParent, ptrCurrentNode)));
#else
                    vtNodes.push_back(std::pair<ObjectUIDType, ObjectTypePtr>(uidLastNode, ptrLastNode));
                    vtNodes.push_back(std::pair<ObjectUIDType, ObjectTypePtr>(uidCurrentNode, ptrCurrentNode));
#endif __POSITION_AWARE_ITEMS__
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
#ifdef __POSITION_AWARE_ITEMS__
            std::pair<ObjectUIDType, std::pair<std::optional<ObjectUIDType>, ObjectTypePtr>> prNodeDetails = vtNodes.back();
#else
            std::pair<ObjectUIDType, ObjectTypePtr> prNodeDetails = vtNodes.back();
#endif __POSITION_AWARE_ITEMS__

#ifdef __POSITION_AWARE_ITEMS__
            if (prNodeDetails.second.second == nullptr)
#else
            if (prNodeDetails.second == nullptr)
#endif __POSITION_AWARE_ITEMS__
            {
                if (vtNodes.size() != 1)
                {
                    throw new std::exception("should not occur!");
                }

#ifdef __POSITION_AWARE_ITEMS__
                std::optional<ObjectUIDType> uidParent = std::nullopt;
                m_ptrCache->template createObjectOfType<IndexNodeType>(m_uidRootNode, uidParent, pivotKey, uidLHSNode, *uidRHSNode);
                m_ptrCache->updateParentUID(uidLHSNode, m_uidRootNode);
                m_ptrCache->updateParentUID(*uidRHSNode, m_uidRootNode);
#else
                m_ptrCache->template createObjectOfType<IndexNodeType>(m_uidRootNode, pivotKey, uidLHSNode, * uidRHSNode); 
#endif __POSITION_AWARE_ITEMS__
                break;
            }

#ifdef __POSITION_AWARE_ITEMS__
            if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*prNodeDetails.second.second->data))
            {
                std::shared_ptr<IndexNodeType> ptrIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*prNodeDetails.second.second->data);
#else
            if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*prNodeDetails.second->data))
            {
                std::shared_ptr<IndexNodeType> ptrIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*prNodeDetails.second->data);
#endif __POSITION_AWARE_ITEMS__

                prNodeDetails.second.second->dirty = true;

                ptrIndexNode->insert(pivotKey, *uidRHSNode);

                uidRHSNode = std::nullopt;

                if (ptrIndexNode->requireSplit(m_nDegree))
                {
#ifdef __POSITION_AWARE_ITEMS__
                    ErrorCode errCode = ptrIndexNode->template split<std::shared_ptr<CacheType>>(m_ptrCache, uidRHSNode, prNodeDetails.second.first, pivotKey);
#else
                    ErrorCode errCode = ptrIndexNode->template split<std::shared_ptr<CacheType>>(m_ptrCache, uidRHSNode, pivotKey); 
#endif __POSITION_AWARE_ITEMS__

                    if (errCode != ErrorCode::Success)
                    {
                        throw new std::exception("should not occur!");
                    }
                }
            }
#ifdef __POSITION_AWARE_ITEMS__
            else if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*prNodeDetails.second.second->data))
            {
                std::shared_ptr<DataNodeType> ptrDataNode = std::get<std::shared_ptr<DataNodeType>>(*prNodeDetails.second.second->data);

                prNodeDetails.second.second->dirty = true;

                ErrorCode errCode = ptrDataNode->template split<std::shared_ptr<CacheType>, ObjectUIDType>(m_ptrCache, uidRHSNode, prNodeDetails.second.first, pivotKey);
#else
            else if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*prNodeDetails.second->data))
            {
                std::shared_ptr<DataNodeType> ptrDataNode = std::get<std::shared_ptr<DataNodeType>>(*prNodeDetails.second->data);

                ErrorCode errCode = ptrDataNode->template split<std::shared_ptr<CacheType>, ObjectUIDType>(m_ptrCache, uidRHSNode, pivotKey);
#endif __POSITION_AWARE_ITEMS__

                if (errCode != ErrorCode::Success)
                {
                    throw new std::exception("should not occur!");
                }                
            }

            uidLHSNode = prNodeDetails.first;

            vtNodes.pop_back();
        }

        return ErrorCode::Success;
    }

    ErrorCode search(const KeyType& key, ValueType& value)
    {
        ErrorCode errCode = ErrorCode::Error;
        
#ifdef __CONCURRENT__
        std::vector<std::shared_lock<std::shared_mutex>> vtLocks;
        vtLocks.push_back(std::shared_lock<std::shared_mutex>(m_mutex));
#endif __CONCURRENT__

#ifdef __POSITION_AWARE_ITEMS__
        std::optional<ObjectUIDType> uidCurrentNodeParent, uidLastNode;
#endif __POSITION_AWARE_ITEMS__

        ObjectUIDType uidCurrentNode = m_uidRootNode.value();
        do
        {
            ObjectTypePtr prNodeDetails = nullptr;

#ifdef __POSITION_AWARE_ITEMS__
            uidCurrentNodeParent = uidLastNode;
            m_ptrCache->getObject(uidCurrentNode, prNodeDetails, uidCurrentNodeParent);    //TODO: lock

            if (uidLastNode != uidCurrentNodeParent)
            {
                throw new std::exception("should not occur!");   // TODO: critical log.
            }
#else __POSITION_AWARE_ITEMS__
            m_ptrCache->getObject(uidCurrentNode, prNodeDetails);    //TODO: lock
#endif __POSITION_AWARE_ITEMS__

#ifdef __CONCURRENT__
            vtLocks.push_back(std::shared_lock<std::shared_mutex>(prNodeDetails->mutex));
            vtLocks.erase(vtLocks.begin());
#endif __CONCURRENT__

            if (prNodeDetails == nullptr)
            {
                throw new std::exception("should not occur!");
            }

            if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*prNodeDetails->data))
            {
                std::shared_ptr<IndexNodeType> ptrIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*prNodeDetails->data);

                uidLastNode = uidCurrentNode;

                uidCurrentNode = ptrIndexNode->getChild(key);
            }
            else if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*prNodeDetails->data))
            {
                std::shared_ptr<DataNodeType> ptrDataNode = std::get<std::shared_ptr<DataNodeType>>(*prNodeDetails->data);

                errCode = ptrDataNode->getValue(key, value);

                break;
            }

            
        } while (true);
        
        return errCode;
    }

    ErrorCode remove(const KeyType& key)
    {
        
#ifdef __CONCURRENT__
        std::vector<std::unique_lock<std::shared_mutex>> vtLocks;
#endif __CONCURRENT__

        ObjectUIDType uidLastNode, uidCurrentNode;
        ObjectTypePtr ptrLastNode = nullptr, ptrCurrentNode = nullptr;

#ifdef __POSITION_AWARE_ITEMS__
        std::optional<ObjectUIDType> uidCurrentNodeParent, uidLastNodeParent;
        std::vector<std::pair<ObjectUIDType, std::pair<std::optional<ObjectUIDType>, ObjectTypePtr>>> vtNodes;
#else
        std::vector<std::pair<ObjectUIDType, ObjectTypePtr>> vtNodes;
#endif __POSITION_AWARE_ITEMS__

#ifdef __CONCURRENT__
        vtLocks.push_back(std::unique_lock<std::shared_mutex>(m_mutex));
#endif __CONCURRENT__

        uidCurrentNode = m_uidRootNode.value();

        do
        {
#ifdef __POSITION_AWARE_ITEMS__
            uidCurrentNodeParent = uidLastNode;
            m_ptrCache->getObject(uidCurrentNode, ptrCurrentNode, uidCurrentNodeParent);    //TODO: lock


            if (ptrLastNode != nullptr && !uidCurrentNodeParent)
            {
                uidCurrentNodeParent = uidLastNode;
                //throw new std::exception("should not occur!");   // TODO: critical log.
            }
            if (ptrLastNode != nullptr && uidCurrentNodeParent != uidLastNode)
            {
                throw new std::exception("should not occur!");   // TODO: critical log.
            }
#else
            m_ptrCache->getObject(uidCurrentNode, ptrCurrentNode);    //TODO: lock
#endif __POSITION_AWARE_ITEMS__
            
#ifdef __CONCURRENT__
            vtLocks.push_back(std::unique_lock<std::shared_mutex>(ptrCurrentNode->mutex));
#endif __CONCURRENT__

            if (ptrCurrentNode == nullptr)
            {
                throw new std::exception("should not occur!");
            }

            if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*ptrCurrentNode->data))
            {
                std::shared_ptr<IndexNodeType> ptrIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*ptrCurrentNode->data);

                if (ptrIndexNode->canTriggerMerge(m_nDegree))
                {
#ifdef __POSITION_AWARE_ITEMS__
                    vtNodes.push_back(std::pair<ObjectUIDType, std::pair<std::optional<ObjectUIDType>, ObjectTypePtr>>(uidLastNode, std::pair<std::optional<ObjectUIDType>, ObjectTypePtr>(uidLastNodeParent, ptrLastNode)));
#else
                    vtNodes.push_back(std::pair<ObjectUIDType, ObjectTypePtr>(uidLastNode, ptrLastNode));
#endif __POSITION_AWARE_ITEMS__
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

#ifdef __POSITION_AWARE_ITEMS__
                uidLastNodeParent = uidCurrentNodeParent;
#endif __POSITION_AWARE_ITEMS__

                uidCurrentNode = ptrIndexNode->getChild(key); // fid it.. there are two kinds of methods..
            }
            else if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*ptrCurrentNode->data))
            {
                std::shared_ptr<DataNodeType> ptrDataNode = std::get<std::shared_ptr<DataNodeType>>(*ptrCurrentNode->data);

                ptrCurrentNode->dirty = true;
                if (ptrDataNode->remove(key) == ErrorCode::KeyDoesNotExist)
                {
                    throw new std::exception("should not occur!");
                }

                ptrDataNode->remove(key);

                if (ptrDataNode->requireMerge(m_nDegree))
                {
#ifdef __POSITION_AWARE_ITEMS__
                    vtNodes.push_back(std::pair<ObjectUIDType, std::pair<std::optional<ObjectUIDType>, ObjectTypePtr>>(uidLastNode, std::pair<std::optional<ObjectUIDType>, ObjectTypePtr>(uidLastNodeParent, ptrLastNode)));
                    vtNodes.push_back(std::pair<ObjectUIDType, std::pair<std::optional<ObjectUIDType>, ObjectTypePtr>>(uidCurrentNode, std::pair<std::optional<ObjectUIDType>, ObjectTypePtr>(uidCurrentNodeParent, ptrCurrentNode)));
#else
                    vtNodes.push_back(std::pair<ObjectUIDType, ObjectTypePtr>(uidLastNode, ptrLastNode));
                    vtNodes.push_back(std::pair<ObjectUIDType, ObjectTypePtr>(uidCurrentNode, ptrCurrentNode));
#endif __POSITION_AWARE_ITEMS__
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

        ObjectUIDType ckChildNode;
        ObjectTypePtr ptrChildNode = nullptr;

        while (vtNodes.size() > 0)
        {
#ifdef __POSITION_AWARE_ITEMS__
            std::pair<ObjectUIDType, std::pair<std::optional<ObjectUIDType>, ObjectTypePtr>> prNodeDetails = vtNodes.back();
#else
            std::pair<ObjectUIDType, ObjectTypePtr> prNodeDetails = vtNodes.back();
#endif __POSITION_AWARE_ITEMS__

#ifdef __POSITION_AWARE_ITEMS__
            if (prNodeDetails.second.second == nullptr)
#else
            if (prNodeDetails.second == nullptr)
#endif __POSITION_AWARE_ITEMS__
            {
                if (vtNodes.size() != 1)
                {
                    throw new std::exception("should not occur!");
                }

                if (*m_uidRootNode != ckChildNode)
                {
                    throw new std::exception("should not occur!");
                }

                ObjectTypePtr ptrCurrentRoot;
                m_ptrCache->getObject(*m_uidRootNode, ptrCurrentRoot);
                ptrCurrentRoot->dirty = true;
                if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*ptrCurrentRoot->data))
                {
                    std::shared_ptr<IndexNodeType> ptrInnerNode = std::get<std::shared_ptr<IndexNodeType>>(*ptrCurrentRoot->data);
                    if (ptrInnerNode->getKeysCount() == 0) {
                        ObjectUIDType _tmp = ptrInnerNode->getChildAt(0);
                        m_ptrCache->remove(*m_uidRootNode);
                        m_uidRootNode = _tmp;

                        std::optional< ObjectUIDType> null_pk;
                        m_ptrCache->updateParentUID(*m_uidRootNode, null_pk);
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


                //if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*ptrChildNode->data))
                //{
                //    std::shared_ptr<IndexNodeType> ptrChildIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*ptrChildNode->data);

                //    if (ptrChildIndexNode->getKeysCount() == 0)
                //    {
                //        m_uidRootNode = ptrChildIndexNode->getChildAt(0);
                //    }

                //}
                //else //if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*ptrChildNode->data))
                //{
                //    m_uidRootNode = ckChildNode;
                //}

                break;
            }

#ifdef __POSITION_AWARE_ITEMS__
            if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*prNodeDetails.second.second->data))
            {
                std::shared_ptr<IndexNodeType> ptrParentIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*prNodeDetails.second.second->data);
#else
            if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*prNodeDetails.second->data))
            {
                std::shared_ptr<IndexNodeType> ptrParentIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*prNodeDetails.second->data);
#endif __POSITION_AWARE_ITEMS__

                if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*ptrChildNode->data))
                {
                    std::optional<ObjectUIDType> uidToDelete = std::nullopt;

                    std::shared_ptr<IndexNodeType> ptrChildIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*ptrChildNode->data);

                    if (ptrChildIndexNode->requireMerge(m_nDegree))
                    {
#ifdef __POSITION_AWARE_ITEMS__
                        ptrChildNode->dirty = true;
                        prNodeDetails.second.second->dirty = true;
                        std::optional<ObjectUIDType> __k = ckChildNode;// prNodeDetails.first;
                        ptrParentIndexNode->template rebalanceIndexNode<std::shared_ptr<CacheType>, shared_ptr<IndexNodeType>>(m_ptrCache, ptrChildIndexNode, key, m_nDegree, ckChildNode, uidToDelete, __k);
#else
                        ptrParentIndexNode->template rebalanceIndexNode<std::shared_ptr<CacheType>, shared_ptr<IndexNodeType>>(m_ptrCache, ptrChildIndexNode, key, m_nDegree, ckChildNode, uidToDelete);
#endif __POSITION_AWARE_ITEMS__


                        if (uidToDelete)
                        {
#ifdef __CONCURRENT__
                            if (*uidToDelete == ckChildNode)
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

                        //if (ptrParentIndexNode->getKeysCount() == 0)
                        //{
                        //    m_uidRootNode = ptrParentIndexNode->getChildAt(0);
                        //    //throw new exception("should not occur!");
                        //}
                    }
                }
                else if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*ptrChildNode->data))
                {
                    std::optional<ObjectUIDType> uidToDelete = std::nullopt;

                    std::shared_ptr<DataNodeType> ptrChildDataNode = std::get<std::shared_ptr<DataNodeType>>(*ptrChildNode->data);

                    ptrChildNode->dirty = true;
                    prNodeDetails.second.second->dirty = true;

                    ptrParentIndexNode->template rebalanceDataNode<std::shared_ptr<CacheType>, shared_ptr<DataNodeType>>(m_ptrCache, ptrChildDataNode, key, m_nDegree, ckChildNode, uidToDelete);

                    if (uidToDelete)
                    {
#ifdef __CONCURRENT__
                        if (*uidToDelete == ckChildNode)
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

                    //if (ptrParentIndexNode->getKeysCount() == 0)
                    //{
                    //    m_uidRootNode = ptrParentIndexNode->getChildAt(0);
                    //    //throw new exception("should not occur!");
                    //}
                }
            }

            ckChildNode = prNodeDetails.first;
#ifdef __POSITION_AWARE_ITEMS__
            ptrChildNode = prNodeDetails.second.second;
#else
            ptrChildNode = prNodeDetails.second; 
#endif __POSITION_AWARE_ITEMS__
            vtNodes.pop_back();
        }
        
        return ErrorCode::Success;
    }

    void print(std::ofstream & out)
    {
        int nSpace = 7;

        std::string prefix;

        out << prefix << "|" << std::endl;
        out << prefix << "|" << std::string(nSpace, '-').c_str() << "(root)";

#ifdef __POSITION_AWARE_ITEMS__
        out << " P[], S[" << (*m_uidRootNode).toString().c_str() << "]";
#endif __POSITION_AWARE_ITEMS__

        out << std::endl;

        ObjectTypePtr ptrRootNode = nullptr;
        m_ptrCache->getObject(m_uidRootNode.value(), ptrRootNode);

        if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*ptrRootNode->data))
        {
            std::shared_ptr<IndexNodeType> ptrIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*ptrRootNode->data);

#ifdef __POSITION_AWARE_ITEMS__
            ptrIndexNode->template print<std::shared_ptr<CacheType>, ObjectTypePtr, DataNodeType>(out, m_ptrCache, 0, prefix, m_uidRootNode);
#else //__POSITION_AWARE_ITEMS__
            ptrIndexNode->template print<std::shared_ptr<CacheType>, ObjectTypePtr, DataNodeType>(out, m_ptrCache, 0, prefix);
#endif __POSITION_AWARE_ITEMS__
        }
        else if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*ptrRootNode->data))
        {
            std::shared_ptr<DataNodeType> ptrDataNode = std::get<std::shared_ptr<DataNodeType>>(*ptrRootNode->data);

            ptrDataNode->print(out, 0, prefix);
        }
    }

    void getstate(size_t& lru, size_t& map)
    {
        return m_ptrCache->getstate(lru, map);
    }

#ifdef __POSITION_AWARE_ITEMS__
public:
    CacheErrorCode updateChildUID(std::optional<ObjectUIDType>& uidObject, ObjectUIDType uidChildOld, ObjectUIDType uidChildNew)
    {
        if (!uidObject)
        {   
            // Update for Root... should not occur.. this case is covered in manual flush.
            throw new std::exception("should not occur!");
        }

        std::optional< ObjectUIDType> t;
        ObjectTypePtr ptrObject = nullptr;
        m_ptrCache->getObject(*uidObject, ptrObject, t, true);

        if (ptrObject == nullptr)
        {
            throw new std::exception("should not occur!");   // TODO: critical log.
        }

        if (!ptrObject->dirty)
        {
            ptrObject->dirty = true;
            //throw new std::exception("should not occur!");   // TODO: critical log.
        }

        if (std::holds_alternative<std::shared_ptr<IndexNodeType>>(*ptrObject->data))
        {
            std::shared_ptr<IndexNodeType> ptrIndexNode = std::get<std::shared_ptr<IndexNodeType>>(*ptrObject->data);

            ptrIndexNode->updateChildUID(uidChildOld, uidChildNew);
        }
        else //if (std::holds_alternative<std::shared_ptr<DataNodeType>>(*ptrObject->data))
        {
            throw new std::exception("should not occur!");   // TODO: critical log.
        }

        return CacheErrorCode::Success;
    }

    CacheErrorCode updateChildUID(std::vector<std::pair<ObjectUIDType, std::pair<ObjectUIDType, ObjectUIDType>>> vtUpdatedUIDs)
    {
        return CacheErrorCode::Success;
    }
#endif __POSITION_AWARE_ITEMS__
};