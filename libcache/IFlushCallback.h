#pragma once
#include "CacheErrorCodes.h"

template <typename ObjectUIDType>
class IFlushCallback
{
public:
	virtual CacheErrorCode keyUpdate(ObjectUIDType uidObject) = 0;
};