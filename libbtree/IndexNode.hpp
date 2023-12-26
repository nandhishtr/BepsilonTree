#pragma once
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <iterator>
#include <iostream>
#include <cmath>
#include <optional>

#include <iostream>
#include <fstream>


#include "ErrorCodes.h"

#define __POSITION_AWARE_ITEMS__

using namespace std;

template <typename KeyType, typename ValueType, typename ObjectUIDType, uint8_t TYPE_UID>
class IndexNode
{
public:
	static const uint8_t UID = TYPE_UID;
	
private:
	typedef IndexNode<KeyType, ValueType, ObjectUIDType, UID> SelfType;

	typedef std::vector<KeyType>::const_iterator KeyTypeIterator;
	typedef std::vector<ObjectUIDType>::const_iterator CacheKeyTypeIterator;

public:
	struct INDEXNODESTRUCT
	{
		std::vector<KeyType> m_vtPivots;
		std::vector<ObjectUIDType> m_vtChildren;
	};

	std::shared_ptr<INDEXNODESTRUCT> m_ptrData;

public:
	~IndexNode()
	{
		// TODO: check for ref count?
		m_ptrData.reset();
	}

	IndexNode()
		: m_ptrData(make_shared<INDEXNODESTRUCT>())
	{	
	}

	IndexNode(std::fstream& is)
		: m_ptrData(make_shared<INDEXNODESTRUCT>())
	{
		size_t nKeyCount, nValueCount;
		is.read(reinterpret_cast<char*>(&nKeyCount), sizeof(size_t));
		is.read(reinterpret_cast<char*>(&nValueCount), sizeof(size_t));

		m_ptrData->m_vtPivots.resize(nKeyCount);
		m_ptrData->m_vtChildren.resize(nValueCount);

		is.read(reinterpret_cast<char*>(m_ptrData->m_vtPivots.data()), nKeyCount * sizeof(KeyType));
		is.read(reinterpret_cast<char*>(m_ptrData->m_vtChildren.data()), nValueCount * sizeof(ObjectUIDType));
	}

	IndexNode(KeyTypeIterator itBeginPivots, KeyTypeIterator itEndPivots, CacheKeyTypeIterator itBeginChildren, CacheKeyTypeIterator itEndChildren)
		: m_ptrData(make_shared<INDEXNODESTRUCT>())
	{
		m_ptrData->m_vtPivots.assign(itBeginPivots, itEndPivots);
		m_ptrData->m_vtChildren.assign(itBeginChildren, itEndChildren);
	}

	IndexNode(const KeyType& pivotKey, const ObjectUIDType& ptrLHSNode, const ObjectUIDType& ptrRHSNode)
		: m_ptrData(make_shared<INDEXNODESTRUCT>())
	{
		m_ptrData->m_vtPivots.push_back(pivotKey);
		m_ptrData->m_vtChildren.push_back(ptrLHSNode);
		m_ptrData->m_vtChildren.push_back(ptrRHSNode);

	}

	inline void insert(const KeyType& pivotKey, const ObjectUIDType& uidSibling)
	{
		size_t nChildIdx = m_ptrData->m_vtPivots.size();
		for (int nIdx = 0; nIdx < m_ptrData->m_vtPivots.size(); ++nIdx)
		{
			if (pivotKey < m_ptrData->m_vtPivots[nIdx])
			{
				nChildIdx = nIdx;
				break;
			}
		}

		m_ptrData->m_vtPivots.insert(m_ptrData->m_vtPivots.begin() + nChildIdx, pivotKey);
		m_ptrData->m_vtChildren.insert(m_ptrData->m_vtChildren.begin() + nChildIdx + 1, uidSibling);
	}

