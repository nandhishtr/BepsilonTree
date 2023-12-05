#pragma once
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <syncstream>
#include <thread>
#include <variant>
#include <typeinfo>

#include "ErrorCodes.h"

template <typename... ValueCoreTypes>
class NoCacheObject
{
public:
	typedef std::variant<ValueCoreTypes...>* CacheValueTypePtr;

	CacheValueTypePtr data;
	mutable std::shared_mutex mutex;

public:
	NoCacheObject(CacheValueTypePtr ptrValue)
	{
		data = ptrValue;
	}

	template<class Type, typename... ArgsType>
	static NoCacheObject* createObjectOfType(ArgsType... args)
	{
		CacheValueTypePtr ptrValue = new std::variant<ValueCoreTypes...>(std::make_shared<Type>(args...));

		NoCacheObject* ptr = new NoCacheObject(ptrValue);

		return ptr;
	}
};


template<typename KeyType, template <typename...> typename ValueType, typename... ValueCoreTypes>
class NoCache
{
public:
	typedef KeyType KeyType;
	typedef ValueType<ValueCoreTypes...>* CacheValueType;

private:
	typedef ValueType<ValueCoreTypes...>* CacheValueTypePtr;

public:
	~NoCache()
	{
	}

	NoCache()
	{
	}

	CacheErrorCode remove(KeyType objKey)
	{
		CacheValueTypePtr ptrValue = reinterpret_cast<CacheValueTypePtr>(objKey);
		delete ptrValue;

		return CacheErrorCode::KeyDoesNotExist;
	}

	CacheValueTypePtr getObjectOfType(KeyType objKey)
	{
		return reinterpret_cast<CacheValueTypePtr>(objKey);
	}

	template <typename Type>
	Type getObjectOfType(KeyType objKey)
	{
		CacheValueTypePtr ptrValue = reinterpret_cast<CacheValueTypePtr>(objKey);

		if (std::holds_alternative<Type>(*ptrValue->data))
		{
			return std::get<Type>(*ptrValue->data);
		}

		return nullptr;
	}

	template<class Type, typename... ArgsType>
	KeyType createObjectOfType(ArgsType... args)
	{
		//CacheValueTypePtr ptrValue = new std::variant<ValueCoreTypes...>(std::make_shared<Type>(args...));
		
		NoCacheObject<ValueCoreTypes...>* ptrValue = NoCacheObject<ValueCoreTypes...>::template createObjectOfType<Type>(args...);

		return reinterpret_cast<KeyType>(ptrValue);
	}

	//template<class Type, typename... ArgsType>
	//CacheValueTypePtr createObjectOfType_(ArgsType ... args)
	//{
	//	//std::shared_ptr<Type> ptrType = std::make_shared<Type>(args...);
	//	CacheValueTypePtr ptrValue = std::make_shared<CacheValueType>(args...);

	//	return ptrValue;// reinterpret_cast<KeyType>(ptrValue);
	//}
};