#pragma once

#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <iterator>
#include <iostream>
#include <cmath>

#include "INode.h"

using namespace std;

//template <typename KeyType, typename ValueType, typename CacheKeyType>
//class IndexNode
//{
//public:
//	struct INDEXNODESTRUCT
//	{
//		std::vector<KeyType> m_vtPivots;
//		std::vector<CacheKeyType> m_vtChildren;
//	};
//
//	typedef std::shared_ptr<IndexNode<KeyType, ValueType, CacheKeyType>> IndexNodePtr;
//	typedef IndexNode<KeyType, ValueType, CacheKeyType> SelfType;
//
//	std::shared_ptr<INDEXNODESTRUCT> m_ptrData;
//
//public:
//	~IndexNode()
//	{
//	}
//
//	IndexNode()
//	{
//		m_ptrData = make_shared<INDEXNODESTRUCT>();
//	}
//
//	IndexNode(std::vector<KeyType>::const_iterator itBeginPivot, std::vector<KeyType>::const_iterator itEndPivot,
//		std::vector<CacheKeyType>::const_iterator itBeginChildren, std::vector<CacheKeyType>::const_iterator itEndChildren)
//		: m_ptrData(make_shared<INDEXNODESTRUCT>())
//	{
//		m_ptrData->m_vtPivots.assign(itBeginPivot, itEndPivot);
//		m_ptrData->m_vtChildren.assign(itBeginChildren, itEndChildren);
//	}
//
//	IndexNode(const KeyType& pivotKey, const CacheKeyType& ptrLHSNode, const CacheKeyType& ptrRHSNode)
//		: m_ptrData(make_shared<INDEXNODESTRUCT>())
//	{
//		m_ptrData->m_vtPivots.push_back(pivotKey);
//		m_ptrData->m_vtChildren.push_back(ptrLHSNode);
//		m_ptrData->m_vtChildren.push_back(ptrRHSNode);
//
//	}
//
//	inline CacheKeyType getChild(const KeyType& key)
//	{
//		size_t nChildIdx = 0;
//		while (nChildIdx < m_ptrData->m_vtPivots.size() && key >= m_ptrData->m_vtPivots[nChildIdx])
//		{
//			nChildIdx++;
//		}
//
//		return m_ptrData->m_vtChildren[nChildIdx];
//	}
//
//	inline CacheKeyType getChildNodeIndex(const KeyType& key)
//	{
//		/*for (size_t nIdx = 0; nIdx < m_ptrData->m_vtPivots.size(); ++nIdx)
//		{
//			if (key < m_ptrData->m_vtPivots[nIdx])
//			{
//				return m_ptrData->m_vtChildren[nIdx];
//			}
//		}
//		return m_ptrData->m_vtChildren[m_ptrData->m_vtPivots.size()];*/
//		size_t nChildIdx = 0;
//		while (nChildIdx < m_ptrData->m_vtPivots.size() && key >= m_ptrData->m_vtPivots[nChildIdx])
//		{
//			nChildIdx++;
//		}
//
//		return m_ptrData->m_vtChildren[nChildIdx];
//	}
//
//	inline bool requireSplit(size_t nDegree)
//	{
//		return m_ptrData->m_vtPivots.size() > nDegree;
//	}
//
//	inline bool canTriggerSplit(size_t nDegree)
//	{
//		return m_ptrData->m_vtPivots.size() + 1 > nDegree;
//	}
//
//	inline bool canTriggerMerge(size_t nDegree)
//	{
//		return m_ptrData->m_vtPivots.size() - 1 <= std::ceil(nDegree / 2.0f);
//
//	}
//
//	inline bool requireMerge(size_t nDegree)
//	{
//		return m_ptrData->m_vtPivots.size() <= std::ceil(nDegree / 2.0f);
//	}
//
//	inline void insert(const KeyType& pivotKey, const CacheKeyType& ptrSibling)
//	{
//		size_t nChildIdx = m_ptrData->m_vtPivots.size();
//		for (int nIdx = 0; nIdx < m_ptrData->m_vtPivots.size(); ++nIdx)
//		{
//			if (pivotKey < m_ptrData->m_vtPivots[nIdx])
//			{
//				nChildIdx = nIdx;
//				break;
//			}
//		}
//
//		m_ptrData->m_vtPivots.insert(m_ptrData->m_vtPivots.begin() + nChildIdx, pivotKey);
//		m_ptrData->m_vtChildren.insert(m_ptrData->m_vtChildren.begin() + nChildIdx + 1, ptrSibling);
//	}
//
//	template <typename Cache>
//	inline ErrorCode split(Cache ptrCache, std::optional<CacheKeyType>& ptrSibling, KeyType& pivotKey)
//	{
//		size_t nMid = m_ptrData->m_vtPivots.size() / 2;
//
//		ptrSibling = ptrCache->template createObjectOfType<IndexNode<KeyType, ValueType, CacheKeyType>>(
//			m_ptrData->m_vtPivots.begin() + nMid + 1, m_ptrData->m_vtPivots.end(),
//			m_ptrData->m_vtChildren.begin() + nMid + 1, m_ptrData->m_vtChildren.end());
//
//		if (!ptrSibling)
//		{
//			return ErrorCode::Error;
//		}
//
//		pivotKey = m_ptrData->m_vtPivots[nMid];
//
//		m_ptrData->m_vtPivots.resize(nMid);
//		m_ptrData->m_vtChildren.resize(nMid + 1);
//
//		return ErrorCode::Success;
//	}
//
//	inline std::tuple<std::optional<KeyType*>, std::optional<KeyType*>, size_t> getImmediateSiblings(KeyType& key)
//	{
//		size_t nChildIdx = 0;
//		while (nChildIdx < m_ptrData->m_vtPivots.size() && key > m_ptrData->m_vtPivots[nChildIdx])
//		{
//			nChildIdx++;
//		}
//
//		std::optional<KeyType*> LHS = nullopt;
//		std::optional<KeyType*> RHS = nullopt;
//
//		if (nChildIdx > 0)
//		{
//			LHS = &m_ptrData->m_vtPivots[nChildIdx - 1];
//		}
//
//		if (nChildIdx < m_ptrData->m_vtPivots.size())
//		{
//			RHS = &m_ptrData->m_vtPivots[nChildIdx + 1];
//		}
//
//		return std::make_tuple(LHS, RHS, nChildIdx);
//	}
//
//	inline size_t getKeysCount() {
//		return m_ptrData->m_vtPivots.size();
//	}
//
//	template <typename Cache, typename CacheValueType>
//	inline ErrorCode rebalance(Cache ptrCache, CacheValueType ptrChildPtr, CacheKeyType cktChild, const KeyType& ktChild, size_t nDegree, std::optional<CacheKeyType>& dlt)
//	{
//		CacheValueType ptrLHSNode = nullptr;
//		CacheValueType ptrRHSNode = nullptr;
//
//		size_t nChildIdx = 0;
//		while (nChildIdx < m_ptrData->m_vtPivots.size() && ktChild >= m_ptrData->m_vtPivots[nChildIdx])
//		{
//			nChildIdx++;
//		}
//
//		if (m_ptrData->m_vtPivots.size() <= 1)
//		{
//
//		}
//
//		if (nChildIdx > 0)
//		{
//			ptrLHSNode = ptrCache->template getObjectOfType<CacheValueType>(m_ptrData->m_vtChildren[nChildIdx - 1]);    //TODO: lock
//			if (ptrLHSNode->getKeysCount() > std::ceil(nDegree / 2.0f))
//			{
//				KeyType key;
//				ptrChildPtr->moveAnEntityFromLHSSibling(ptrLHSNode, m_ptrData->m_vtPivots[nChildIdx - 1], key);
//
//				m_ptrData->m_vtPivots[nChildIdx - 1] = key;
//				return ErrorCode::Success;
//			}
//		}
//
//		if (nChildIdx < m_ptrData->m_vtPivots.size())
//		{
//			ptrRHSNode = ptrCache->template getObjectOfType<CacheValueType>(m_ptrData->m_vtChildren[nChildIdx + 1]);    //TODO: lock
//			if (ptrRHSNode->getKeysCount() > std::ceil(nDegree / 2.0f))
//			{
//				KeyType key;
//				ptrChildPtr->moveAnEntityFromRHSSibling(ptrRHSNode, m_ptrData->m_vtPivots[nChildIdx], key);
//
//				m_ptrData->m_vtPivots[nChildIdx] = key;
//				return ErrorCode::Success;
//			}
//		}		
//
//		if (nChildIdx > 0)
//		{
//			ptrLHSNode->mergeNodes(ptrChildPtr, m_ptrData->m_vtPivots[nChildIdx - 1]);
//
//			//ptrCache->remove(ptrChildptr);
//
//			m_ptrData->m_vtPivots.erase(m_ptrData->m_vtPivots.begin() + nChildIdx - 1);
//			m_ptrData->m_vtChildren.erase(m_ptrData->m_vtChildren.begin() + nChildIdx);
//
//			dlt = cktChild;
//
//			return ErrorCode::Success;
//		}
//
//		if (nChildIdx < m_ptrData->m_vtPivots.size())
//		{
//			ptrChildPtr->mergeNodes(ptrRHSNode, m_ptrData->m_vtPivots[nChildIdx]);
//
//			dlt = m_ptrData->m_vtChildren[nChildIdx + 1];
//
//			m_ptrData->m_vtPivots.erase(m_ptrData->m_vtPivots.begin() + nChildIdx);
//			m_ptrData->m_vtChildren.erase(m_ptrData->m_vtChildren.begin() + nChildIdx + 1);
//
//			return ErrorCode::Success;
//		}
//		throw new exception("should not occur!");
//	}
//
//	template <typename Cache, typename CacheValueType>
//	inline ErrorCode rebalance_(Cache ptrCache, CacheValueType ptrChildPtr, CacheKeyType cktChild, const KeyType& ktChild, size_t nDegree, std::optional<CacheKeyType>& dlt)
//	{
//		CacheValueType ptrLHSNode = nullptr;
//		CacheValueType ptrRHSNode = nullptr;
//
//		size_t nChildIdx = 0;
//		while (nChildIdx < m_ptrData->m_vtPivots.size() && ktChild >= m_ptrData->m_vtPivots[nChildIdx])
//		{
//			nChildIdx++;
//		}
//
//		if (nChildIdx > 0)
//		{
//			ptrLHSNode = ptrCache->template getObjectOfType<CacheValueType>(m_ptrData->m_vtChildren[nChildIdx - 1]);    //TODO: lock
//			if (ptrLHSNode->getKeysCount() > std::ceil(nDegree / 2.0f))
//			{
//				KeyType key;
//				ptrChildPtr->moveAnEntityFromLHSSibling(ptrLHSNode, key);
//
//				m_ptrData->m_vtPivots[nChildIdx - 1] = key;
//				return ErrorCode::Success;
//			}
//		}
//
//		if (nChildIdx < m_ptrData->m_vtPivots.size())
//		{
//			ptrRHSNode = ptrCache->template getObjectOfType<CacheValueType>(m_ptrData->m_vtChildren[nChildIdx + 1]);    //TODO: lock
//			if (ptrRHSNode->getKeysCount() > std::ceil(nDegree / 2.0f))
//			{
//				KeyType key;
//				ptrChildPtr->moveAnEntityFromRHSSibling(ptrRHSNode, key);
//
//				m_ptrData->m_vtPivots[nChildIdx] = key;
//				return ErrorCode::Success;
//			}
//		}
//
//		if (nChildIdx > 0)
//		{
//			ptrLHSNode->mergeNodes(ptrChildPtr);
//
//			//ptrCache->remove(ptrChildptr);
//
//			m_ptrData->m_vtPivots.erase(m_ptrData->m_vtPivots.begin() + nChildIdx - 1);
//			m_ptrData->m_vtChildren.erase(m_ptrData->m_vtChildren.begin() + nChildIdx);
//
//			dlt = cktChild;
//
//			return ErrorCode::Success;
//		}
//
//		if (nChildIdx < m_ptrData->m_vtPivots.size())
//		{
//			ptrChildPtr->mergeNodes(ptrRHSNode);
//
//			dlt = m_ptrData->m_vtChildren[nChildIdx + 1];
//
//			m_ptrData->m_vtPivots.erase(m_ptrData->m_vtPivots.begin() + nChildIdx);
//			m_ptrData->m_vtChildren.erase(m_ptrData->m_vtChildren.begin() + nChildIdx + 1);
//
//			return ErrorCode::Success;
//		}
//		throw new exception("should not occur!");
//	}
//
//	inline void moveAnEntityFromLHSSibling(shared_ptr<SelfType> ptrLHSSibling, KeyType& keytoassign, KeyType& ktPivot)
//	{
//		KeyType key = ptrLHSSibling->m_ptrData->m_vtPivots.back();
//		CacheKeyType value = ptrLHSSibling->m_ptrData->m_vtChildren.back();
//
//		ptrLHSSibling->m_ptrData->m_vtPivots.pop_back();
//		ptrLHSSibling->m_ptrData->m_vtChildren.pop_back();
//
//		m_ptrData->m_vtPivots.insert(m_ptrData->m_vtPivots.begin(), keytoassign);
//		m_ptrData->m_vtChildren.insert(m_ptrData->m_vtChildren.begin(), value);
//
//		ktPivot = key;
//	}
//
//	inline void moveAnEntityFromRHSSibling(shared_ptr<SelfType> ptrRHSSibling, KeyType& keytoassign, KeyType& ktPivot)
//	{
//		KeyType key = ptrRHSSibling->m_ptrData->m_vtPivots.front();
//		CacheKeyType value = ptrRHSSibling->m_ptrData->m_vtChildren.front();
//
//		ptrRHSSibling->m_ptrData->m_vtPivots.erase(ptrRHSSibling->m_ptrData->m_vtPivots.begin());
//		ptrRHSSibling->m_ptrData->m_vtChildren.erase(ptrRHSSibling->m_ptrData->m_vtChildren.begin());
//		
//		m_ptrData->m_vtPivots.push_back(keytoassign);
//		m_ptrData->m_vtChildren.push_back(value);
//
//		ktPivot = key;// ptrRHSSibling->m_ptrData->m_vtPivots.front();
//	}
//
//	inline void mergeNodes(shared_ptr<SelfType> ptrSibling, KeyType& keytoassign)
//	{
//		m_ptrData->m_vtPivots.push_back(keytoassign);
//		m_ptrData->m_vtPivots.insert(m_ptrData->m_vtPivots.end(), ptrSibling->m_ptrData->m_vtPivots.begin(), ptrSibling->m_ptrData->m_vtPivots.end());
//		m_ptrData->m_vtChildren.insert(m_ptrData->m_vtChildren.end(), ptrSibling->m_ptrData->m_vtChildren.begin(), ptrSibling->m_ptrData->m_vtChildren.end());
//	}
//
//public:
//	void wieHiestDu() {
//		printf("ich heisse InternalNode.\n");
//	}
//};

