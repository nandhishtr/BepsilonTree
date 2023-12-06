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