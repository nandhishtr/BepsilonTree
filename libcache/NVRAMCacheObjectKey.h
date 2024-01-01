#pragma once
#include "IObjectUID.h"
#include <cstdint>

class NVRAMObjectUID : public IObjectUID
{
public:
	uintptr_t m_nPointerValue;

public:
	NVRAMObjectUID()
		: m_nPointerValue(NULL)
	{
	}

	NVRAMObjectUID(void* ptr)
	{
		m_nPointerValue = reinterpret_cast<uintptr_t>(ptr);
	}
};
