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
	LRUCacheObject(CoreTypesWrapperPtr ptrObject)
	{
		dirty = true;
		data = ptrObject;
	}

	LRUCacheObject(std::fstream& is)
	{
		dirty = true;
		CoreTypesMarshaller::template deserialize<CoreTypesWrapper, CoreTypes...>(is, data);
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
};