#pragma once
#include <memory>

template<typename T>
struct SharedPtrHash
{
	std::size_t operator()(const std::shared_ptr<T>& ptr) const;
};


template<typename T>
struct SharedPtrEqual {
	bool operator()(const std::shared_ptr<T>& lhs, const std::shared_ptr<T>& rhs) const;
};

