#pragma once
#include "ICacheObjectKey.h"
#include <cstdint>

class DRAMCacheObjectKey : public ICacheObjectKey
{
public:
	uintptr_t m_nPointerValue;

public:
	DRAMCacheObjectKey()
		: m_nPointerValue(NULL)
	{
	}

	DRAMCacheObjectKey(void* ptr)
	{
		m_nPointerValue = reinterpret_cast<uintptr_t>(ptr);
	}
};
