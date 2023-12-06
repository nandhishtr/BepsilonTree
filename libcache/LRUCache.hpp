#pragma once
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <syncstream>
#include <thread>
#include <variant>
#include <typeinfo>
#include <unordered_map>

#include "ErrorCodes.h"
#include "UnsortedMapUtil.h"

template <
	template <typename, template <typename> typename, typename> typename StorageType, typename KeyType,
	template <typename...> typename ValueType, typename... ValueCoreTypes
>
class LRUCache
{
public:
	typedef KeyType KeyType;
	typedef std::shared_ptr<ValueType<ValueCoreTypes...>> CacheValueType;

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

	std::unique_ptr<StorageType<KeyType, ValueType, ValueCoreTypes...>> m_ptrStorage;

public:
	~LRUCache()
	{
	}

	LRUCache(size_t nCapacity = 10000)
		: m_nCapacity(nCapacity)
		, m_ptrHead(nullptr)
		, m_ptrTail(nullptr)
	{
		m_ptrStorage = std::make_unique<StorageType<KeyType, ValueType, ValueCoreTypes...>>(m_nCapacity);
	}

	CacheErrorCode remove(KeyType objKey)
	{
		auto it = m_mpObject.find(objKey);
		if (it != m_mpObject.end()) 
		{
			//TODO: explicitly call std::shared destructor!
			m_mpObject.erase(it);
			return CacheErrorCode::Success;
		}

		return CacheErrorCode::KeyDoesNotExist;
	}

	CacheValueType getObject(KeyType key)
	{
		if (m_mpObject.find(key) != m_mpObject.end())
		{
			std::shared_ptr<Item> ptrItem = m_mpObject[key];
			moveToFront(ptrItem);
			return ptrItem->m_ptrValue;
		}

		std::shared_ptr<ValueType<ValueCoreTypes...>> ptrValue = m_ptrStorage->getObject(key);
		if (ptrValue != nullptr)
		{
			moveItemToDRAM(key, ptrValue);
			return ptrValue;
		}
		
		return nullptr;
	}

	template <typename Type>
	Type getObjectOfType(KeyType key)
	{
		if (m_mpObject.find(key) != m_mpObject.end())
		{
			std::shared_ptr<Item> ptrItem = m_mpObject[key];
			moveToFront(ptrItem);

			if (std::holds_alternative<Type>(*ptrItem->m_ptrValue->data))
			{
				return std::get<Type>(*ptrItem->m_ptrValue->data);
			}

			return nullptr;
		}

		std::shared_ptr<ValueType<ValueCoreTypes...>> ptrValue = m_ptrStorage->getObject(key);
		if (ptrValue != nullptr)
		{
			moveItemToDRAM(key, ptrValue);

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
		//TODO .. do we really need these much objects? ponder!
		KeyType key;
		std::shared_ptr<ValueType<ValueCoreTypes...>> ptrValue = ValueType<ValueCoreTypes...>::template createObjectOfType<Type>(args...);

		this->generateKey(key, ptrValue);

		std::shared_ptr<Item> ptrItem = std::make_shared<Item>(key, ptrValue);

		if (m_mpObject.find(key) != m_mpObject.end())
		{
			std::shared_ptr<Item> ptrItem = m_mpObject[key];
			ptrItem->m_ptrValue = ptrValue;
			moveToFront(ptrItem);
		}
		else
		{
			tryMoveItemToStorage();

			m_mpObject[key] = ptrItem;
			
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

		return key;
	}

private:
	void moveToFront(std::shared_ptr<Item> ptrItem)
	{
		if (ptrItem == m_ptrHead)
			return;

		if (ptrItem == m_ptrTail)
		{
			m_ptrTail = ptrItem->m_ptrPrev;
			m_ptrTail->m_ptrNext = nullptr;
		}
		else
		{
			ptrItem->m_ptrPrev->m_ptrNext = ptrItem->m_ptrNext;
			ptrItem->m_ptrNext->m_ptrPrev = ptrItem->m_ptrPrev;
		}

		ptrItem->m_ptrPrev = nullptr;
		ptrItem->m_ptrNext = m_ptrHead;
		m_ptrHead->m_ptrPrev = ptrItem;
		m_ptrHead = ptrItem;
	}

	void tryMoveItemToStorage()
	{
		while (m_mpObject.size() >= m_nCapacity)
		{
			if (m_ptrTail->m_ptrValue.use_count() > 1)
			{
				throw std::invalid_argument("reached an invalid state..");
			}

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

			ptrTemp.reset();
		}
	}

	void moveItemToDRAM(KeyType key, std::shared_ptr<ValueType<ValueCoreTypes...>> ptrNVRAMObj)
	{
		std::shared_ptr<Item> ptrItem = std::make_shared<Item>(key, ptrNVRAMObj);

		m_mpObject[key] = ptrItem;
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

		tryMoveItemToStorage();
	}

	void generateKey(uintptr_t& key, std::shared_ptr<ValueType<ValueCoreTypes...>> ptrValue)
	{
		key = reinterpret_cast<uintptr_t>(&(*ptrValue.get()));
	}
};