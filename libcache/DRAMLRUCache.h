#pragma once
#include <iostream>
#include <unordered_map>

#include "ICache.h"
#include "DRAMCacheObject.h"
#include "DRAMVolatileStorage.h"

template <
	template <typename, typename> typename DRAMStorage, typename StorageKeyType, typename StorageValueCoreType,
	template <typename, typename... > typename StorageValueType, typename... StorageValueOtherCoreTypes
>
class DRAMLRUCache : public ICoreCache
{
public:
	typedef StorageKeyType KeyType;

	typedef StorageKeyType CacheStorageKeyType;

private:
	struct Item
	{
	public:
		std::shared_ptr<StorageKeyType> m_ptrKey;
		std::shared_ptr<StorageValueType<StorageValueCoreType, StorageValueOtherCoreTypes...>> m_ptrValue;
		std::shared_ptr<Item> m_ptrPrev;
		std::shared_ptr<Item> m_ptrNext;

		Item(std::shared_ptr<StorageKeyType> oKey, std::shared_ptr<StorageValueType<StorageValueCoreType, StorageValueOtherCoreTypes...>> ptrValue)
			: m_ptrNext(nullptr)
			, m_ptrPrev(nullptr)
		{
			m_ptrKey = oKey;
			m_ptrValue = ptrValue;
		}
	};

	std::unordered_map<std::shared_ptr<StorageKeyType>, std::shared_ptr<Item>, SharedPtrHash<StorageKeyType>, SharedPtrEqual<StorageKeyType>> m_mpObject;

	size_t m_nCapacity;
	std::shared_ptr<Item> m_ptrHead;
	std::shared_ptr<Item> m_ptrTail;

	std::unique_ptr<DRAMStorage<StorageKeyType, StorageValueCoreType>> m_ptrStorage;
public:
	DRAMLRUCache(size_t nCapacity)
		: m_nCapacity(nCapacity)
		, m_ptrHead(nullptr)
		, m_ptrTail(nullptr)
	{
		const std::size_t n = sizeof...(StorageValueOtherCoreTypes);
		std::cout << n << std::endl;

		m_ptrStorage = std::make_unique<DRAMStorage<StorageKeyType, StorageValueCoreType>>(1000);
	}

	CacheErrorCode remove(StorageKeyType& objKey)
	{
		/*auto it = m_mpObject.find(objKey);
		if (it != m_mpObject.end()) {
			m_mpObject.erase(it);
			return CacheErrorCode::Success;
		}*/

		return CacheErrorCode::KeyDoesNotExist;
	}

	template<class Type>
	std::shared_ptr<Type> getObjectOfType(StorageKeyType& objKey/*, Hints for nvm!!*/)
	{
		std::shared_ptr<StorageKeyType> ptrKey = std::make_shared<StorageKeyType>(objKey);	//TODO: Use Object type directly.
		if (m_mpObject.find(ptrKey) != m_mpObject.end())
		{
			std::shared_ptr<Item> ptrItem = m_mpObject[ptrKey];
			moveToFront(ptrItem);
			return std::static_pointer_cast<Type>(ptrItem->m_ptrValue->m_ptrCoreObject);
		}

		std::shared_ptr<StorageValueCoreType> ptrValue = m_ptrStorage->getObject(ptrKey);
		if (ptrValue != nullptr)
		{
			moveItemToDRAM(ptrKey, ptrValue);
			return std::static_pointer_cast<Type>(ptrValue);
		}

		return nullptr;
	}

