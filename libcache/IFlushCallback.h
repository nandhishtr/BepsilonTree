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
	virtual CacheErrorCode keyUpdate(std::optional<ObjectUIDType>& uidParent, ObjectUIDType uidOld, ObjectUIDType uidNew) = 0;
	virtual CacheErrorCode keysUpdate(std::vector<std::pair<ObjectUIDType, std::pair<ObjectUIDType, ObjectUIDType>>> vtUpdatedUIDs) = 0;
};