#pragma once
#include "ICacheObject.h"
#include "DRAMCacheObjectKey.h"

class DRAMCacheObject : public ICacheObject
{
public:
	DRAMCacheObject()
	{

	}

	explicit operator DRAMCacheObjectKey* () const
	{
		return  new DRAMCacheObjectKey((void*)this);
	}

public:
	
};



