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
	typedef std::variant<std::shared_ptr<ValueCoreTypes>...> CacheValueType;
	typedef std::variant<std::shared_ptr<ValueCoreTypes>...>* CacheValueTypePtr;

public:
	typedef std::tuple<ValueCoreTypes...> ObjectCoreTypes;

public:
	bool dirty;
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
		CacheValueTypePtr ptrValue = new CacheValueType(std::make_shared<Type>(args...));

		NoCacheObject* ptr = new NoCacheObject(ptrValue);

		return ptr;
	}

	template<class Type, typename... ArgsType>
	static NoCacheObject* createObjectOfType(std::shared_ptr<Type>& ptrCoreValue, ArgsType... args)
	{
		ptrCoreValue = std::make_shared<Type>(args...);
		CacheValueTypePtr ptrValue = new CacheValueType(ptrCoreValue);

		NoCacheObject* ptr = new NoCacheObject(ptrValue);

		return ptr;
	}
};