#pragma once
#include "IObjectUID.h"
#include <cstdint>

class DRAMObjectUID : public IObjectUID
{
public:
	uintptr_t m_nPointerValue;

public:
	DRAMObjectUID()
		: m_nPointerValue(NULL)
	{
	}

	DRAMObjectUID(void* ptr)
	{
		m_nPointerValue = reinterpret_cast<uintptr_t>(ptr);
	}
};