	template <typename CacheType, typename ObjectType>
#ifdef __POSITION_AWARE_ITEMS__
	inline ErrorCode rebalanceIndexNode(CacheType ptrCache, ObjectType ptrChild, const KeyType& key, size_t nDegree, ObjectUIDType& uidChild, std::optional<ObjectUIDType>& uidObjectToDelete, std::optional<ObjectUIDType>& uidparent)
#else
	inline ErrorCode rebalanceIndexNode(CacheType ptrCache, ObjectType ptrChild, const KeyType& key, size_t nDegree, ObjectUIDType& uidChild, std::optional<ObjectUIDType>& uidObjectToDelete)
#endif __POSITION_AWARE_ITEMS__
	{
		ObjectType ptrLHSNode = nullptr;
		ObjectType ptrRHSNode = nullptr;

		size_t nChildIdx = getChildNodeIdx(key);

		if (nChildIdx > 0)
		{
			ptrCache->template getObjectOfType<ObjectType>(m_ptrData->m_vtChildren[nChildIdx - 1], ptrLHSNode);    //TODO: lock

			if (ptrLHSNode->getKeysCount() > std::ceil(nDegree / 2.0f))	// TODO: macro?
			{
				KeyType key;
#ifdef __POSITION_AWARE_ITEMS__
				ptrChild->moveAnEntityFromLHSSibling<CacheType>(ptrLHSNode, m_ptrData->m_vtPivots[nChildIdx - 1], key, uidparent, ptrCache);
#else
				ptrChild->moveAnEntityFromLHSSibling(ptrLHSNode, m_ptrData->m_vtPivots[nChildIdx - 1], key);
#endif __POSITION_AWARE_ITEMS__

				m_ptrData->m_vtPivots[nChildIdx - 1] = key;
				return ErrorCode::Success;
			}
		}

		if (nChildIdx < m_ptrData->m_vtPivots.size())
		{
			ptrCache->template getObjectOfType<ObjectType>(m_ptrData->m_vtChildren[nChildIdx + 1], ptrRHSNode);    //TODO: lock

			if (ptrRHSNode->getKeysCount() > std::ceil(nDegree / 2.0f))
			{
				KeyType key;
#ifdef __POSITION_AWARE_ITEMS__
				ptrChild->moveAnEntityFromRHSSibling<CacheType>(ptrRHSNode, m_ptrData->m_vtPivots[nChildIdx], key, uidparent, ptrCache);
#else
				ptrChild->moveAnEntityFromRHSSibling(ptrRHSNode, m_ptrData->m_vtPivots[nChildIdx], key);
#endif __POSITION_AWARE_ITEMS__

				m_ptrData->m_vtPivots[nChildIdx] = key;
				return ErrorCode::Success;
			}
		}

		if (nChildIdx > 0)
		{
#ifdef __POSITION_AWARE_ITEMS__
			ptrLHSNode->mergeNodes<CacheType>(ptrChild, m_ptrData->m_vtPivots[nChildIdx - 1], uidparent, ptrCache);
#else
			ptrLHSNode->mergeNodes(ptrChild, m_ptrData->m_vtPivots[nChildIdx - 1]);
#endif __POSITION_AWARE_ITEMS__

			uidObjectToDelete = m_ptrData->m_vtChildren[nChildIdx];
			if (uidObjectToDelete != uidChild)
			{
				throw new std::exception("should not occur!");
			}

			m_ptrData->m_vtPivots.erase(m_ptrData->m_vtPivots.begin() + nChildIdx - 1);
			m_ptrData->m_vtChildren.erase(m_ptrData->m_vtChildren.begin() + nChildIdx);

			//uidObjectToDelete = uidChild;

			return ErrorCode::Success;
		}

		if (nChildIdx < m_ptrData->m_vtPivots.size())
		{
#ifdef __POSITION_AWARE_ITEMS__
			ptrChild->mergeNodes<CacheType>(ptrRHSNode, m_ptrData->m_vtPivots[nChildIdx], uidparent, ptrCache);
#else
			ptrChild->mergeNodes(ptrRHSNode, m_ptrData->m_vtPivots[nChildIdx]);
#endif __POSITION_AWARE_ITEMS__

			uidObjectToDelete = m_ptrData->m_vtChildren[nChildIdx + 1];

			m_ptrData->m_vtPivots.erase(m_ptrData->m_vtPivots.begin() + nChildIdx);
			m_ptrData->m_vtChildren.erase(m_ptrData->m_vtChildren.begin() + nChildIdx + 1);

			return ErrorCode::Success;
		}

		throw new exception("should not occur!"); // TODO: critical log entry.
	}

