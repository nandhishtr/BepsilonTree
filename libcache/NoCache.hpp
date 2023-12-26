#pragma once
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <syncstream>
#include <thread>
#include <variant>
#include <typeinfo>

#include "CacheErrorCodes.h"
#include "IFlushCallback.h"

template<typename KeyType, template <typename...> typename ValueType, typename... ValueCoreTypes>
class NoCache
{
public:
	typedef KeyType ObjectUIDType;
	typedef ValueType<ValueCoreTypes...> ObjectType;
	typedef ValueType<ValueCoreTypes...>* ObjectTypePtr;
	typedef std::tuple<ValueCoreTypes...> ObjectCoreTypes;

public:
	~NoCache()
	{
	}

	NoCache()
	{
	}

	template <typename... InitArgs>
	CacheErrorCode init(InitArgs... args)
	{
		return CacheErrorCode::Success;
	}

	CacheErrorCode remove(ObjectUIDType objKey)
	{
		ObjectTypePtr ptrValue = reinterpret_cast<ObjectTypePtr>(objKey);
		delete ptrValue;

		return CacheErrorCode::KeyDoesNotExist;
	}

	CacheErrorCode getObject(ObjectUIDType objKey, ObjectTypePtr& ptrObject)
	{
		ptrObject = reinterpret_cast<ObjectTypePtr>(objKey);
		return CacheErrorCode::Success;
	}

	template <typename Type>
	CacheErrorCode getObjectOfType(ObjectUIDType objKey, Type& ptrObject)
	{
		ObjectTypePtr ptrValue = reinterpret_cast<ObjectTypePtr>(objKey);

		if (std::holds_alternative<Type>(*ptrValue->data))
		{
			ptrObject = std::get<Type>(*ptrValue->data);
			return CacheErrorCode::Success;
		}

		return CacheErrorCode::Error;
	}

	template<class Type, typename... ArgsType>
	CacheErrorCode createObjectOfType(std::optional<ObjectUIDType>& key, ArgsType... args)
	{
		//ObjectTypePtr ptrValue = new std::variant<ValueCoreTypes...>(std::make_shared<Type>(args...));
		
		ValueType<ValueCoreTypes...>* ptrValue = ValueType<ValueCoreTypes...>::template createObjectOfType<Type>(args...);

		key = reinterpret_cast<ObjectUIDType>(ptrValue);
		return CacheErrorCode::Success;
	}

	void getstate(size_t& lru, size_t& map)
	{
		lru = 1;
		map = 1;
	}
};
