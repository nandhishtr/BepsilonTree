#pragma once
#include "ICacheObjectKey.h"
#include <cstdint>

class NVRAMCacheObjectKey : public ICacheObjectKey
{
public:
	uintptr_t m_nPointerValue;

public:
	NVRAMCacheObjectKey()
		: m_nPointerValue(NULL)
	{
	}

	NVRAMCacheObjectKey(void* ptr)
	{
		m_nPointerValue = reinterpret_cast<uintptr_t>(ptr);
	}
};