	template<class Type, typename... ArgsType>
	StorageKeyType createObjectOfType(ArgsType ... args)
	{
		//TODO .. do we really need these much objects? ponder!
		std::shared_ptr<StorageValueCoreType> ptrCoreObj = std::make_shared<Type>(args...);

		std::shared_ptr<StorageKeyType> ptrKey = this->getKey(ptrCoreObj);

		std::shared_ptr<StorageValueType<StorageValueCoreType, StorageValueOtherCoreTypes...>> ptrDRAMObj = std::make_shared<StorageValueType<StorageValueCoreType, StorageValueOtherCoreTypes...>>(ptrCoreObj);

		std::shared_ptr<Item> ptrItem = std::make_shared<Item>(ptrKey, ptrDRAMObj);

		if (m_mpObject.find(ptrKey) != m_mpObject.end())
		{
			std::shared_ptr<Item> ptrItem = m_mpObject[ptrKey];
			ptrItem->m_ptrValue = ptrDRAMObj;
			moveToFront(ptrItem);
		}
		else
		{
			tryMoveItemToStorage();

			m_mpObject[ptrKey] = ptrItem;
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

		return *ptrKey.get();

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

	void tryMoveItemToStorage()
	{
		while (m_mpObject.size() >= m_nCapacity)
		{
			if (m_ptrTail->m_ptrValue.use_count() > 1)
			{
				throw std::invalid_argument("reached an invalid state..");
			}

			m_ptrStorage->addObject(m_ptrTail->m_ptrKey, m_mpObject[m_ptrTail->m_ptrKey]->m_ptrValue->m_ptrCoreObject);
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
	}

	void moveItemToDRAM(std::shared_ptr<StorageKeyType> ptrKey, std::shared_ptr<StorageValueCoreType> ptrCoreObj)
	{
		std::shared_ptr<StorageValueType<StorageValueCoreType, StorageValueOtherCoreTypes...>> ptrDRAMObj = std::make_shared<StorageValueType<StorageValueCoreType, StorageValueOtherCoreTypes...>>(ptrCoreObj);

		std::shared_ptr<Item> ptrItem = std::make_shared<Item>(ptrKey, ptrDRAMObj);

		m_mpObject[ptrKey] = ptrItem;
		if (!m_ptrHead) {
			m_ptrHead = ptrItem;
			m_ptrTail = ptrItem;
		}
		else {
			ptrItem->m_ptrNext = m_ptrHead;
			m_ptrHead->m_ptrPrev = ptrItem;
			m_ptrHead = ptrItem;
		}

		tryMoveItemToStorage();
	}

	std::shared_ptr<StorageKeyType> getKey(std::shared_ptr<StorageValueCoreType> obj)
	{
		std::shared_ptr<StorageKeyType> ptrKey = std::make_shared<StorageKeyType>();
		generateKey(ptrKey, obj);

		return ptrKey;
	}

	void generateKey(std::shared_ptr<uintptr_t> key, std::shared_ptr<StorageValueCoreType> obj)
	{
		*key = reinterpret_cast<uintptr_t>(&(*obj.get()));
	}

	void generateKey(std::shared_ptr<DRAMObjectUID> key, std::shared_ptr<StorageValueCoreType> obj)
	{
		*key = DRAMObjectUID(&(*obj.get()));
	}
};

//template <typename StorageKeyType, template <typename, typename, typename> typename StorageValueType, typename ValueDRAMCoreType, typename ValueDRAMCoreType, typename ValueBufferCoreType>
//class DRAMLRUCacheEx : public ICoreCache
//{
//private:
//	struct Item
//	{
//	public:
//		std::shared_ptr<StorageKeyType> m_ptrKey;
//		std::shared_ptr<StorageValueType<ValueDRAMCoreType, ValueDRAMCoreType, ValueBufferCoreType>> m_ptrValue;
//		std::shared_ptr<Item> m_ptrPrev;
//		std::shared_ptr<Item> m_ptrNext;
//
//		Item(std::shared_ptr<StorageKeyType> oKey, std::shared_ptr<StorageValueType<ValueDRAMCoreType, ValueDRAMCoreType, ValueBufferCoreType>> ptrValue)
//			: m_ptrNext(nullptr)
//			, m_ptrPrev(nullptr)
//		{
//			m_ptrKey = oKey;
//			m_ptrValue = ptrValue;
//		}
//	};
//
//	std::unordered_map<std::shared_ptr<StorageKeyType>, std::shared_ptr<Item>, SharedPtrHash<StorageKeyType>, SharedPtrEqual<StorageKeyType>> m_mpObject;
//
//	size_t m_nCapacity;
//	std::shared_ptr<Item> m_ptrHead;
//	std::shared_ptr<Item> m_ptrTail;
//
//	// callback of the tree to update key.
//
//public:
//	DRAMLRUCacheEx(size_t nCapacity)
//		: m_nCapacity(nCapacity)
//		, m_ptrHead(nullptr)
//		, m_ptrTail(nullptr)
//	{
//	}
//
//	CacheErrorCode remove(StorageKeyType& objKey)
//	{
//		/*auto it = m_mpObject.find(objKey);
//		if (it != m_mpObject.end()) {
//			m_mpObject.erase(it);
//			return CacheErrorCode::Success;
//		}*/
//
//		return CacheErrorCode::KeyDoesNotExist;
//	}
//
//	//.. getMutableObjectOfType
//	//.. getReadObjectOfType
//
//	template<class Type>
//	std::shared_ptr<Type> getObjectOfType(StorageKeyType& objKey/*, Hints for nvm!!*/)
//	{
//
//
//		/*std::shared_ptr<StorageKeyType> key = std::make_shared<StorageKeyType>(objKey);
//		if (m_mpObject.find(key) != m_mpObject.end())
//		{
//			std::shared_ptr<Item> ptrItem = m_mpObject[key];
//			moveToFront(ptrItem);
//			return std::static_pointer_cast<Type>(ptrItem->m_ptrValue->m_ptrCoreObject);
//		}*/
//
//		return nullptr;
//	}
//
//	template<class Type, typename... ArgsType>
//	StorageKeyType createObjectOfType(ArgsType ... args)
//	{
//		//.. do we really need these much objects? ponder!
//		std::shared_ptr<ValueDRAMCoreType> ptrCoreObj = std::make_shared<Type>(args...);
//		std::shared_ptr<StorageKeyType> key = this->getKey(ptrCoreObj);
//
//		std::shared_ptr<ValueBufferCoreType> ptrBufferObj = nullptr;
//
//		std::shared_ptr<StorageValueType<ValueDRAMCoreType, ValueDRAMCoreType, ValueBufferCoreType>> ptrDRAMObj = std::make_shared<StorageValueType<ValueDRAMCoreType, ValueDRAMCoreType, ValueBufferCoreType>>(ptrCoreObj, nullptr, ptrBufferObj);
//
//		std::shared_ptr<Item> ptrItem = std::make_shared<Item>(key, ptrDRAMObj);
//
//		return NULL;
//
//		if (m_mpObject.find(key) != m_mpObject.end())
//		{
//			// Update existing key
//			std::shared_ptr<Item> ptrItem = m_mpObject[key];
//			ptrItem->m_ptrValue = ptrDRAMObj;
//			moveToFront(ptrItem);
//		}
//		else
//		{
//			// Insert new key
//			if (m_mpObject.size() == m_nCapacity)
//			{
//				if (m_ptrTail->m_ptrValue.use_count() > 1)
//				{
//					throw std::invalid_argument("reached an invalid state..");
//				}
//
//				// Remove the least recently used item
//				m_mpObject.erase(m_ptrTail->m_ptrKey);
//
//				std::shared_ptr<Item> ptrTemp = m_ptrTail;
//				m_ptrTail = m_ptrTail->m_ptrPrev;
//				if (m_ptrTail)
//				{
//					m_ptrTail->m_ptrNext = nullptr;
//				}
//				else
//				{
//					m_ptrHead = nullptr;
//				}
//
//				ptrTemp.reset();
//			}
//
//			m_mpObject[key] = ptrItem;
//			if (!m_ptrHead) {
//				m_ptrHead = ptrItem;
//				m_ptrTail = ptrItem;
//			}
//			else {
//				ptrItem->m_ptrNext = m_ptrHead;
//				m_ptrHead->m_ptrPrev = ptrItem;
//				m_ptrHead = ptrItem;
//			}
//		}
//
//		return *key.get();
//
//	}
//
//private:
//	void moveToFront(std::shared_ptr<Item> ptrItem)
//	{
//		if (ptrItem == m_ptrHead)
//		{
//			return;
//		}
//
//		if (ptrItem == m_ptrTail)
//		{
//			m_ptrTail = ptrItem->m_ptrPrev;
//			m_ptrTail->m_ptrNext = nullptr;
//		}
//		else
//		{
//			ptrItem->m_ptrPrev->m_ptrNext = ptrItem->m_ptrNext;
//			ptrItem->m_ptrNext->m_ptrPrev = ptrItem->m_ptrPrev;
//		}
//
//		ptrItem->m_ptrPrev = nullptr;
//		ptrItem->m_ptrNext = m_ptrHead;
//		m_ptrHead->m_ptrPrev = ptrItem;
//		m_ptrHead = ptrItem;
//	}
//
//	std::shared_ptr<StorageKeyType> getKey(std::shared_ptr<ValueDRAMCoreType> obj)
//	{
//		std::shared_ptr<StorageKeyType> ptrKey = std::make_shared<StorageKeyType>();
//		generateKey(ptrKey, obj);
//
//		return ptrKey;
//	}
//
//	void generateKey(std::shared_ptr<uintptr_t> key, std::shared_ptr<ValueDRAMCoreType> obj)
//	{
//		*key = reinterpret_cast<uintptr_t>(&(*obj.get()));
//	}
//
//	void generateKey(std::shared_ptr<DRAMObjectUID> key, std::shared_ptr<ValueDRAMCoreType> obj)
//	{
//		*key = DRAMObjectUID(&(*obj.get()));
//	}
//};
//
