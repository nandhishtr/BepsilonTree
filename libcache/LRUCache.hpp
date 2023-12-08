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

#include "ErrorCodes.h"
#include "UnsortedMapUtil.h"

//#define __CONCURRENT__

template <
	template <typename, template <typename, typename> typename, typename, typename> typename StorageType, typename KeyType,
	template <typename, typename...> typename ValueType, typename ValueCoreTypesMarshaller, typename... ValueCoreTypes
>
class LRUCache
{
	typedef LRUCache<StorageType, KeyType, ValueType, ValueCoreTypesMarshaller, ValueCoreTypes...> SelfType;

public:
	typedef KeyType KeyType;
	typedef std::shared_ptr<ValueType<ValueCoreTypesMarshaller, ValueCoreTypes...>> CacheValueType;

private:
	struct Item
	{
	public:
		KeyType m_oKey;
		CacheValueType m_ptrValue;
		std::shared_ptr<Item> m_ptrPrev;
		std::shared_ptr<Item> m_ptrNext;

		Item(KeyType& key, CacheValueType ptrValue)
			: m_ptrNext(nullptr)
			, m_ptrPrev(nullptr)
		{
			m_oKey = key;
			m_ptrValue = ptrValue;
		}
	};

	std::unordered_map<KeyType, std::shared_ptr<Item>> m_mpObject;


	size_t m_nCapacity;
	std::shared_ptr<Item> m_ptrHead;
	std::shared_ptr<Item> m_ptrTail;

#ifdef __CONCURRENT__
	bool m_bStop;

	std::thread m_threadLRU;
	std::thread m_threadCacheFlush;

	mutable std::shared_mutex m_mtxCache;
	mutable std::shared_mutex m_mtxLRUQueue;

	std::queue<std::shared_ptr<Item>> m_qLRUItems;
#endif __CONCURRENT__

	std::unique_ptr<StorageType<KeyType, ValueType, ValueCoreTypesMarshaller, ValueCoreTypes...>> m_ptrStorage;

public:
	~LRUCache()
	{
	}

	template <typename... StorageArgs>
	LRUCache(size_t nCapacity, StorageArgs... args)
		: m_nCapacity(nCapacity)
		, m_ptrHead(nullptr)
		, m_ptrTail(nullptr)
	{
		m_ptrStorage = std::make_unique<StorageType<KeyType, ValueType, ValueCoreTypesMarshaller, ValueCoreTypes...>>(args...);

#ifdef __CONCURRENT__
		m_bStop = false;
		m_threadLRU = std::thread(handlerLRUQueue, this);
		m_threadCacheFlush = std::thread(handlerCacheFlush, this);
#endif __CONCURRENT__
	}

	CacheErrorCode remove(KeyType objKey)
	{
#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex>  lock_store(m_mtxCache);
#endif __CONCURRENT__

		auto it = m_mpObject.find(objKey);
		if (it != m_mpObject.end()) 
		{
			//TODO: explicitly call std::shared destructor!
			//TODO: look for ref count?
			m_mpObject.erase(it);
			return CacheErrorCode::Success;
		}

		return CacheErrorCode::KeyDoesNotExist;
	}

	CacheValueType getObject(KeyType key)
	{
#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex> lock_store(m_mtxCache);
#endif __CONCURRENT__

		if (m_mpObject.find(key) != m_mpObject.end())
		{
			std::shared_ptr<Item> ptrItem = m_mpObject[key];

#ifdef __CONCURRENT__
			lock_store.unlock();
			
			std::unique_lock<std::shared_mutex> lock_lru_queue(m_mtxLRUQueue);

			m_qLRUItems.push(ptrItem);
#else
			moveToFront(ptrItem);
#endif __CONCURRENT__

			return ptrItem->m_ptrValue;
		}

		std::shared_ptr<ValueType<ValueCoreTypesMarshaller, ValueCoreTypes...>> ptrValue = m_ptrStorage->getObject(key);
		if (ptrValue != nullptr)
		{
			std::shared_ptr<Item> ptrItem = std::make_shared<Item>(key, ptrValue);

			m_mpObject[key] = ptrItem;

#ifdef __CONCURRENT__
			lock_store.unlock();

			std::unique_lock<std::shared_mutex> lock_lru_queue(m_mtxLRUQueue);

			m_qLRUItems.push(ptrItem);
#else
			moveToFront(ptrItem);
			flushItemsToStorage();
#endif __CONCURRENT__

			return ptrValue;
		}
		
		return nullptr;
	}

	template <typename Type>
	Type getObjectOfType(KeyType key)
	{
#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex> lock_store(m_mtxCache);
#endif __CONCURRENT__

		if (m_mpObject.find(key) != m_mpObject.end())
		{
			std::shared_ptr<Item> ptrItem = m_mpObject[key];

#ifdef __CONCURRENT__
			lock_store.unlock();

			std::unique_lock<std::shared_mutex> lock_lru_queue(m_mtxLRUQueue);

			m_qLRUItems.push(ptrItem);
#else
			moveToFront(ptrItem);

#endif __CONCURRENT__

			if (std::holds_alternative<Type>(*ptrItem->m_ptrValue->data))
			{
				return std::get<Type>(*ptrItem->m_ptrValue->data);
			}

			return nullptr;
		}

		std::shared_ptr<ValueType<ValueCoreTypesMarshaller, ValueCoreTypes...>> ptrValue = m_ptrStorage->getObject(key);
		if (ptrValue != nullptr)
		{
			std::shared_ptr<Item> ptrItem = std::make_shared<Item>(key, ptrValue);

			m_mpObject[key] = ptrItem;

#ifdef __CONCURRENT__
			lock_store.unlock();

			std::unique_lock<std::shared_mutex> lock_lru_queue(m_mtxLRUQueue);

			m_qLRUItems.push(ptrItem);

			lock_lru_queue.unlock();
#else
			moveToFront(ptrItem);
			flushItemsToStorage();
#endif __CONCURRENT__


			if (std::holds_alternative<Type>(*ptrValue->data))
			{
				return std::get<Type>(*ptrValue->data);
			}

			return nullptr;
		}

		return nullptr;
	}

