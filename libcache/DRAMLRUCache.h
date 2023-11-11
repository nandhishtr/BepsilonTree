#pragma once
#include <iostream>
#include <unordered_map>

#include "ICache.h"
#include "DRAMCacheObject.h"
#include "DRAMCacheObjectKey.h"
#include "UnsortedMapUtil.h"

template <typename KeyType = uintptr_t, typename ValueType = DRAMCacheObject>
class DRAMLRUCache : public ICoreCache
{
private:
	struct Item {
	public:
		std::shared_ptr<KeyType> m_ptrKey;
		std::shared_ptr<ValueType> m_ptrValue;
		std::shared_ptr<Item> m_ptrPrev;
		std::shared_ptr<Item> m_ptrNext;

		Item(std::shared_ptr<KeyType> oKey, std::shared_ptr<ValueType> ptrValue)
			: m_ptrNext(nullptr)
			, m_ptrPrev(nullptr)
		{
			m_ptrKey = oKey;
			m_ptrValue = ptrValue;
		}
	};

	//std::unordered_map<KeyType, std::shared_ptr<ValueType>, std::hash<KeyType>> m_mpObject;
	std::unordered_map<std::shared_ptr<KeyType>, std::shared_ptr<Item>, SharedPtrHash<KeyType>, SharedPtrEqual<KeyType>> m_mpObject;

	size_t m_nCapacity;
	std::shared_ptr<Item> m_ptrHead;
	std::shared_ptr<Item> m_ptrTail;

public:
	DRAMLRUCache(size_t nCapacity)
		: m_nCapacity(nCapacity)
		, m_ptrHead(nullptr)
		, m_ptrTail(nullptr)
	{
	}

	CacheErrorCode remove(KeyType& objKey)
	{
		/*auto it = m_mpObject.find(objKey);
		if (it != m_mpObject.end()) {
			m_mpObject.erase(it);
			return CacheErrorCode::Success;
		}*/

		return CacheErrorCode::KeyDoesNotExist;
	}

	template<class Type>
	std::shared_ptr<Type> getObjectOfType(KeyType& objKey)
	{
		std::shared_ptr<KeyType> key = std::make_shared<KeyType>(objKey);
		if (m_mpObject.find(key) != m_mpObject.end())
		{
			std::shared_ptr<Item> ptrItem = m_mpObject[key];
			moveToFront(ptrItem);
			return std::static_pointer_cast<Type>(ptrItem->m_ptrValue);
		}

		/*auto it = m_mpObject.find(objKey);
		if (it != m_mpObject.end()) {
			return std::static_pointer_cast<Type>(it->second);
		}*/

		return nullptr;
	}

	template<class Type, typename... ArgsType>
	KeyType createObjectOfType(ArgsType ... args)
	{
		std::shared_ptr<Type> obj = std::make_shared<Type>(args...);
		////
		KeyType objKey = NULL;
		ValueType::get(objKey, obj);

		std::shared_ptr<KeyType> key = std::make_shared<KeyType>(objKey);
		//m_mpObject[objKey] = obj;



		std::shared_ptr<Item> ptrItem = std::make_shared<Item>(key, obj);

		if (m_mpObject.find(key) != m_mpObject.end())
		{
			// Update existing key
			std::shared_ptr<Item> ptrItem = m_mpObject[key];
			ptrItem->m_ptrValue = obj;

			moveToFront(ptrItem);
		}
		else
		{
			// Insert new key
			if (m_mpObject.size() == m_nCapacity)
			{

				if (m_ptrTail->m_ptrValue.use_count() > 1)
				{
					std::cout << "****************************************************tail is still in use.." << std::endl;
					//return nullptr;
				}

				// Remove the least recently used item
				m_mpObject.erase(m_ptrTail->m_ptrKey);

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

			m_mpObject[key] = ptrItem;
			if (!m_ptrHead) {
				m_ptrHead = ptrItem;
				m_ptrTail = ptrItem;
			}
			else {
				ptrItem->m_ptrNext = m_ptrHead;
				m_ptrHead->m_ptrPrev = ptrItem;
				m_ptrHead = ptrItem;
			}
		}

		return objKey;
	}

private:
	void moveToFront(std::shared_ptr<Item> ptrItem)
	{
		if (ptrItem == m_ptrHead)
		{
			return;
		}

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
};

template class DRAMLRUCache<uintptr_t, DRAMCacheObject>;
template class DRAMLRUCache<DRAMCacheObjectKey, DRAMCacheObject>;
