#pragma once
#include <iostream>
#include <unordered_map>

#include "ICache.h"
#include <variant>
#include <typeinfo>


template <typename... ValueCoreTypes>
class NoCacheObject
{
public:
	std::variant<std::shared_ptr<ValueCoreTypes...>> m_objData;

public:
	NoCacheObject(ValueCoreTypes... args)
		: m_objData(args)
	{
	}

	template <typename CoreValueType>
	void setData(CoreValueType objData)
	{
		m_objData = objData;
	}
};


template<typename KeyType, typename... ValueCoreTypes>
class NoCache : public ICoreCache
{
public:
	typedef KeyType KeyType;

	typedef std::variant<ValueCoreTypes...>* CacheValueType;

private:
	typedef std::variant<ValueCoreTypes...>* CacheValueTypePtr;

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

		if (std::holds_alternative<Type>(*ptrValue))
		{
			return std::get<Type>(*ptrValue);
		}

		return nullptr;
	}

	template<class Type, typename... ArgsType>
	KeyType createObjectOfType(ArgsType ... args)
	{
		CacheValueTypePtr ptrValue = new std::variant<ValueCoreTypes...>(std::make_shared<Type>(args...));
		
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