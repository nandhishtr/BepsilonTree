#include "pch.h"
#include "UnsortedMapUtil.h"
#include "DRAMCacheObjectKey.h"


bool SharedPtrEqual<uintptr_t>::operator()(const std::shared_ptr<uintptr_t>& lhs, const std::shared_ptr<uintptr_t>& rhs) const {
	return *lhs.get() == *rhs.get();
}
std::size_t SharedPtrHash<uintptr_t>::operator()(const std::shared_ptr<uintptr_t>& ptr) const {
	return  std::hash<uintptr_t>()(*(ptr.get()));
}

bool SharedPtrEqual<DRAMCacheObjectKey>::operator()(const std::shared_ptr<DRAMCacheObjectKey>& lhs, const std::shared_ptr<DRAMCacheObjectKey>& rhs) const {
	return lhs->m_nPointerValue == rhs->m_nPointerValue;
}

std::size_t SharedPtrHash<DRAMCacheObjectKey>::operator()(const std::shared_ptr<DRAMCacheObjectKey>& ptr) const {
	using std::hash;

	return hash<uintptr_t>()((*ptr.get()).m_nPointerValue);
}