	template <typename CacheType, typename ObjectType>
	inline ErrorCode rebalanceDataNode(CacheType ptrCache, ObjectType ptrChild, const KeyType& key, size_t nDegree, ObjectUIDType& uidChild, std::optional<ObjectUIDType>& uidObjectToDelete)
	{
		ObjectType ptrLHSNode = nullptr;
		ObjectType ptrRHSNode = nullptr;

		size_t nChildIdx = getChildNodeIdx(key);


		if (nChildIdx > 0)
		{
			ptrCache->template getObjectOfType<ObjectType>(m_ptrData->m_vtChildren[nChildIdx - 1], ptrLHSNode);    //TODO: lock

			if (ptrLHSNode->getKeysCount() > std::ceil(nDegree / 2.0f))
			{
				KeyType key;
				ptrChild->moveAnEntityFromLHSSibling(ptrLHSNode, key);

				m_ptrData->m_vtPivots[nChildIdx - 1] = key;
				return ErrorCode::Success;
			}
		}

		if (nChildIdx < m_ptrData->m_vtPivots.size())
		{
			ptrCache->template getObjectOfType<ObjectType>(m_ptrData->m_vtChildren[nChildIdx + 1], ptrRHSNode);    //TODO: lock

			if (ptrRHSNode->getKeysCount() > std::ceil(nDegree / 2.0f))
			{
				KeyType key;
				ptrChild->moveAnEntityFromRHSSibling(ptrRHSNode, key);

				m_ptrData->m_vtPivots[nChildIdx] = key;
				return ErrorCode::Success;
			}
		}

		if (nChildIdx > 0)
		{
			ptrLHSNode->mergeNode(ptrChild);

			uidObjectToDelete = m_ptrData->m_vtChildren[nChildIdx];
			if (uidObjectToDelete != uidChild)
			{
				throw new std::exception("should not occur!");
			}

			m_ptrData->m_vtPivots.erase(m_ptrData->m_vtPivots.begin() + nChildIdx - 1);
			m_ptrData->m_vtChildren.erase(m_ptrData->m_vtChildren.begin() + nChildIdx);

			//uidObjectToDelete = uidChild;

			return ErrorCode::Success;
		}

		if (nChildIdx < m_ptrData->m_vtPivots.size())
		{
			ptrChild->mergeNode(ptrRHSNode);

			uidObjectToDelete = m_ptrData->m_vtChildren[nChildIdx + 1];

			m_ptrData->m_vtPivots.erase(m_ptrData->m_vtPivots.begin() + nChildIdx);
			m_ptrData->m_vtChildren.erase(m_ptrData->m_vtChildren.begin() + nChildIdx + 1);

			return ErrorCode::Success;
		}
		
		throw new exception("should not occur!"); // TODO: critical log entry.
	}

	inline size_t getKeysCount() 
	{
		return m_ptrData->m_vtPivots.size();
	}

	inline size_t getChildNodeIdx(const KeyType& key)
	{
		size_t nChildIdx = 0;
		while (nChildIdx < m_ptrData->m_vtPivots.size() && key >= m_ptrData->m_vtPivots[nChildIdx])
		{
			nChildIdx++;
		}

		return nChildIdx;
	}

	inline ObjectUIDType getChildAt(size_t nIdx) 
	{
		return m_ptrData->m_vtChildren[nIdx];
	}

	inline ObjectUIDType getChild(const KeyType& key)
	{
		return m_ptrData->m_vtChildren[getChildNodeIdx(key)];
	}

	inline bool requireSplit(size_t nDegree)
	{
		return m_ptrData->m_vtPivots.size() > nDegree;
	}

