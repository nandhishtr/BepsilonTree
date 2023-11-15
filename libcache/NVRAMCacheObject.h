#pragma once
#include "IDRAMCacheObject.h"
#include "INVRAMCacheObject.h"
#include "IBufferCacheObject.h"


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


template <typename CoreObjectType = INVRAMCacheObject>
class NVRAMCacheObject : public ICacheObject
{
public:
	std::shared_ptr<CoreObjectType> m_ptrCoreObject;

public:
	NVRAMCacheObject(std::shared_ptr<CoreObjectType> ptrObj)
		: m_ptrCoreObject(ptrObj)
	{

	}
};
