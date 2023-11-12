#pragma once
#include <memory>
#include "DRAMCacheObjectKey.h"
#include "NVRAMCacheObjectKey.h"

template<typename T>
struct SharedPtrHash
{
	std::size_t operator()(const std::shared_ptr<uintptr_t>& ptr) const {
		return  std::hash<uintptr_t>()(*(ptr.get()));
	}

	std::size_t operator()(const std::shared_ptr<DRAMCacheObjectKey>& ptr) const {
		using std::hash;

		return hash<uintptr_t>()((*ptr.get()).m_nPointerValue);
	}

	std::size_t operator()(const std::shared_ptr<NVRAMCacheObjectKey>& ptr) const {
		using std::hash;

		return hash<uintptr_t>()((*ptr.get()).m_nPointerValue);
	}
};

template<typename T>
struct SharedPtrEqual {
	bool operator()(const std::shared_ptr<uintptr_t>& lhs, const std::shared_ptr<uintptr_t>& rhs) const {
		return *lhs.get() == *rhs.get();
	}

	bool operator()(const std::shared_ptr<DRAMCacheObjectKey>& lhs, const std::shared_ptr<DRAMCacheObjectKey>& rhs) const {
		return lhs->m_nPointerValue == rhs->m_nPointerValue;
	}

	bool operator()(const std::shared_ptr<NVRAMCacheObjectKey>& lhs, const std::shared_ptr<NVRAMCacheObjectKey>& rhs) const {
		return lhs->m_nPointerValue == rhs->m_nPointerValue;
	}
};