	inline bool canTriggerSplit(size_t nDegree)
	{
		return m_ptrData->m_vtPivots.size() + 1 > nDegree;
	}

	inline bool canTriggerMerge(size_t nDegree)
	{
		return m_ptrData->m_vtPivots.size() <= std::ceil(nDegree / 2.0f) + 1;	// TODO: macro!

	}

	inline bool requireMerge(size_t nDegree)
	{
		return m_ptrData->m_vtPivots.size() <= std::ceil(nDegree / 2.0f);
	}

	template <typename Cache>
#ifdef __POSITION_AWARE_ITEMS__
	inline ErrorCode split(Cache ptrCache, std::optional<ObjectUIDType>& ptrSibling, std::optional<ObjectUIDType>& keyparent, KeyType& pivotKey)
#else
	inline ErrorCode split(Cache ptrCache, std::optional<ObjectUIDType>& ptrSibling, KeyType& pivotKey)
#endif __POSITION_AWARE_ITEMS__
	{
		size_t nMid = m_ptrData->m_vtPivots.size() / 2;

#ifdef __POSITION_AWARE_ITEMS__
		std::shared_ptr< SelfType> _ptr = nullptr;
		ptrCache->template createObjectOfType<SelfType>(ptrSibling, keyparent, _ptr,
			m_ptrData->m_vtPivots.begin() + nMid + 1, m_ptrData->m_vtPivots.end(),
			m_ptrData->m_vtChildren.begin() + nMid + 1, m_ptrData->m_vtChildren.end());
		_ptr->updateParent<Cache>(ptrCache, ptrSibling);
#else
		ptrCache->template createObjectOfType<SelfType>(ptrSibling,
			m_ptrData->m_vtPivots.begin() + nMid + 1, m_ptrData->m_vtPivots.end(),
			m_ptrData->m_vtChildren.begin() + nMid + 1, m_ptrData->m_vtChildren.end());
#endif __POSITION_AWARE_ITEMS__

		if (!ptrSibling)
		{
			return ErrorCode::Error;
		}

		pivotKey = m_ptrData->m_vtPivots[nMid];

		m_ptrData->m_vtPivots.resize(nMid);
		m_ptrData->m_vtChildren.resize(nMid + 1);

		return ErrorCode::Success;
	}

#ifdef __POSITION_AWARE_ITEMS__
	template <typename CacheType>
	inline void moveAnEntityFromLHSSibling(shared_ptr<SelfType> ptrLHSSibling, KeyType& keytoassign, KeyType& pivotKey, std::optional<ObjectUIDType>& uidparent, CacheType ptrCache)
#else
	inline void moveAnEntityFromLHSSibling(shared_ptr<SelfType> ptrLHSSibling, KeyType& keytoassign, KeyType& pivotKey)
#endif __POSITION_AWARE_ITEMS__
	{
		KeyType key = ptrLHSSibling->m_ptrData->m_vtPivots.back();
		ObjectUIDType value = ptrLHSSibling->m_ptrData->m_vtChildren.back();

		ptrLHSSibling->m_ptrData->m_vtPivots.pop_back();
		ptrLHSSibling->m_ptrData->m_vtChildren.pop_back();

		if (ptrLHSSibling->m_ptrData->m_vtPivots.size() == 0)
		{
			throw new std::exception("should not occur!");
		}

		m_ptrData->m_vtPivots.insert(m_ptrData->m_vtPivots.begin(), keytoassign);
		m_ptrData->m_vtChildren.insert(m_ptrData->m_vtChildren.begin(), value);

		pivotKey = key;

#ifdef __POSITION_AWARE_ITEMS__
		if (uidparent)
		ptrCache->updateParentUID(value, uidparent);
#endif __POSITION_AWARE_ITEMS__

	}

#ifdef __POSITION_AWARE_ITEMS__
	template <typename CacheType>
	inline void moveAnEntityFromRHSSibling(shared_ptr<SelfType> ptrRHSSibling, KeyType& keytoassign, KeyType& pivotKey, std::optional<ObjectUIDType>& uidparent, CacheType ptrCache)
#else
	inline void moveAnEntityFromRHSSibling(shared_ptr<SelfType> ptrRHSSibling, KeyType& keytoassign, KeyType& pivotKey)
#endif __POSITION_AWARE_ITEMS__
	{
		KeyType key = ptrRHSSibling->m_ptrData->m_vtPivots.front();
		ObjectUIDType value = ptrRHSSibling->m_ptrData->m_vtChildren.front();

		ptrRHSSibling->m_ptrData->m_vtPivots.erase(ptrRHSSibling->m_ptrData->m_vtPivots.begin());
		ptrRHSSibling->m_ptrData->m_vtChildren.erase(ptrRHSSibling->m_ptrData->m_vtChildren.begin());

		if (ptrRHSSibling->m_ptrData->m_vtPivots.size() == 0)
		{
			throw new std::exception("should not occur!");

		}

		m_ptrData->m_vtPivots.push_back(keytoassign);
		m_ptrData->m_vtChildren.push_back(value);

		pivotKey = key;// ptrRHSSibling->m_ptrData->m_vtPivots.front();

#ifdef __POSITION_AWARE_ITEMS__
		if (uidparent)
		ptrCache->updateParentUID(value, uidparent);
#endif __POSITION_AWARE_ITEMS__
	}

#ifdef __POSITION_AWARE_ITEMS__
	template <typename CacheType>
	inline void mergeNodes(shared_ptr<SelfType> ptrSibling, KeyType& pivotKey, std::optional<ObjectUIDType>& uidparent, CacheType ptrCache)
#else
	inline void mergeNodes(shared_ptr<SelfType> ptrSibling, KeyType& pivotKey)
#endif __POSITION_AWARE_ITEMS__
	{
		m_ptrData->m_vtPivots.push_back(pivotKey);
		m_ptrData->m_vtPivots.insert(m_ptrData->m_vtPivots.end(), ptrSibling->m_ptrData->m_vtPivots.begin(), ptrSibling->m_ptrData->m_vtPivots.end());
		m_ptrData->m_vtChildren.insert(m_ptrData->m_vtChildren.end(), ptrSibling->m_ptrData->m_vtChildren.begin(), ptrSibling->m_ptrData->m_vtChildren.end());

#ifdef __POSITION_AWARE_ITEMS__
		if (uidparent)
		{
			auto it = ptrSibling->m_ptrData->m_vtChildren.begin();
			while (it != ptrSibling->m_ptrData->m_vtChildren.end())
			{
				ptrCache->updateParentUID(*it, uidparent);
				it++;
			}
		}
#endif __POSITION_AWARE_ITEMS__
	}

public:
	inline void writeToStream(std::fstream& os, uint8_t& uidObjectType, size_t& nDataSize)
	{
		static_assert(
			std::is_trivial<KeyType>::value &&
			std::is_standard_layout<KeyType>::value &&
			std::is_trivial<ObjectUIDType::NodeUID>::value &&
			std::is_standard_layout<ObjectUIDType::NodeUID>::value,
			"Can only deserialize POD types with this function");

		uidObjectType = UID;

		size_t nKeyCount = m_ptrData->m_vtPivots.size();
		size_t nValueCount = m_ptrData->m_vtChildren.size();

		nDataSize = sizeof(uint8_t) + (nKeyCount * sizeof(KeyType)) + (nValueCount * sizeof(ValueType)) + sizeof(size_t) + sizeof(size_t);

		os.write(reinterpret_cast<const char*>(&UID), sizeof(uint8_t));
		os.write(reinterpret_cast<const char*>(&nKeyCount), sizeof(size_t));
		os.write(reinterpret_cast<const char*>(&nValueCount), sizeof(size_t));
		os.write(reinterpret_cast<const char*>(m_ptrData->m_vtPivots.data()), nKeyCount * sizeof(KeyType));
		os.write(reinterpret_cast<const char*>(m_ptrData->m_vtChildren.data()), nValueCount * sizeof(ObjectUIDType));
	}

