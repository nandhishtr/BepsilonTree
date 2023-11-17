#pragma once
#include <iostream>
#include <unordered_map>

#include "ICache.h"

template <typename ObjectType>
class NoCache : public ICoreCache
{
public:
	typedef ObjectType KeyType;

public:
	~NoCache()
	{
	}

	NoCache()
	{
	}

	CacheErrorCode remove(ObjectType objKey)
	{
		throw std::invalid_argument("missing functionality..");
		return CacheErrorCode::KeyDoesNotExist;
	}

	template<class Type>
	std::shared_ptr<Type> getObjectOfType(ObjectType objKey)
	{
		return std::static_pointer_cast<Type>(objKey);
	}

	template<class Type, typename... ArgsType>
	ObjectType createObjectOfType(ArgsType ... args)
	{
		return std::make_shared<Type>(args...);
	}
};