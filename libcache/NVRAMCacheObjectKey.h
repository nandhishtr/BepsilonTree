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

	//bool operator <(const NVRAMObjectKey& rhs) const
	//{
	//	return this->m_nPointerValue < rhs.m_nPointerValue;
	//}

	//bool operator<(const std::shared_ptr<NVRAMObjectKey>& rhs) const
	////bool operator<(const std::shared_ptr<NVRAMObjectKey>& rhs) const
	//{
	//	return this->m_nPointerValue < (*rhs.get()).m_nPointerValue;
	//}

	//bool operator() (const NVRAMObjectKey& lhs, const NVRAMObjectKey& rhs) const
	//{
	//	return lhs.m_nPointerValue < rhs.m_nPointerValue;
	//}

	//bool operator==(const NVRAMObjectKey& rhs) const
	//{
	//	return this->m_nPointerValue == rhs.m_nPointerValue;
	//}


	//bool operator()(const std::shared_ptr<NVRAMObjectKey>& lhs, const std::shared_ptr<NVRAMObjectKey>& rhs) const {
	//	return lhs.get() == rhs.get();
	//}

	//template<typename T = NVRAMObjectKey>
	//std::size_t operator()(const std::shared_ptr < T>& key) const
	//{
	//	using std::hash;

	//	return hash<uintptr_t>()((*key.get()).m_nPointerValue);
	//}

	//template<typename T = NVRAMObjectKey>
	//std::size_t operator()(const NVRAMObjectKey& key) const
	//{
	//	using std::hash;

	//	return hash<uintptr_t>()(m_nPointerValue);
	//}

	//std::size_t operator()(const std::shared_ptr<NVRAMCacheObjectKey>& ptr) const;
	//bool operator()(const std::shared_ptr<NVRAMCacheObjectKey>& lhs, const std::shared_ptr<NVRAMCacheObjectKey>& rhs) const;
};
