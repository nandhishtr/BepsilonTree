#pragma once
#include <memory>
#include <unordered_map>

#include "ErrorCodes.h"

//#define __CONCURRENT__

template<
	typename ICallback,
	typename ObjectUIDType_, 
	template <typename, typename...> typename ObjectType_, 
	typename CoreTypesMarshaller, 
	typename... ObjectCoreTypes>
class VolatileStorage
{
	typedef VolatileStorage<ICallback, ObjectUIDType_, ObjectType_, CoreTypesMarshaller, ObjectCoreTypes...> SelfType;

public:
	typedef ObjectUIDType_ ObjectUIDType;
	typedef ObjectType_<CoreTypesMarshaller, ObjectCoreTypes...> ObjectType;

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

	std::shared_ptr<ObjectType> getObject(const ObjectUIDType uidObject)
	{
		std::cout << "((" << m_mpObject.size() << " ))--" << uidObject.toString().c_str() << ",,," ;
		auto it = m_mpObject.begin();
		while (it != m_mpObject.end()) {
			std::cout << (*it).first.toString().c_str() << "|";
			it++;

		}
		std::cout << std::endl;

#ifdef __CONCURRENT__
		std::shared_lock<std::shared_mutex> lock_file_storage(m_mtxStorage);
#endif __CONCURRENT__

		std::shared_ptr<ObjectType> ptrObject = nullptr;
		if (m_mpObject.find(uidObject) != m_mpObject.end())
		{
			std::cout << "(( found ))";
			ptrObject = m_mpObject[uidObject];
			//m_mpObject.erase(uidObject);	// since it is volatile cache.. add each time.. if this is set then 'dirty' should be false.

			ptrObject->dirty = false; //if this is set then dont erase the object! here.. technically object should be erased here...
		}

		return ptrObject;
	}

	CacheErrorCode remove(const ObjectUIDType& uidObject)
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

	CacheErrorCode addObject(const ObjectUIDType& uidObject, std::shared_ptr<ObjectType> ptrValue, ObjectUIDType& uidUpdated)
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

		m_mpObject[uidUpdated] = std::make_shared<ObjectType>(*ptrValue);

		return CacheErrorCode::Success;
	}

	inline size_t getWritePos()
	{
		return m_nCounter;
	}

	inline size_t getBlockSize()
	{
		return UINT32_MAX;
	}

	inline ObjectUIDType::Media getMediaType()
	{
		return ObjectUIDType::DRAM;
	}

	CacheErrorCode addObjects(std::vector<std::pair<ObjectUIDType, std::pair<std::optional<ObjectUIDType>, std::shared_ptr<ObjectType>>>>& vtObjects, size_t nNewOffset)
	{
#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex> lock_file_storage(m_mtxStorage);
#endif __CONCURRENT__

		m_nCounter = nNewOffset;

		auto it = vtObjects.begin();
		while (it != vtObjects.end())
		{
			
			if (m_mpObject.find(*(*it).second.first) != m_mpObject.end())
			{
				throw new std::logic_error("should not occur!");
			}

			m_mpObject[*(*it).second.first] = std::make_shared<ObjectType>(*(*it).second.second);

			it++;
		}

		return CacheErrorCode::Success;
	}
};

