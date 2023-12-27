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

#define __POSITION_AWARE_ITEMS__

template <typename CoreTypesMarshaller, typename... CoreTypes>
class LRUCacheObject
{
public:
	typedef std::tuple<CoreTypes...> ObjectCoreTypes;

private:
	typedef std::variant<std::shared_ptr<CoreTypes>...> CoreTypesWrapper;
	typedef std::shared_ptr<std::variant<std::shared_ptr<CoreTypes>...>> CoreTypesWrapperPtr;

public:
#ifdef __POSITION_AWARE_ITEMS__
	bool dirty;
#endif __POSITION_AWARE_ITEMS__
	CoreTypesWrapperPtr data;
	mutable std::shared_mutex mutex;

public:
	LRUCacheObject(CoreTypesWrapperPtr ptrObject)
	{
#ifdef __POSITION_AWARE_ITEMS__
		dirty = true;
#endif __POSITION_AWARE_ITEMS__
		data = ptrObject;
	}

	LRUCacheObject(std::fstream& is)
	{
#ifdef __POSITION_AWARE_ITEMS__
		dirty = true;
#endif __POSITION_AWARE_ITEMS__
		CoreTypesMarshaller::template deserialize<CoreTypesWrapper, CoreTypes...>(is, data);
	}

	LRUCacheObject(const char* szBuffer)
	{
#ifdef __POSITION_AWARE_ITEMS__
		dirty = true;
#endif __POSITION_AWARE_ITEMS__
		CoreTypesMarshaller::template deserialize<CoreTypesWrapper, CoreTypes...>(szBuffer, data);
	}

	template<class Type, typename... ArgsType>
	static std::shared_ptr<LRUCacheObject> createObjectOfType(ArgsType... args)
	{
		CoreTypesWrapperPtr ptrValue = std::make_shared<CoreTypesWrapper>(std::make_shared<Type>(args...));

		return std::make_shared<LRUCacheObject>(ptrValue);
	}

	template<class Type, typename... ArgsType>
	static std::shared_ptr<LRUCacheObject> createObjectOfType(std::shared_ptr<Type>& ptrCoreObject, ArgsType... args)
	{
		ptrCoreObject = std::make_shared<Type>(args...);
		CoreTypesWrapperPtr ptrValue = std::make_shared<CoreTypesWrapper>(ptrCoreObject);

		return std::make_shared<LRUCacheObject>(ptrValue);
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