	template<class Type, typename... ArgsType>
	KeyType createObjectOfType(ArgsType... args)
	{
#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex> lock_store(m_mtxCache);
#endif __CONCURRENT__

		//TODO .. do we really need these much objects? ponder!
		KeyType key;
		std::shared_ptr<ValueType<ValueCoreTypesMarshaller, ValueCoreTypes...>> ptrValue = ValueType<ValueCoreTypesMarshaller, ValueCoreTypes...>::template createObjectOfType<Type>(args...);

		this->generateKey(key, ptrValue);

		std::shared_ptr<Item> ptrItem = std::make_shared<Item>(key, ptrValue);

		if (m_mpObject.find(key) != m_mpObject.end())
		{
			std::shared_ptr<Item> ptrItem = m_mpObject[key];
			ptrItem->m_ptrValue = ptrValue;
		}
		else
		{
			m_mpObject[key] = ptrItem;
		}

#ifdef __CONCURRENT__
		lock_store.unlock();

		std::unique_lock<std::shared_mutex> lock_lru_queue(m_mtxLRUQueue);

		m_qLRUItems.push(ptrItem);
#else
		moveToFront(ptrItem);
		flushItemsToStorage();
#endif __CONCURRENT__

		return key;
	}

private:
	inline void moveToFront(std::shared_ptr<Item> ptrItem)
	{
		if (ptrItem == m_ptrHead)
			return;

		if (!m_ptrHead) 
		{
			m_ptrHead = ptrItem;
			m_ptrTail = ptrItem;

			ptrItem->m_ptrNext = nullptr;
			ptrItem->m_ptrPrev = nullptr;

			return;
		}


		if (ptrItem == m_ptrTail)
		{
			m_ptrTail = ptrItem->m_ptrPrev;
			m_ptrTail->m_ptrNext = nullptr;
			
			ptrItem->m_ptrPrev = nullptr;
			ptrItem->m_ptrNext = m_ptrHead;
			
			m_ptrHead->m_ptrPrev = ptrItem;
			m_ptrHead = ptrItem;

			return;
		}

		if (ptrItem->m_ptrPrev != nullptr && ptrItem->m_ptrNext != nullptr)
		{
			ptrItem->m_ptrPrev->m_ptrNext = ptrItem->m_ptrNext;
			ptrItem->m_ptrNext->m_ptrPrev = ptrItem->m_ptrPrev;
		}

		ptrItem->m_ptrPrev = nullptr;
		ptrItem->m_ptrNext = m_ptrHead;
		m_ptrHead->m_ptrPrev = ptrItem;
		m_ptrHead = ptrItem;
	}

	inline void flushItemsToStorage()
	{
#ifdef __CONCURRENT__
		int nFlushCount = 0;
		std::vector<KeyType> vtItems;

		std::unique_lock<std::shared_mutex> lock_store(m_mtxCache);

		if ((nFlushCount = m_mpObject.size()) <= m_nCapacity)
			return;

		lock_store.unlock();

		// !!! Address following!!
		//m_ptrStorage->addObject(m_ptrTail->m_oKey, m_ptrTail->m_ptrValue);

		std::unique_lock<std::shared_mutex> lock_lru_queue(m_mtxLRUQueue);

		for (int idx = 0; idx < nFlushCount; idx++)
		{
			if (m_ptrTail == nullptr)
				break;

			std::shared_ptr<Item> ptrTemp = m_ptrTail;

			vtItems.push_back(ptrTemp->m_oKey);

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

		lock_lru_queue.unlock();

		std::unique_lock<std::shared_mutex> re_lock_store(m_mtxCache);

		auto it = vtItems.begin();
		while (it != vtItems.end())
		{
			m_mpObject.erase(*it);
		}
#else
		while (m_mpObject.size() >= m_nCapacity)
		{
			m_ptrStorage->addObject(m_ptrTail->m_oKey, m_ptrTail->m_ptrValue);
			m_mpObject.erase(m_ptrTail->m_oKey);

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

	inline void generateKey(uintptr_t& key, std::shared_ptr<ValueType<ValueCoreTypesMarshaller, ValueCoreTypes...>> ptrValue)
	{
		key = reinterpret_cast<uintptr_t>(&(*ptrValue.get()));
	}

#ifdef __CONCURRENT__
	static void handlerLRUQueue(SelfType* ptrSelf) 
	{
		std::queue<std::shared_ptr<Item>> qLocal;

		do 
		{
			std::unique_lock<std::shared_mutex> lock_store(ptrSelf->m_mtxLRUQueue);

			qLocal = ptrSelf->m_qLRUItems;

			lock_store.unlock();

			while (qLocal.size() > 0)
			{
				// TODO: logic to move items in bunch!
				ptrSelf->moveToFront(qLocal.front());
				qLocal.pop();
			}

			std::this_thread::sleep_for(100ms);

		} while (!ptrSelf->m_bStop);
	}

	static void handlerCacheFlush(SelfType* ptrSelf)
	{
		do
		{
			ptrSelf->flushItemsToStorage();

			std::this_thread::sleep_for(100ms);

		} while (!ptrSelf->m_bStop);
	}
#endif __CONCURRENT__
};