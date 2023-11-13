#pragma once
#include "ICacheObject.h"

class INVRAMCacheObject
{
	// interface
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
