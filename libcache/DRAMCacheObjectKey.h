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

	//bool operator <(const DRAMObjectKey& rhs) const
	//{
	//	return this->m_nPointerValue < rhs.m_nPointerValue;
	//}

	//bool operator<(const std::shared_ptr<DRAMObjectKey>& rhs) const
	////bool operator<(const std::shared_ptr<DRAMObjectKey>& rhs) const
	//{
	//	return this->m_nPointerValue < (*rhs.get()).m_nPointerValue;
	//}

	//bool operator() (const DRAMObjectKey& lhs, const DRAMObjectKey& rhs) const
	//{
	//	return lhs.m_nPointerValue < rhs.m_nPointerValue;
	//}

	//bool operator==(const DRAMObjectKey& rhs) const
	//{
	//	return this->m_nPointerValue == rhs.m_nPointerValue;
	//}


	//bool operator()(const std::shared_ptr<DRAMObjectKey>& lhs, const std::shared_ptr<DRAMObjectKey>& rhs) const {
	//	return lhs.get() == rhs.get();
	//}

	//template<typename T = DRAMObjectKey>
	//std::size_t operator()(const std::shared_ptr < T>& key) const
	//{
	//	using std::hash;

	//	return hash<uintptr_t>()((*key.get()).m_nPointerValue);
	//}

	//template<typename T = DRAMObjectKey>
	//std::size_t operator()(const DRAMObjectKey& key) const
	//{
	//	using std::hash;

	//	return hash<uintptr_t>()(m_nPointerValue);
	//}

	//std::size_t operator()(const std::shared_ptr<DRAMCacheObjectKey>& ptr) const;
	//bool operator()(const std::shared_ptr<DRAMCacheObjectKey>& lhs, const std::shared_ptr<DRAMCacheObjectKey>& rhs) const;
};
