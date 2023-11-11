#pragma once
//#include "DRAM.h"
//
//template <typename KeyType, typename ValueType>
//class LocalDRAMCachw
//{
//public:
//	template <class Object>
//	IObjectKey* createNewObject()
//	{
//		ICacheObject* obj = new Object();
//		IObjectKey* objKey = NULL;
//
//		if (this->add(obj, objKey) != CacheErrorCode::Success)
//		{
//			delete obj;
//			throw std::bad_alloc;
//		}
//
//		return objKey;
//	}
//};
//
