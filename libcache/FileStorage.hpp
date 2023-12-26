#pragma once
#include <memory>
#include <iostream>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <variant>
#include <cmath>

#include "ErrorCodes.h"
#include "IFlushCallback.h"

//#define __CONCURRENT__
#define __POSITION_AWARE_ITEMS__

template<
	typename ICallback,
	typename ObjectUIDType, 
	template <typename, typename...> typename ObjectType, 
	typename CoreTypesMarshaller, 
	typename... ObjectCoreTypes
>
class FileStorage
{
	typedef FileStorage<ICallback, ObjectUIDType, ObjectType, CoreTypesMarshaller, ObjectCoreTypes...> SelfType;

public:
	typedef ObjectUIDType ObjectUIDType;
	typedef ObjectType<CoreTypesMarshaller, ObjectCoreTypes...> ObjectType;

private:
	size_t m_nFileSize;
	size_t m_nBlockSize;

	std::string m_stFilename;
	std::fstream m_fsStorage;

	size_t m_nNextBlock;
	std::vector<bool> m_vtAllocationTable;

	ICallback* m_ptrCallback;

#ifdef __CONCURRENT__
	bool m_bStopFlush;
	std::thread m_threadBatchFlush;

	mutable std::shared_mutex m_mtxCache;

	std::unordered_map<ObjectUIDType, std::shared_ptr<ObjectType>> m_mpObjects;
#endif __CONCURRENT__

public:
	~FileStorage()
	{
#ifdef __CONCURRENT__
		m_bStopFlush = true;
		m_threadBatchFlush.join();

		m_mpObjects.clear();
#endif __CONCURRENT__
	}

	FileStorage(size_t nBlockSize, size_t nFileSize, std::string stFilename)
		: m_nFileSize(nFileSize)
		, m_nBlockSize(nBlockSize)
		, m_stFilename(stFilename)
		, m_nNextBlock(0)
	{
		m_vtAllocationTable.resize(nFileSize/nBlockSize, false);

		//m_fsStorage.rdbuf()->pubsetbuf(0, 0);
		m_fsStorage.open(stFilename.c_str(), std::ios::binary | std::ios::in | std::ios::out);
		
		if (!m_fsStorage.is_open())
		{
			throw new exception("should not occur!");   // TODO: critical log.
		}

#ifdef __CONCURRENT__
		m_bStopFlush = false;
		m_threadBatchFlush = std::thread(handlerBatchFlush, this);
#endif __CONCURRENT__
	}

	template <typename... InitArgs>
	CacheErrorCode init(ICallback* ptrCallback, InitArgs... args)
	{
		m_ptrCallback = ptrCallback;// getNthElement<0>(args...);
		return CacheErrorCode::Success;
	}

	std::shared_ptr<ObjectType> getObject(ObjectUIDType uidObject)
	{
#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex> lock_file_storage(m_mtxCache);

		if (m_mpObjects.find(uidObject) == m_mpObjects.end())
		{
			throw new std::exception("should not occur!");
		}

		std::shared_ptr<ObjectType> ptrObject = m_mpObjects[uidObject];
		m_mpObjects.erase(uidObject);

		return ptrObject;

		lock_file_storage.unlock();
#endif __CONCURRENT__

		m_fsStorage.seekg(uidObject.m_uid.FATPOINTER.m_ptrFile.m_nOffset);

		std::shared_ptr<ObjectType> ptrObject = std::make_shared<ObjectType>(m_fsStorage);
		ptrObject->dirty = false;

		return ptrObject;
	}

	CacheErrorCode remove(ObjectUIDType ptrKey)
	{
		return CacheErrorCode::KeyDoesNotExist;
	}

#ifdef __POSITION_AWARE_ITEMS__
	CacheErrorCode addObject(ObjectUIDType uidObject, std::shared_ptr<ObjectType> ptrObject, std::optional<ObjectUIDType>& uidParent)
#else
	CacheErrorCode addObject(ObjectUIDType uidObject, std::shared_ptr<ObjectType> ptrObject)
#endif __POSITION_AWARE_ITEMS__
	{
#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex> lock_file_storage(m_mtxCache);

		//if (m_mpObjects.find(ptrKey) != m_mpObjects.end())
		//{
		//	throw new std::exception("should not occur!");
		//}

		m_mpObjects[uidObject] = ptrObject;

		return CacheErrorCode::Success;
#else __CONCURRENT__

		size_t nBufferSize = 0;
		uint8_t uidObjectType = 0;

		m_fsStorage.seekp(m_nNextBlock * m_nBlockSize);
		ptrObject->serialize(m_fsStorage, uidObjectType, nBufferSize);

		size_t nRequiredBlocks = std::ceil( (nBufferSize + sizeof(uint8_t)) / (float)m_nBlockSize);

		ObjectUIDType uidUpdated = ObjectUIDType::createAddressFromFileOffset(m_nNextBlock * m_nBlockSize, nBufferSize + sizeof(uint8_t));

		for (int idx = 0; idx < nRequiredBlocks; idx++)
		{
			m_vtAllocationTable[m_nNextBlock++] = true;
		}
	
		m_fsStorage.flush();

#ifdef __POSITION_AWARE_ITEMS__
		m_ptrCallback->updateChildUID(uidParent, uidObject, uidUpdated);
#endif __POSITION_AWARE_ITEMS__

#endif __CONCURRENT__

		return CacheErrorCode::Success;
	}

#ifdef __CONCURRENT__
	void performBatchFlush()
	{
		std::unordered_map<ObjectUIDType, ObjectUIDType> mpUpdatedUIDs;
#ifdef __CONCURRENT__
		std::unique_lock<std::shared_mutex> lock_file_storage(m_mtxCache);
#endif __CONCURRENT__

		auto it = m_mpObjects.begin();
		while (it != m_mpObjects.end())
		{
			std::tuple<uint8_t, const std::byte*, size_t> tpSerializedData = it->second->serialize();

			m_fsStorage.seekp(m_nNextBlock * m_nBlockSize);
			m_fsStorage.write((char*)(&std::get<0>(tpSerializedData)), sizeof(uint8_t));
			m_fsStorage.write((char*)(std::get<1>(tpSerializedData)), std::get<2>(tpSerializedData));

			size_t nBlockRequired = std::ceil(std::get<2>(tpSerializedData) / (float)m_nBlockSize);

			ObjectUIDType uid = ObjectUIDType::createAddressFromFileOffset(m_nBlockSize, nBlockRequired * m_nBlockSize);
			mpUpdatedUIDs[it->first] = uid;

			for (int idx = 0; idx < nBlockRequired; idx++)
			{
				m_vtAllocationTable[m_nNextBlock++] = true;
			}
		}
		m_fsStorage.flush();

		m_ptrCallback->keysUpdate(mpUpdatedUIDs);
	}

	static void handlerBatchFlush(SelfType* ptrSelf)
	{
		do
		{
			ptrSelf->performBatchFlush();

			std::this_thread::sleep_for(100ms);

		} while (!ptrSelf->m_bStopFlush);
	}
#endif __CONCURRENT__
};

