#pragma once
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <syncstream>
#include <thread>
#include <variant>
#include <typeinfo>

#include <iostream>
#include <fstream>

#include "ErrorCodes.h"

template <typename T>
std::shared_ptr<T> cloneSharedPtr(const std::shared_ptr<T>& source) {
	return source ? std::make_shared<T>(*source) : nullptr;
}

template <typename... Types>
std::variant<std::shared_ptr<Types>...> cloneVariant(const std::variant<std::shared_ptr<Types>...>& source) {
	using VariantType = std::variant<std::shared_ptr<Types>...>;

	return std::visit([](const auto& ptr) -> VariantType {
		return VariantType(cloneSharedPtr(ptr));
		}, source);
}

template <typename CoreTypesMarshaller, typename... CoreTypes>
class LRUCacheObject
{
public:
	typedef std::tuple<CoreTypes...> ObjectCoreTypes;

private:
	typedef std::variant<std::shared_ptr<CoreTypes>...> CoreTypesWrapper;
	typedef std::shared_ptr<std::variant<std::shared_ptr<CoreTypes>...>> CoreTypesWrapperPtr;

	

public:
	bool dirty;
	CoreTypesWrapperPtr data;
	mutable std::shared_mutex mutex;

public:
	template<class Type>
	LRUCacheObject(std::shared_ptr<Type> ptrCoreObject)
		: dirty(true)
	{
		data = std::make_shared<CoreTypesWrapper>(ptrCoreObject);
	}

	//template <typename Type>
	LRUCacheObject(const LRUCacheObject& source)
		: dirty(true)
	{
		data = std::make_shared<CoreTypesWrapper>(cloneVariant(*source.data));
	}

	LRUCacheObject(std::fstream& is)
		: dirty(true)
	{
		CoreTypesMarshaller::template deserialize<CoreTypesWrapper, CoreTypes...>(is, data);
	}

	LRUCacheObject(const char* szBuffer)
		: dirty(true)
	{
		CoreTypesMarshaller::template deserialize<CoreTypesWrapper, CoreTypes...>(szBuffer, data);
	}

	inline void serialize(std::fstream& os, uint8_t& uidObjectType, size_t& nBufferSize)
	{
		CoreTypesMarshaller::template serialize<CoreTypes...>(os, *data, uidObjectType, nBufferSize);
	}

	inline void serialize(char*& szBuffer, uint8_t& uidObjectType, size_t& nBufferSize)
	{
		CoreTypesMarshaller::template serialize<CoreTypes...>(szBuffer, *data, uidObjectType, nBufferSize);
	}
};