template <typename KeyType, typename ValueType, typename CacheType>
class InternalNode : public INode<KeyType, ValueType, CacheType>
{
	typedef CacheType::KeyType CacheKeyType;
	typedef std::shared_ptr<CacheType> CacheTypePtr;
	typedef std::shared_ptr<INode<KeyType, ValueType, CacheType>> INodePtr;
	typedef std::shared_ptr<InternalNode<KeyType, ValueType, CacheType>> InternalNodePtr;

private:
	uint32_t m_nDegree;

	std::vector<KeyType> m_vtPivots;
	std::vector<CacheKeyType> m_vtChildren;

public:
	~InternalNode()
	{
	}

	InternalNode(uint32_t nDegree) : m_nDegree(nDegree)
	{

	}
	InternalNode(uint32_t nDegree, int nPivot, CacheKeyType ptrLHSNode, CacheKeyType ptrRHSNode)
		: m_nDegree(nDegree)
	{
		LOG(INFO) << "Creating InternalNode .. nPivot=" << nPivot << ".";

		m_vtPivots.push_back(nPivot);
		m_vtChildren.push_back(ptrLHSNode);
		m_vtChildren.push_back(ptrRHSNode);
	}

	InternalNode(uint32_t nDegree, InternalNode<KeyType, ValueType, CacheType>* ptrNodeSource, size_t nOffset)
		: m_nDegree(nDegree)
	{
		LOG(INFO) << "Creating InternalNode .. due to split .., Offset=" << nOffset << ".";

		this->m_vtPivots.assign(ptrNodeSource->m_vtPivots.begin() + nOffset, ptrNodeSource->m_vtPivots.end());
		this->m_vtChildren.assign(ptrNodeSource->m_vtChildren.begin() + nOffset, ptrNodeSource->m_vtChildren.end());

		std::ostringstream oss;
		std::copy(m_vtPivots.begin(), m_vtPivots.end(), std::ostream_iterator<int>(oss, ","));
		LOG(INFO) << "Newly created node details (" << oss.str() << ").";

	}

