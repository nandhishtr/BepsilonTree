#include "pch.h"
#include "DRAMCacheObjectKey.h"

//bool DRAMCacheObjectKey::operator()(const std::shared_ptr<DRAMCacheObjectKey>& lhs, const std::shared_ptr<DRAMCacheObjectKey>& rhs) const {
//	return lhs->m_nPointerValue == rhs->m_nPointerValue;
//}
//
//std::size_t DRAMCacheObjectKey::operator()(const std::shared_ptr<DRAMCacheObjectKey>& ptr) const {
//	using std::hash;
//
//	return hash<uintptr_t>()((*ptr.get()).m_nPointerValue);
//}
