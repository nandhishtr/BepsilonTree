#pragma once
#include <memory>
#include "IDRAMCacheObject.h"


template <typename CoreObjectType = IDRAMCacheObject>
class DRAMCacheObject : public ICacheObject
{
public:
	std::shared_ptr<CoreObjectType> m_ptrCoreObject;

public:
	DRAMCacheObject(std::shared_ptr<CoreObjectType> ptrObj)
		: m_ptrCoreObject(ptrObj)
	{

	}
};

