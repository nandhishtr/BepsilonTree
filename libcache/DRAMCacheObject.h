#pragma once
#include <memory>

#include "ICacheObject.h"

class IDRAMCacheObject
{
	// interface
};

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