	void wieHiestDu() {
		printf("ich heisse InternalNode.\n");
	}

	ErrorCode insert(const KeyType& key, const ValueType& value, CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
	{
		int nChildIdx = 0;
		while (nChildIdx < m_vtPivots.size() && key > m_vtPivots[nChildIdx]) 
		{
			nChildIdx++;
		}

		LOG(INFO) << "Insert (Key=" << key << ", Value=" << value << ") Idx=" << nChildIdx << ".";

		INodePtr ptrParentNode = ptrCache->template getObjectOfType<INode<KeyType, ValueType, CacheType>>(m_vtChildren[nChildIdx]);
		if (ptrParentNode == NULL)
		{
			LOG(ERROR) << "Failed to get object.";

			return ErrorCode::Error;
		}

		int nChildPivot = 0;
		std::optional<CacheKeyType> ptrChildNode;
		ErrorCode retCode = ptrParentNode->insert(key, value, ptrCache, ptrChildNode, nChildPivot);

		if (retCode == ErrorCode::Error)
		{
			LOG(ERROR) << "Failed to insert object.";

			return ErrorCode::Error;
		}

		if (ptrChildNode)
		{
			LOG(INFO) << "Insert object due to split at the previous level.";

			return insert(ptrCache, *ptrChildNode, nChildPivot, ptrSiblingNode, nSiblingPivot);
		}

		return ErrorCode::Success;
	}

	ErrorCode insert(CacheTypePtr ptrCache, CacheKeyType ptrChildNode, int nChildPivot, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
	{
		int nChildIdx = m_vtPivots.size();
		for (int nIdx = 0; nIdx < m_vtPivots.size(); ++nIdx) 
		{
			if (nChildPivot < m_vtPivots[nIdx]) 
			{
				nChildIdx = nIdx;
				break;
			}
		}

		LOG(INFO) << "Insert object nChildIdx=" << nChildIdx << ".";

		m_vtPivots.insert(m_vtPivots.begin() + nChildIdx, nChildPivot);
		m_vtChildren.insert(m_vtChildren.begin() + nChildIdx + 1, ptrChildNode);

		if (requireSplit())
		{
			return split(ptrCache, ptrSiblingNode, nSiblingPivot);
		}

		return ErrorCode::Success;
	}


	//ErrorCode remove(const KeyType& key);

	ErrorCode search(CacheTypePtr ptrCache, const KeyType& key, ValueType& value)
	{
		int nIdx = getChildNodeIndex(key);

		INodePtr ptrChildNode = ptrCache->template getObjectOfType<INode<KeyType, ValueType, CacheType>>(m_vtChildren[nIdx]);
		if (ptrChildNode == NULL)
		{
			LOG(INFO) << "Failed to get object (nIdx=" << nIdx << ").";

			return ErrorCode::Error;
		}

		return ptrChildNode->search(ptrCache, key, value);
	}

	size_t getChildCount()
	{
		return m_vtPivots.size();
	}

	//ErrorCode moveAnEntityFromLHSSibling(INodePtr ptrLHSSibling, KeyType& oKey);
	//ErrorCode moveAnEntityFromRHSSibling(INodePtr ptrRHSSibling, KeyType& oKey);

	//ErrorCode mergeNodes(INodePtr ptrSiblingToMerge);
private:
	bool requireSplit()
	{
		if (m_vtPivots.size() > m_nDegree)
			return true;

		return false;
	}

	size_t getChildNodeIndex(const KeyType& key)
	{
		for (size_t nIdx = 0; nIdx < m_vtPivots.size(); ++nIdx) 
		{
			if (key < m_vtPivots[nIdx]) 
			{
				return nIdx;
			}
		}
		return m_vtPivots.size();
	}

	//ErrorCode rebalance(size_t nChildNodeIdx);
	ErrorCode split(CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
	{
		LOG(INFO) << "Trying to split object.";

		int nOffset = m_vtPivots.size() / 2;

		nSiblingPivot = this->m_vtPivots[nOffset];
		ptrSiblingNode = ptrCache->template createObjectOfType<InternalNode<KeyType, ValueType, CacheType>>(m_nDegree, this, nOffset);

		//m_vtPivots.erase(m_vtPivots.begin() + nOffset, m_vtPivots.end());
		//m_vtChildren.erase(m_vtChildren.begin() + nOffset, m_vtChildren.end());

		m_vtPivots.resize(nOffset);
		m_vtChildren.resize(nOffset + 1);

		std::ostringstream oss;
		std::copy(m_vtPivots.begin(), m_vtPivots.end(), std::ostream_iterator<int>(oss, ","));
		LOG(INFO) << "Current object after split (" << oss.str() << ") child count=" << m_vtChildren.size() << ".";

		LOG(INFO) << "Pivot=" << nSiblingPivot << ").";
		return ErrorCode::Success;
	}

	void print(CacheTypePtr ptrCache, int nLevel)
	{
		std::ostringstream oss;

		if (!m_vtPivots.empty())
		{
			// Convert all but the last element to avoid a trailing ","
			std::copy(m_vtPivots.begin(), m_vtPivots.end() - 1,
				std::ostream_iterator<int>(oss, ","));

			// Now add the last element with no delimiter
			oss << m_vtPivots.back();
		}

		printf("%s> level: %d\n", std::string(nLevel, '.').c_str(), nLevel);
		printf("%s  pivots: %s\n", std::string(nLevel, ' ').c_str(), oss.str().c_str());

		for (int i = 0; i < m_vtChildren.size(); i++)
		{
			printf("%s  child: %d\n", std::string(nLevel, ' ').c_str(), i);


			INodePtr ptrChildNode = ptrCache->template getObjectOfType<INode<KeyType, ValueType, CacheType>>(m_vtChildren[i]);
			if (ptrChildNode == NULL)
				return;

			ptrChildNode->print(ptrCache, nLevel + 1);
		}
	}
};