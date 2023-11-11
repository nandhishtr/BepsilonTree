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
	static void get(uintptr_t& key, std::shared_ptr<DRAMCacheObject> ptrObj)
	{
		key = reinterpret_cast<uintptr_t>(&(*ptrObj.get()));
	}

	static void get(DRAMCacheObjectKey& key, std::shared_ptr<DRAMCacheObject> ptrObj)
	{
		key = DRAMCacheObjectKey(&(*ptrObj.get()));
	}
};

