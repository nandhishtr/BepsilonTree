#pragma once
#include <memory>
#include <unordered_map>

#include "ErrorCodes.h"

//#define __CONCURRENT__

template<
	typename ICallback,
	typename ObjectUIDType, 
	template <typename, typename...> typename ObjectType, 
	typename CoreTypesMarshaller, 
	typename... ObjectCoreTypes>
class VolatileStorage
{
	typedef VolatileStorage<ICallback, ObjectUIDType, ObjectType, CoreTypesMarshaller, ObjectCoreTypes...> SelfType;

public:
	typedef ObjectUIDType ObjectUIDType;
	typedef ObjectType<CoreTypesMarshaller, ObjectCoreTypes...> ObjectType;

private:
	size_t m_nCounter;
	mutable std::shared_mutex m_mtxStorage;

	size_t m_nPoolSize;
	std::unordered_map<ObjectUIDType, std::shared_ptr<ObjectType>> m_mpObject;

public:
	VolatileStorage(size_t nPoolSize)
		: m_nCounter(0)
		, m_nPoolSize(nPoolSize)
	{
	}

	template <typename... InitArgs>
	CacheErrorCode init(InitArgs... args)
	{
		return CacheErrorCode::Success;
	}

	std::shared_ptr<ObjectType> getObject(ObjectUIDType uidObject)
	{
#ifdef __CONCURRENT__
		std::shared_lock<std::shared_mutex> lock_file_storage(m_mtxStorage);
#endif __CONCURRENT__

		std::shared_ptr<ObjectType> ptrObject = nullptr;
		if (m_mpObject.find(uidObject) != m_mpObject.end())
		{
			ptrObject = m_mpObject[uidObject];
			m_mpObject.erase(uidObject);
		}

		return ptrObject;
	}

	CacheErrorCode remove(ObjectUIDType uidObject)
	{
#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex> lock_file_storage(m_mtxStorage);
#endif __CONCURRENT__

		auto it = m_mpObject.find(uidObject);
		if (it != m_mpObject.end())
		{
			m_mpObject.erase((*it).first);
			return CacheErrorCode::Success;
		}

		return CacheErrorCode::KeyDoesNotExist;
	}

	CacheErrorCode addObject(ObjectUIDType uidObject, std::shared_ptr<ObjectType> ptrValue, ObjectUIDType& uidUpdated)
	{
#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex> lock_file_storage(m_mtxStorage);
#endif __CONCURRENT__

		if (m_mpObject.size() >= m_nPoolSize)
		{
			return CacheErrorCode::Error;
		}

		uidUpdated = ObjectUIDType::createAddressFromDRAMCacheCounter(m_nCounter);

		m_nCounter++;

		m_mpObject[uidUpdated] = ptrValue;

		return CacheErrorCode::Success;
	}
};

