#pragma once
#include <memory>
#include <unordered_map>

#include "UnsortedMapUtil.h"
#include "ErrorCodes.h"

template<typename KeyType, template <typename, typename...> typename ValueType, typename TypeMarshaller, typename... ValueCoreTypes>
class VolatileStorage
{
	typedef ValueType<TypeMarshaller, ValueCoreTypes...> StorageValueType;

private:
	size_t m_nPoolSize;
	std::unordered_map<KeyType, std::shared_ptr<StorageValueType>> m_mpObject;

public:
	VolatileStorage(size_t nPoolSize)
		: m_nPoolSize(nPoolSize)
	{
	}

	std::shared_ptr<StorageValueType> getObject(KeyType ptrKey)
	{
		if (m_mpObject.find(ptrKey) != m_mpObject.end())
		{
			return m_mpObject[ptrKey];
		}

		return nullptr;
	}

	CacheErrorCode addObject(KeyType ptrKey, std::shared_ptr<StorageValueType> ptrValue)
	{
		if (m_mpObject.size() >= m_nPoolSize)
		{
			return CacheErrorCode::Error;
		}

		m_mpObject[ptrKey] = ptrValue;

		return CacheErrorCode::Success;
	}
};

