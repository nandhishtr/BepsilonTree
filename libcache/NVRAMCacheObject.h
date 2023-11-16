#pragma once
#include "IDRAMCacheObject.h"
#include "INVRAMCacheObject.h"
#include "IBufferCacheObject.h"
#include <iostream>

template <typename CoreDRAMObjectType = IDRAMCacheObject, typename CoreNVRAMObjectType = INVRAMCacheObject, typename CoreBufferObjectType = IBufferCacheObject>
class NVRAMCacheObjectEx : public ICacheObject
{
public:
	std::shared_ptr<CoreDRAMObjectType> m_ptrCoreDRAMObject;
	std::shared_ptr<CoreNVRAMObjectType> m_ptrCoreNVRAMObject;
	std::shared_ptr<CoreBufferObjectType> m_ptrCoreBufferObject;
public:
	NVRAMCacheObjectEx(std::shared_ptr<CoreDRAMObjectType> ptrDRAMObj, std::shared_ptr<CoreNVRAMObjectType> ptrNVRAMObj, std::shared_ptr<CoreBufferObjectType> ptrBufferObj)
		: m_ptrCoreDRAMObject(ptrDRAMObj)
		, m_ptrCoreNVRAMObject(ptrNVRAMObj)
		, m_ptrCoreBufferObject(ptrBufferObj)
	{

	}
};


template <typename CoreObjectType>
class NVRAMCacheObject : public ICacheObject
{
public:
	std::shared_ptr<CoreObjectType> m_ptrCoreObject;

public:
	NVRAMCacheObject(std::shared_ptr<CoreObjectType> ptrObj)
		: m_ptrCoreObject(ptrObj)
	{
		//const std::size_t n = sizeof...(CoreObjectType);
		std::cout << typeid(CoreObjectType).name() << std::endl;
	}
};

template <typename CoreObjectType1, typename CoreObjectType2>
class NVRAMCacheObject2 : public ICacheObject
{
public:
	std::shared_ptr<CoreObjectType1> m_ptrCoreObject;
	std::shared_ptr<CoreObjectType2> m_ptrCoreObject2;

public:
	NVRAMCacheObject2(std::shared_ptr<CoreObjectType1> ptrObj)
		: m_ptrCoreObject(ptrObj)
	{
		//const std::size_t n = sizeof...(CoreObjectType);
		std::cout << typeid(CoreObjectType1).name() << std::endl;
		std::cout << typeid(CoreObjectType2).name() << std::endl;
	}
};