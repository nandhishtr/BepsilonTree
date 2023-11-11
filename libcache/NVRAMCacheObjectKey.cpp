#include "pch.h"
#include "NVRAMCacheObjectKey.h"

//bool NVRAMCacheObjectKey::operator()(const std::shared_ptr<NVRAMCacheObjectKey>& lhs, const std::shared_ptr<NVRAMCacheObjectKey>& rhs) const {
//	return lhs->m_nPointerValue == rhs->m_nPointerValue;
//}
//
//std::size_t NVRAMCacheObjectKey::operator()(const std::shared_ptr<NVRAMCacheObjectKey>& ptr) const {
//	using std::hash;
//
//	return hash<uintptr_t>()((*ptr.get()).m_nPointerValue);
//}
