#pragma once
#include <unordered_map>
#include "CacheErrorCodes.h"

template <typename ObjectUIDType, typename ObjectType>
class IFlushCallback
{
protected:
	mutable std::shared_mutex m_mtxUIDsUpdate;
	std::unordered_map<ObjectUIDType, ObjectUIDType> m_mpUIDsUpdate;

public:
	virtual CacheErrorCode updateChildUID(const std::optional<ObjectUIDType>& uidObject, const ObjectUIDType& uidChildOld, const ObjectUIDType& uidChildNew) = 0;
	virtual CacheErrorCode updateChildUID(std::vector<std::pair<ObjectUIDType, std::pair<ObjectUIDType, ObjectUIDType>>> vtUpdatedUIDs) = 0;

	virtual void applyExistingUpdates(std::vector<std::pair<ObjectUIDType, std::pair<ObjectUIDType, std::shared_ptr<ObjectType>>>>& vtNodes
		, std::unordered_map<ObjectUIDType, std::optional<ObjectUIDType>>& mpUpdatedUIDs) = 0;
};