	ErrorCode updateChildUID(ObjectUIDType uidOld, ObjectUIDType uidNew)
	{
		auto it = m_ptrData->m_vtChildren.begin();
		while (it != m_ptrData->m_vtChildren.end())
		{
			if (*it == uidOld)
			{
				*it = uidNew;
				return ErrorCode::Success;
			}
			it++;
		}
		return ErrorCode::Error;
	}

	template <typename Cache>
	ErrorCode updateParent(Cache ptrCache, std::optional<ObjectUIDType>& parentuid)
	{
		auto it = m_ptrData->m_vtChildren.begin();
		while (it != m_ptrData->m_vtChildren.end())
		{
			if( ptrCache->updateParentUID(*it, parentuid) != CacheErrorCode::Success)
				throw new std::exception("should not occur!");
			it++;
		}
		return ErrorCode::Error;
	}

public:
	template <typename CacheType, typename ObjectType, typename DataNodeType>
#ifdef __POSITION_AWARE_ITEMS__
	void print(std::ofstream& out, CacheType ptrCache, size_t nLevel, string prefix, std::optional<ObjectUIDType> uidParent)
#else //__POSITION_AWARE_ITEMS__
	void print(std::ofstream& out, CacheType ptrCache, size_t nLevel, string prefix)
#endif __POSITION_AWARE_ITEMS__
	{
		int nSpace = 7;

		prefix.append(std::string(nSpace - 1, ' '));
		prefix.append("|");
		for (size_t nIndex = 0; nIndex < m_ptrData->m_vtChildren.size(); nIndex++)
		{
			out << " " << prefix << std::endl;
			out << " " << prefix << std::string(nSpace, '-').c_str();// << std::endl;

			if (nIndex < m_ptrData->m_vtPivots.size())
			{
				out << " < (" << m_ptrData->m_vtPivots[nIndex] << ")";// << std::endl;
			}
			else {
				out << " >= (" << m_ptrData->m_vtPivots[nIndex - 1] << ")";// << std::endl;
			}


			out << std::endl;

			ObjectType ptrNode = nullptr;
#ifdef __POSITION_AWARE_ITEMS__
			std::optional<ObjectUIDType> uidCurrentNodeParent;

			uidCurrentNodeParent = uidParent;
			ptrCache->getObject(m_ptrData->m_vtChildren[nIndex], ptrNode, uidCurrentNodeParent);    //TODO: lock

			if (uidCurrentNodeParent != uidParent)
			{
				throw new std::exception("should not occur!");   // TODO: critical log.
			}
#else
			ptrCache->getObject(m_ptrData->m_vtChildren[nIndex], ptrNode);
#endif __POSITION_AWARE_ITEMS__

			if (std::holds_alternative<shared_ptr<SelfType>>(*ptrNode->data))
			{
				shared_ptr<SelfType> ptrIndexNode = std::get<shared_ptr<SelfType>>(*ptrNode->data);

#ifdef __POSITION_AWARE_ITEMS__
				ptrIndexNode->template print<CacheType, ObjectType, DataNodeType>(out, ptrCache, nLevel + 1, prefix, m_ptrData->m_vtChildren[nIndex]);
#else //__POSITION_AWARE_ITEMS__
				ptrIndexNode->template print<CacheType, ObjectType, DataNodeType>(out, ptrCache, nLevel + 1, prefix);
#endif __POSITION_AWARE_ITEMS__
			}
			else if (std::holds_alternative<shared_ptr<DataNodeType>>(*ptrNode->data))
			{
				shared_ptr<DataNodeType> ptrDataNode = std::get<shared_ptr<DataNodeType>>(*ptrNode->data);
				ptrDataNode->print(out, nLevel + 1, prefix);
			}
		}
	}

	void wieHiestDu() {
		printf("ich heisse InternalNode.\n");
	}
};