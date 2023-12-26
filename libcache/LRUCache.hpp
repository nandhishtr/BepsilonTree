#pragma once
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <syncstream>
#include <thread>
#include <variant>
#include <typeinfo>
#include <unordered_map>
#include <queue>
#include  <algorithm>
#include <tuple>

#include "ErrorCodes.h"
#include "IFlushCallback.h"
#include "VariadicNthType.h"

//#define __CONCURRENT__
#define __POSITION_AWARE_ITEMS__

#ifdef __POSITION_AWARE_ITEMS__
template <typename ICallback, typename StorageType>
class LRUCache : public ICallback
#else //__POSITION_AWARE_ITEMS__
template <typename StorageType>
class LRUCache
#endif __POSITION_AWARE_ITEMS__
{

#ifdef __POSITION_AWARE_ITEMS__
	typedef LRUCache<ICallback, StorageType> SelfType;
#else //__POSITION_AWARE_ITEMS__
	typedef LRUCache<StorageType> SelfType;
#endif __POSITION_AWARE_ITEMS__

public:
	typedef StorageType::ObjectUIDType ObjectUIDType;
	typedef StorageType::ObjectType ObjectType;
	typedef std::shared_ptr<ObjectType> ObjectTypePtr;

private:
	struct Item
	{
	public:
#ifdef __POSITION_AWARE_ITEMS__
		std::optional<ObjectUIDType> m_uidParent;
#endif __POSITION_AWARE_ITEMS__

		ObjectUIDType m_uidSelf;
		ObjectTypePtr m_ptrObject;
		std::shared_ptr<Item> m_ptrPrev;
		std::shared_ptr<Item> m_ptrNext;

#ifdef __POSITION_AWARE_ITEMS__
		Item(ObjectUIDType& key, ObjectTypePtr ptrObject, std::optional<ObjectUIDType>& uidParent)
			: m_ptrNext(nullptr)
			, m_ptrPrev(nullptr)
		{
			m_uidSelf = key;
			m_ptrObject = ptrObject;

			m_uidParent = uidParent;
		}
#endif __POSITION_AWARE_ITEMS__

		Item(ObjectUIDType& key, ObjectTypePtr ptrObject)
			: m_ptrNext(nullptr)
			, m_ptrPrev(nullptr)
		{
			m_uidSelf = key;
			m_ptrObject = ptrObject;
		}

		~Item()
		{
			m_ptrPrev = nullptr;
			m_ptrNext = nullptr;
			m_ptrObject = nullptr;
		}
	};

#ifdef __POSITION_AWARE_ITEMS__
	ICallback* m_ptrCallback;
#endif __POSITION_AWARE_ITEMS__

	std::shared_ptr<Item> m_ptrHead;
	std::shared_ptr<Item> m_ptrTail;

	std::unique_ptr<StorageType> m_ptrStorage;

	size_t m_nCacheCapacity;
	std::unordered_map<ObjectUIDType, std::shared_ptr<Item>> m_ptrObjects;

#ifdef __CONCURRENT__
	bool m_bStop;

	std::thread m_threadCacheFlush;

	mutable std::shared_mutex m_mtxCache;
	mutable std::shared_mutex m_mtxStorage;
#endif __CONCURRENT__

public:
	~LRUCache()
	{
#ifdef __CONCURRENT__
		m_bStop = true;
		m_threadCacheFlush.join();
#endif __CONCURRENT__

		m_ptrHead = nullptr;
		m_ptrTail = nullptr;
		m_ptrStorage = nullptr;

		m_ptrObjects.clear();
	}

	template <typename... StorageArgs>
	LRUCache(size_t nCapacity, StorageArgs... args)
		: m_nCacheCapacity(nCapacity)
		, m_ptrHead(nullptr)
		, m_ptrTail(nullptr)
	{
		m_ptrStorage = std::make_unique<StorageType>(args...);

#ifdef __CONCURRENT__
		m_bStop = false;
		m_threadCacheFlush = std::thread(handlerCacheFlush, this);
#endif __CONCURRENT__
	}

	template <typename... InitArgs>
#ifdef __POSITION_AWARE_ITEMS__
	CacheErrorCode init(ICallback* ptrCallback, InitArgs... args)
#else __POSITION_AWARE_ITEMS__
	CacheErrorCode init(InitArgs... args)
#endif  __POSITION_AWARE_ITEMS__
	{



#ifdef __POSITION_AWARE_ITEMS__

#ifdef __CONCURRENT__
		m_ptrCallback = ptrCallback;
		return m_ptrStorage->init(this/*getNthElement<0>(args...)*/);
#else // ! __CONCURRENT__
		return m_ptrStorage->init(ptrCallback/*getNthElement<0>(args...)*/);
#endif __CONCURRENT__

#else //__POSITION_AWARE_ITEMS__
		//return m_ptrStorage->template init<InitArgs>(args...);
		return m_ptrStorage->init();
#endif __POSITION_AWARE_ITEMS__
	}

	CacheErrorCode remove(ObjectUIDType uidObject)
	{
#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex>  lock_cache(m_mtxCache);
#endif __CONCURRENT__

		auto it = m_ptrObjects.find(uidObject);
		if (it != m_ptrObjects.end()) 
		{
			removeFromLRU((*it).second);
			m_ptrObjects.erase((*it).first);
			return CacheErrorCode::Success;
		}

		CacheErrorCode errCode = m_ptrStorage->remove(uidObject);
		if (errCode != CacheErrorCode::Success)
		{
			throw new std::exception("should not occur!");
		}

		return CacheErrorCode::Success;
		//return CacheErrorCode::KeyDoesNotExist;
	}

#ifdef __POSITION_AWARE_ITEMS__
	CacheErrorCode getObject(ObjectUIDType uidObject, ObjectTypePtr& ptrObject, std::optional<ObjectUIDType>& keyParent)
	{
#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex> lock_cache(m_mtxCache); // std::unique_lock due to LRU's linked-list update! is there any better way?
#endif __CONCURRENT__

		if (m_ptrObjects.find(uidObject) != m_ptrObjects.end())
		{
			std::shared_ptr<Item> ptrItem = m_ptrObjects[uidObject];
			moveToFront(ptrItem);
			ptrObject = ptrItem->m_ptrObject;
			keyParent = ptrItem->m_uidParent;
			return CacheErrorCode::Success;
		}

#ifdef __CONCURRENT__
		std::shared_lock<std::shared_mutex> lock_storage(m_mtxStorage); // TODO: requesting the same key?
		lock_cache.unlock();
#endif __CONCURRENT__

		std::shared_ptr<ObjectType> _ptrObject = m_ptrStorage->getObject(uidObject);

#ifdef __CONCURRENT__
		lock_storage.unlock();
#endif __CONCURRENT__

		if (_ptrObject != nullptr)
		{
			std::shared_ptr<Item> ptrItem = std::make_shared<Item>(uidObject, _ptrObject, keyParent);

#ifdef __CONCURRENT__
			std::unique_lock<std::shared_mutex> re_lock_cache(m_mtxCache);
#endif __CONCURRENT__

			if (m_ptrObjects.find(uidObject) != m_ptrObjects.end())
			{
				std::shared_ptr<Item> ptrItem = m_ptrObjects[uidObject];
				moveToFront(ptrItem);
				ptrObject = ptrItem->m_ptrObject;
				keyParent = ptrItem->m_uidParent;
				return CacheErrorCode::Success;
			}

			m_ptrObjects[uidObject] = ptrItem;

			if (!m_ptrHead) 
			{
				m_ptrHead = ptrItem;
				m_ptrTail = ptrItem;
			}
			else 
			{
				ptrItem->m_ptrNext = m_ptrHead;
				m_ptrHead->m_ptrPrev = ptrItem;
				m_ptrHead = ptrItem;
			}

			ptrObject = _ptrObject;

#ifndef __CONCURRENT__
			flushItemsToStorage();
#endif __CONCURRENT__

			return CacheErrorCode::Success;
		}
		
		return CacheErrorCode::Error;
	}
#endif __POSITION_AWARE_ITEMS__

	CacheErrorCode getObject(ObjectUIDType uidObject, ObjectTypePtr & ptrObject)
	{
#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex> lock_cache(m_mtxCache); // std::unique_lock due to LRU's linked-list update! is there any better way?
#endif __CONCURRENT__

		if (m_ptrObjects.find(uidObject) != m_ptrObjects.end())
		{
			std::shared_ptr<Item> ptrItem = m_ptrObjects[uidObject];
			moveToFront(ptrItem);
			ptrObject = ptrItem->m_ptrObject;
			return CacheErrorCode::Success;
		}

#ifdef __CONCURRENT__
		std::shared_lock<std::shared_mutex> lock_storage(m_mtxStorage); // TODO: requesting the same key?
		lock_cache.unlock();
#endif __CONCURRENT__

		std::shared_ptr<ObjectType> _ptrObject = m_ptrStorage->getObject(uidObject);

#ifdef __CONCURRENT__
		lock_storage.unlock();
#endif __CONCURRENT__

		if (_ptrObject != nullptr)
		{
			std::shared_ptr<Item> ptrItem = std::make_shared<Item>(uidObject, _ptrObject);

#ifdef __CONCURRENT__
			std::unique_lock<std::shared_mutex> re_lock_cache(m_mtxCache);
#endif __CONCURRENT__

			if (m_ptrObjects.find(uidObject) != m_ptrObjects.end())
			{
				std::shared_ptr<Item> ptrItem = m_ptrObjects[uidObject];
				moveToFront(ptrItem);
				ptrObject = ptrItem->m_ptrObject;
				return CacheErrorCode::Success;
			}

			m_ptrObjects[uidObject] = ptrItem;

			if (!m_ptrHead)
			{
				m_ptrHead = ptrItem;
				m_ptrTail = ptrItem;
			}
			else
			{
				ptrItem->m_ptrNext = m_ptrHead;
				m_ptrHead->m_ptrPrev = ptrItem;
				m_ptrHead = ptrItem;
			}

			ptrObject = _ptrObject;

#ifndef __CONCURRENT__
			flushItemsToStorage();
#endif __CONCURRENT__

			return CacheErrorCode::Success;
		}

		return CacheErrorCode::Error;
	}

#ifdef __POSITION_AWARE_ITEMS__
	CacheErrorCode getObjectFromCacheOnly(ObjectUIDType uidObject, ObjectTypePtr& ptrObject, std::optional<ObjectUIDType>& uidParent)
	{
#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex> lock_cache(m_mtxCache); // std::unique_lock due to LRU's linked-list update! is there any better way?
#endif __CONCURRENT__

		if (m_ptrObjects.find(uidObject) != m_ptrObjects.end())
		{
			std::shared_ptr<Item> ptrItem = m_ptrObjects[uidObject];
			//moveToFront(ptrItem);
			ptrObject = ptrItem->m_ptrObject;
			uidParent = ptrItem->m_uidParent;
			return CacheErrorCode::Success;
		}

		return CacheErrorCode::Error;
	}
#endif __POSITION_AWARE_ITEMS__

#ifdef __POSITION_AWARE_ITEMS__
	template <typename Type>
	CacheErrorCode getObjectOfType(ObjectUIDType key, Type& ptrObject, std::optional<ObjectUIDType>& keyParent)
	{
#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex> lock_cache(m_mtxCache);
#endif __CONCURRENT__

		if (m_ptrObjects.find(key) != m_ptrObjects.end())
		{
			std::shared_ptr<Item> ptrItem = m_ptrObjects[key];
			moveToFront(ptrItem);

//#ifdef __CONCURRENT__
//			lock_cache.unlock();
//#endif __CONCURRENT__

			if (std::holds_alternative<Type>(*ptrItem->m_ptrObject->data))
			{
				ptrObject = std::get<Type>(*ptrItem->m_ptrObject->data);
				keyParent = ptrItem->m_uidParent;
				return CacheErrorCode::Success;
			}

			return CacheErrorCode::Error;
		}

#ifdef __CONCURRENT__
		std::shared_lock<std::shared_mutex> lock_storage(m_mtxStorage);
		lock_cache.unlock();
#endif __CONCURRENT__

		std::shared_ptr<ObjectType> ptrValue = m_ptrStorage->getObject(key);

#ifdef __CONCURRENT__
		lock_storage.unlock();
#endif __CONCURRENT__

		if (ptrValue != nullptr)
		{
			std::shared_ptr<Item> ptrItem = std::make_shared<Item>(key, ptrValue, keyParent);

#ifdef __CONCURRENT__
			std::unique_lock<std::shared_mutex> re_lock_cache(m_mtxCache);
#endif __CONCURRENT__


			if (m_ptrObjects.find(key) != m_ptrObjects.end())
			{
				std::shared_ptr<Item> ptrItem = m_ptrObjects[key];
				moveToFront(ptrItem);

				if (std::holds_alternative<Type>(*ptrItem->m_ptrObject->data))
				{
					ptrObject = std::get<Type>(*ptrItem->m_ptrObject->data);
					keyParent = ptrItem->m_uidParent;
					return CacheErrorCode::Success;
				}

				return CacheErrorCode::Error;
			}

			m_ptrObjects[key] = ptrItem;

			if (!m_ptrHead)
			{
				m_ptrHead = ptrItem;
				m_ptrTail = ptrItem;
			}
			else
			{
				ptrItem->m_ptrNext = m_ptrHead;
				m_ptrHead->m_ptrPrev = ptrItem;
				m_ptrHead = ptrItem;
			}

//#ifdef __CONCURRENT__
//			lock_cache.unlock();
//#endif __CONCURRENT__

			if (std::holds_alternative<Type>(*ptrValue->data))
			{
				ptrObject = std::get<Type>(*ptrValue->data);
				return CacheErrorCode::Success;
			}

#ifndef __CONCURRENT__
			flushItemsToStorage();
#endif __CONCURRENT__

			return CacheErrorCode::Error;
		}

		return CacheErrorCode::Error;
	}
#endif __POSITION_AWARE_ITEMS__

	template <typename Type>
	CacheErrorCode getObjectOfType(ObjectUIDType key, Type& ptrObject)
	{
#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex> lock_cache(m_mtxCache);
#endif __CONCURRENT__

		if (m_ptrObjects.find(key) != m_ptrObjects.end())
		{
			std::shared_ptr<Item> ptrItem = m_ptrObjects[key];
			moveToFront(ptrItem);

			//#ifdef __CONCURRENT__
			//			lock_cache.unlock();
			//#endif __CONCURRENT__

			if (std::holds_alternative<Type>(*ptrItem->m_ptrObject->data))
			{
				ptrObject = std::get<Type>(*ptrItem->m_ptrObject->data);
				return CacheErrorCode::Success;
			}

			return CacheErrorCode::Error;
		}

#ifdef __CONCURRENT__
		std::shared_lock<std::shared_mutex> lock_storage(m_mtxStorage);
		lock_cache.unlock();
#endif __CONCURRENT__

		std::shared_ptr<ObjectType> ptrValue = m_ptrStorage->getObject(key);

#ifdef __CONCURRENT__
		lock_storage.unlock();
#endif __CONCURRENT__

		if (ptrValue != nullptr)
		{
			std::shared_ptr<Item> ptrItem = std::make_shared<Item>(key, ptrValue);

#ifdef __CONCURRENT__
			std::unique_lock<std::shared_mutex> re_lock_cache(m_mtxCache);
#endif __CONCURRENT__

			if (m_ptrObjects.find(key) != m_ptrObjects.end())
			{
				std::shared_ptr<Item> ptrItem = m_ptrObjects[key];
				moveToFront(ptrItem);

				if (std::holds_alternative<Type>(*ptrItem->m_ptrObject->data))
				{
					ptrObject = std::get<Type>(*ptrItem->m_ptrObject->data);
					return CacheErrorCode::Success;
				}

				return CacheErrorCode::Error;
			}

			m_ptrObjects[key] = ptrItem;

			if (!m_ptrHead)
			{
				m_ptrHead = ptrItem;
				m_ptrTail = ptrItem;
			}
			else
			{
				ptrItem->m_ptrNext = m_ptrHead;
				m_ptrHead->m_ptrPrev = ptrItem;
				m_ptrHead = ptrItem;
			}

			//#ifdef __CONCURRENT__
			//			lock_cache.unlock();
			//#endif __CONCURRENT__

			if (std::holds_alternative<Type>(*ptrValue->data))
			{
				ptrObject = std::get<Type>(*ptrValue->data);
				return CacheErrorCode::Success;
			}

#ifndef __CONCURRENT__
			flushItemsToStorage();
#endif __CONCURRENT__

			return CacheErrorCode::Error;
		}

		return CacheErrorCode::Error;
	}

	template<class Type, typename... ArgsType>
#ifdef __POSITION_AWARE_ITEMS__
	CacheErrorCode createObjectOfType(std::optional<ObjectUIDType>& uidObject, std::optional<ObjectUIDType>& uidParent, ArgsType... args)
#else
	CacheErrorCode createObjectOfType(std::optional<ObjectUIDType>& uidObject, ArgsType... args)
#endif __POSITION_AWARE_ITEMS__
	{
		std::shared_ptr<ObjectType> ptrValue = ObjectType::template createObjectOfType<Type>(args...);

		uidObject = ObjectUIDType::createAddressFromVolatilePointer(reinterpret_cast<uintptr_t>(ptrValue.get()));

#ifdef __POSITION_AWARE_ITEMS__
		std::shared_ptr<Item> ptrItem = std::make_shared<Item>(*uidObject, ptrValue, uidParent);
#else
		std::shared_ptr<Item> ptrItem = std::make_shared<Item>(*uidObject, ptrValue);
#endif __POSITION_AWARE_ITEMS__

#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex> lock_cache(m_mtxCache);
#endif __CONCURRENT__

		if (m_ptrObjects.find(*uidObject) != m_ptrObjects.end())
		{
			std::shared_ptr<Item> ptrItem = m_ptrObjects[*uidObject];
			ptrItem->m_ptrObject = ptrValue;
			moveToFront(ptrItem);
		}
		else
		{
			m_ptrObjects[*uidObject] = ptrItem;
			if (!m_ptrHead) 
			{
				m_ptrHead = ptrItem;
				m_ptrTail = ptrItem;
			}
			else 
			{
				ptrItem->m_ptrNext = m_ptrHead;
				m_ptrHead->m_ptrPrev = ptrItem;
				m_ptrHead = ptrItem;
			}
		}

#ifdef __CONCURRENT__
		//..
#else
		flushItemsToStorage();
#endif __CONCURRENT__

		return CacheErrorCode::Success;
	}

	template<class Type, typename... ArgsType>
#ifdef __POSITION_AWARE_ITEMS__
	CacheErrorCode createObjectOfType(std::optional<ObjectUIDType>& uidObject, std::optional<ObjectUIDType>& uidParent, std::shared_ptr<Type>& ptrCoreObject, ArgsType... args)
#else
	CacheErrorCode createObjectOfType(std::optional<ObjectUIDType>& uidObject, std::shared_ptr<Type>& ptrCoreObject, ArgsType... args)
#endif __POSITION_AWARE_ITEMS__
	{
		std::shared_ptr<ObjectType> ptrObject = ObjectType::template createObjectOfType<Type>(ptrCoreObject, args...);

		uidObject = ObjectUIDType::createAddressFromVolatilePointer(reinterpret_cast<uintptr_t>(ptrObject.get()));

#ifdef __POSITION_AWARE_ITEMS__
		std::shared_ptr<Item> ptrItem = std::make_shared<Item>(*uidObject, ptrObject, uidParent);
#else
		std::shared_ptr<Item> ptrItem = std::make_shared<Item>(*uidObject, ptrObject);
#endif __POSITION_AWARE_ITEMS__

#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex> lock_cache(m_mtxCache);
#endif __CONCURRENT__

		if (m_ptrObjects.find(*uidObject) != m_ptrObjects.end())
		{
			std::shared_ptr<Item> ptrItem = m_ptrObjects[*uidObject];
			ptrItem->m_ptrObject = ptrObject;
			moveToFront(ptrItem);
		}
		else
		{
			m_ptrObjects[*uidObject] = ptrItem;
			if (!m_ptrHead)
			{
				m_ptrHead = ptrItem;
				m_ptrTail = ptrItem;
			}
			else
			{
				ptrItem->m_ptrNext = m_ptrHead;
				m_ptrHead->m_ptrPrev = ptrItem;
				m_ptrHead = ptrItem;
			}
		}

#ifdef __CONCURRENT__
		//..
#else
		flushItemsToStorage();
#endif __CONCURRENT__

		return CacheErrorCode::Success;
	}

	CacheErrorCode updateParentUID(ObjectUIDType& uidChild, std::optional<ObjectUIDType>& uidParent)
	{
		if (m_ptrObjects.find(uidChild) != m_ptrObjects.end())
		{
			m_ptrObjects[uidChild]->m_uidParent = uidParent;
			return CacheErrorCode::Success;
		}
		return CacheErrorCode::Error;
	}

	void getstate(size_t& lru, size_t& map)
	{
		lru = 0;
		std::shared_ptr<Item> _ptrItem = m_ptrHead;
		do
		{
			lru++;
			_ptrItem = _ptrItem->m_ptrNext;

		} while (_ptrItem != nullptr);

		map = m_ptrObjects.size();
	}

private:
	void moveToTail(std::shared_ptr<Item> tail, std::shared_ptr<Item> nodeToMove) {
		if (tail == nullptr || nodeToMove == nullptr)
			return; // Invalid input

		// Detach the node from its current position
		if (nodeToMove->m_ptrPrev != nullptr)
			nodeToMove->m_ptrPrev->m_ptrNext = nodeToMove->m_ptrNext;
		else
			tail = nodeToMove->m_ptrNext;

		if (nodeToMove->m_ptrNext != nullptr)
			nodeToMove->m_ptrNext->m_ptrPrev = nodeToMove->m_ptrPrev;

		// Attach the node to the tail
		if (tail != nullptr) {
			tail->m_ptrNext = nodeToMove;
			nodeToMove->m_ptrPrev = tail;
			nodeToMove->m_ptrNext = nullptr;
			tail = nodeToMove;
		}
		else {
			// If the list is empty, set both head and tail to the node
			tail = nodeToMove;
		}
	}

	void interchangeWithTail(std::shared_ptr<Item> currentNode) {
		if (currentNode == nullptr || currentNode == m_ptrTail) {
			// Node is already the tail or is null
			return;
		}

		// Adjust prev and next pointers of adjacent nodes
		if (currentNode->m_ptrPrev) {
			currentNode->m_ptrPrev->m_ptrNext = currentNode->m_ptrNext;
		}
		else {
			// If currentNode is the head, update head
			m_ptrHead = currentNode->m_ptrNext;
		}

		if (currentNode->m_ptrNext) {
			currentNode->m_ptrNext->m_ptrPrev = currentNode->m_ptrPrev;
		}

		// Update prev and next pointers of the currentNode
		currentNode->m_ptrPrev = m_ptrTail;
		currentNode->m_ptrNext = nullptr;

		// Update tail's next pointer to the currentNode
		m_ptrTail->m_ptrNext = currentNode;

		// Update tail to be the currentNode
		m_ptrTail = currentNode;
	}

	inline void moveToFront(std::shared_ptr<Item> ptrItem)
	{
		if (ptrItem == m_ptrHead)
		{
			//m_ptrHead->m_ptrPrev = nullptr;
			return;
		}

		if (ptrItem->m_ptrPrev) {
			ptrItem->m_ptrPrev->m_ptrNext = ptrItem->m_ptrNext;
		}

		if (ptrItem->m_ptrNext) {
			ptrItem->m_ptrNext->m_ptrPrev = ptrItem->m_ptrPrev;
		}

		if (ptrItem == m_ptrTail) {
			m_ptrTail = ptrItem->m_ptrPrev;
		}

		ptrItem->m_ptrPrev = nullptr;
		ptrItem->m_ptrNext = m_ptrHead;

		if (m_ptrHead) {
			m_ptrHead->m_ptrPrev = ptrItem;
		}
		m_ptrHead = ptrItem;
	}

	inline void removeFromLRU(std::shared_ptr<Item> ptrItem)
	{
		if (ptrItem->m_ptrPrev != nullptr) {
			ptrItem->m_ptrPrev->m_ptrNext = ptrItem->m_ptrNext;
		}
		else {
			// If the node to be removed is the head
			m_ptrHead = ptrItem->m_ptrNext;
			if (m_ptrHead != nullptr)
				m_ptrHead->m_ptrPrev = nullptr;

		}

		if (ptrItem->m_ptrNext != nullptr) {
			ptrItem->m_ptrNext->m_ptrPrev = ptrItem->m_ptrPrev;
		}
		else {
			// If the node to be removed is the tail
			m_ptrTail = ptrItem->m_ptrPrev;
			if (m_ptrTail != nullptr)

				m_ptrTail->m_ptrNext = nullptr;
		}
	}

	int getlrucount()
	{
		int cnt = 0;
		std::shared_ptr<Item> _ptrItem = m_ptrHead;
		do
		{
			cnt++;
			_ptrItem = _ptrItem->m_ptrNext;

		} while (_ptrItem != nullptr);
		return cnt;
	}

	inline void flushItemsToStorage()
	{
#ifdef __CONCURRENT__

		std::vector<std::pair<ObjectUIDType, ObjectTypePtr>> vtItems;

		std::unique_lock<std::shared_mutex> lock_cache(m_mtxCache);

		if (m_ptrObjects.size() < m_nCacheCapacity)
			return;

		size_t nFlushCount = m_ptrObjects.size() - m_nCacheCapacity;
		for (size_t idx = 0; idx < nFlushCount; idx++)
		{
			if (!m_ptrTail->m_ptrObject->mutex.try_lock())
			{
				// in use.. TODO: proceed with the next one.
				break;
			}
			m_ptrTail->m_ptrObject->mutex.unlock();


			std::shared_ptr<Item> ptrTemp = m_ptrTail;

			vtItems.push_back(std::pair<ObjectUIDType, ObjectTypePtr>(ptrTemp->m_uidSelf, ptrTemp->m_ptrObject));

			m_ptrObjects.erase(ptrTemp->m_uidSelf);

			m_ptrTail = ptrTemp->m_ptrPrev;

			ptrTemp->m_ptrPrev = nullptr;
			ptrTemp->m_ptrNext = nullptr;

			if (m_ptrTail)
			{
				m_ptrTail->m_ptrNext = nullptr;
			}
			else
			{
				m_ptrHead = nullptr;
			}
		}

		std::unique_lock<std::shared_mutex> lock_storage(m_mtxStorage);

		lock_cache.unlock();

		auto it = vtItems.begin();
		while (it != vtItems.end())
		{
			m_ptrStorage->addObject((*it).first, (*it).second);

			if ((*it).second.use_count() != 2)
			{
				throw new std::exception("should not occur!");
			}

			it++;
		}

		vtItems.clear();
#else
		while (m_ptrObjects.size() >= m_nCacheCapacity)
		{
#ifdef __POSITION_AWARE_ITEMS__
			if (m_ptrTail->m_uidParent == std::nullopt)
			{
				//moveToFront(m_ptrTail);
				return;
			}

			std::shared_ptr<Item> _ptrPrev = m_ptrTail->m_ptrPrev;
			while (_ptrPrev->m_ptrPrev != nullptr)
			{
				if (m_ptrTail->m_uidSelf == _ptrPrev->m_uidParent)
				{
					int b = getlrucount();
					interchangeWithTail(_ptrPrev);
					int a = getlrucount();
					if (a!=b)
						std::cout << "mismatch" << std::endl;

					_ptrPrev = m_ptrTail->m_ptrPrev;
					continue;
				}
				_ptrPrev = _ptrPrev->m_ptrPrev;
			}

			if (m_ptrTail->m_uidParent == std::nullopt)
			{
				//moveToFront(m_ptrTail);
				return;
			}


			m_ptrStorage->addObject(m_ptrTail->m_uidSelf, m_ptrTail->m_ptrObject, m_ptrTail->m_uidParent);
#else
			m_ptrStorage->addObject(m_ptrTail->m_uidSelf, m_ptrTail->m_ptrObject);
#endif __POSITION_AWARE_ITEMS__
			m_ptrObjects.erase(m_ptrTail->m_uidSelf);

			std::shared_ptr<Item> ptrTemp = m_ptrTail;
			m_ptrTail = m_ptrTail->m_ptrPrev;
			if (m_ptrTail)
			{
				m_ptrTail->m_ptrNext = nullptr;
			}
			else
			{
				m_ptrHead = nullptr;
			}
		}
#endif __CONCURRENT__
	}

#ifdef __CONCURRENT__
	static void handlerCacheFlush(SelfType* ptrSelf)
	{
		do
		{
			ptrSelf->flushItemsToStorage();

			std::this_thread::sleep_for(100ms);

		} while (!ptrSelf->m_bStop);
	}
#endif __CONCURRENT__

#ifdef __POSITION_AWARE_ITEMS__
public:
	CacheErrorCode updateChildUID(std::optional<ObjectUIDType>& uidObject, ObjectUIDType uidChildOld, ObjectUIDType uidChildNew)
	{
		return CacheErrorCode::Success;
	}

	CacheErrorCode updateChildUID(std::vector<std::pair<ObjectUIDType, std::pair<ObjectUIDType, ObjectUIDType>>> vtUpdatedUIDs)
	{
		return CacheErrorCode::Success;
	}
#endif __POSITION_AWARE_ITEMS__
};