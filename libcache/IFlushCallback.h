#pragma once
#include <unordered_map>
#include "CacheErrorCodes.h"

template <typename ObjectUIDType>
class IFlushCallback
{
protected:
	mutable std::shared_mutex m_mtxUIDsUpdate;
	std::unordered_map<ObjectUIDType, ObjectUIDType> m_mpUIDsUpdate;

public:
	virtual CacheErrorCode updateChildUID(std::optional<ObjectUIDType>& uidObject, ObjectUIDType uidChildOld, ObjectUIDType uidChildNew) = 0;
	virtual CacheErrorCode updateChildUID(std::vector<std::pair<ObjectUIDType, std::pair<ObjectUIDType, ObjectUIDType>>> vtUpdatedUIDs) = 0;
};