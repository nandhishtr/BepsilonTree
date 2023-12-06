#pragma once
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <syncstream>
#include <thread>
#include <variant>
#include <typeinfo>

#include "ErrorCodes.h"

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

	CacheValueTypePtr getObject(KeyType objKey)
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
		
		ValueType<ValueCoreTypes...>* ptrValue = ValueType<ValueCoreTypes...>::template createObjectOfType<Type>(args...);

		return reinterpret_cast<KeyType>(ptrValue);
	}
};
