#pragma once
#include <memory>

template<typename T>
struct SharedPtrHash
{
	std::size_t operator()(const std::shared_ptr<uintptr_t>& ptr) const {
		return  std::hash<uintptr_t>()(*(ptr.get()));
	}
};

template<typename T>
struct SharedPtrEqual {
	bool operator()(const std::shared_ptr<uintptr_t>& lhs, const std::shared_ptr<uintptr_t>& rhs) const {
		return *lhs.get() == *rhs.get();
	}
};

