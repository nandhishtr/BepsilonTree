#pragma once
#include "ICacheObject.h"
#include "NVRAMCacheObjectKey.h"

class NVRAMCacheObject : public ICacheObject
{
public:
	NVRAMCacheObject()
	{

	}


	//explicit operator NVRAMCacheObjectKey* () const
	//{
	//	return  new NVRAMCacheObjectKey((void*)this);
	//}

public:
	//template <typename KeyType>
	//KeyType* getKey()
	//{
	//	KeyType* ptrKey = new KeyType();
	//	generateKey(ptrKey);

	//	return ptrKey;
	//}

	//template <typename KeyType>
	//inline void generateKey(KeyType*& key);
};

