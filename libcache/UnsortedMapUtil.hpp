#pragma once
#include <memory>
#include "ObjectFatUID.h"

template<typename T>
struct SharedPtrHash
{
	std::size_t operator()(const std::shared_ptr<uintptr_t>& ptr) const {
		return  std::hash<uintptr_t>()(*(ptr.get()));
	}

	std::size_t operator()(const ObjectFatUID& rhs) const
	{
		size_t hashValue = 0;
		hashValue ^= std::hash<uint8_t>()(rhs.m_uid.m_nMediaType);

		switch (rhs.m_uid.m_nMediaType)
		{
		case ObjectFatUID::Media::Volatile:
			hashValue ^= std::hash<uintptr_t>()(rhs.m_uid.FATPOINTER.m_ptrVolatile);
			break;
		case ObjectFatUID::Media::File:
			size_t offsetHash = std::hash<uint32_t>()(rhs.m_uid.FATPOINTER.m_ptrFile.m_nOffset);
			size_t sizeHash = std::hash<uint32_t>()(rhs.m_uid.FATPOINTER.m_ptrFile.m_nSize);
			hashValue ^= offsetHash ^ (sizeHash + 0x9e3779b9 + (offsetHash << 6) + (offsetHash >> 2));
			break;
		}

		return hashValue;
	}

};

template<typename T>
struct SharedPtrEqual {
	bool operator()(const std::shared_ptr<uintptr_t>& lhs, const std::shared_ptr<uintptr_t>& rhs) const {
		return *lhs.get() == *rhs.get();
	}

	bool operator()(const ObjectFatUID& lhs, const ObjectFatUID& rhs) const
	{
		return memcmp(&lhs.m_uid, &rhs.m_uid, sizeof(NodeUID)) == 0;
	}

};

