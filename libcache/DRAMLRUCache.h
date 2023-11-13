#pragma once
#include <iostream>
#include <unordered_map>

#include "ICache.h"
#include "DRAMCacheObject.h"
#include "DRAMCacheObjectKey.h"
#include "UnsortedMapUtil.h"

template <typename KeyType, template <typename> typename ValueType, typename ValueCoreType>
class DRAMLRUCache : public ICoreCache
{
private:
	struct Item 
	{
	public:
		std::shared_ptr<KeyType> m_ptrKey;
		std::shared_ptr<ValueType<ValueCoreType>> m_ptrValue;
		std::shared_ptr<Item> m_ptrPrev;
		std::shared_ptr<Item> m_ptrNext;

		Item(std::shared_ptr<KeyType> oKey, std::shared_ptr<ValueType<ValueCoreType>> ptrValue)
			: m_ptrNext(nullptr)
			, m_ptrPrev(nullptr)
		{
			m_ptrKey = oKey;
			m_ptrValue = ptrValue;
		}
	};

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
	std::shared_ptr<Type> getObjectOfType(KeyType& objKey/*, Hints for nvm!!*/)
	{
		std::shared_ptr<KeyType> key = std::make_shared<KeyType>(objKey);
		if (m_mpObject.find(key) != m_mpObject.end())
		{
			std::shared_ptr<Item> ptrItem = m_mpObject[key];
			moveToFront(ptrItem);
			return std::static_pointer_cast<Type>(ptrItem->m_ptrValue->m_ptrCoreObject);
		}

		return nullptr;
	}

	template<class Type, typename... ArgsType>
	KeyType createObjectOfType(ArgsType ... args)
	{
		//.. do we really need these much objects? ponder!
		std::shared_ptr<ValueCoreType> ptrCoreObj = std::make_shared<Type>(args...);
		std::shared_ptr<KeyType> key = this->getKey(ptrCoreObj);

		std::shared_ptr<ValueType<ValueCoreType>> ptrDRAMObj = std::make_shared<ValueType<ValueCoreType>>(ptrCoreObj);

		std::shared_ptr<Item> ptrItem = std::make_shared<Item>(key, ptrDRAMObj);

		if (m_mpObject.find(key) != m_mpObject.end())
		{
			// Update existing key
			std::shared_ptr<Item> ptrItem = m_mpObject[key];
			ptrItem->m_ptrValue = ptrDRAMObj;
			moveToFront(ptrItem);
		}
		else
		{
			// Insert new key
			if (m_mpObject.size() == m_nCapacity)
			{
				if (m_ptrTail->m_ptrValue.use_count() > 1)
				{
					throw std::invalid_argument("reached an invalid state..");
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

		return *key.get();
		
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

	std::shared_ptr<KeyType> getKey(std::shared_ptr<ValueCoreType> obj)
	{
		std::shared_ptr<KeyType> ptrKey = std::make_shared<KeyType>();
		generateKey(ptrKey, obj);

		return ptrKey;
	}

	void generateKey(std::shared_ptr<uintptr_t> key, std::shared_ptr<ValueCoreType> obj)
	{
		*key = reinterpret_cast<uintptr_t>(&(*obj.get()));
	}

	void generateKey(std::shared_ptr<DRAMCacheObjectKey> key, std::shared_ptr<ValueCoreType> obj)
	{
		*key = DRAMCacheObjectKey(&(*obj.get()));
	}
};

//template class DRAMLRUCache<uintptr_t, DRAMCacheObject>;
//template class DRAMLRUCache<DRAMCacheObjectKey, DRAMCacheObject>;
