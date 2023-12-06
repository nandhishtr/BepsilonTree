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
class LRUCacheObject
{
public:
	mutable std::shared_mutex mutex;
	std::shared_ptr<std::variant<ValueCoreTypes...>> data;

public:
	LRUCacheObject(std::shared_ptr<std::variant<ValueCoreTypes...>> ptrValue)
	{
		data = ptrValue;
	}

	template<class Type, typename... ArgsType>
	static std::shared_ptr<LRUCacheObject> createObjectOfType(ArgsType... args)
	{
		std::shared_ptr<std::variant<ValueCoreTypes...>> ptrValue = std::make_shared<std::variant<ValueCoreTypes...>>(std::make_shared<Type>(args...));

		return std::make_shared<LRUCacheObject>(ptrValue);
	}
};