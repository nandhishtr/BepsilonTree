#include "pch.h"
#include "LeafNode.h"

//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//LeafNode<KeyType, ValueType, CacheType>::~LeafNode()
//{
//
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//LeafNode<KeyType, ValueType, CacheType>::LeafNode(uint32_t nDegree)
//	: m_nDegree(nDegree)
//{
//
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//LeafNode<KeyType, ValueType, CacheType>::LeafNode(uint32_t nDegree, LeafNode<KeyType, ValueType, CacheType>* ptrNodeSource, size_t nOffset)
//	: m_nDegree(nDegree)
//{
//	this->m_vtKeys.assign(ptrNodeSource->m_vtKeys.begin() + nOffset, ptrNodeSource->m_vtKeys.end());
//	this->m_vtValues.assign(ptrNodeSource->m_vtValues.begin() + nOffset, ptrNodeSource->m_vtValues.end());
//
//	std::ostringstream oss;
//	std::copy(m_vtKeys.begin(), m_vtKeys.end(), std::ostream_iterator<int>(oss, ","));
//	std::cout << "LeafNode::split - after - new node - pivots: " << oss.str() << std::endl;
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//ErrorCode LeafNode<KeyType, ValueType, CacheType>::insert(const KeyType& key, const ValueType& value, CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
//{
//	std::cout << "LeafNode::insert(" << key << "," << value << ")" << std::endl;
//
//	m_vtKeys.push_back(key);
//	m_vtValues.push_back(value);
//
//	if (requireSplit())
//	{
//		return split(ptrCache, ptrSiblingNode, nSiblingPivot);
//	}
//
//	return ErrorCode::Success;
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//ErrorCode LeafNode<KeyType, ValueType, CacheType>::insert(CacheTypePtr ptrCache, CacheKeyType ptrChildNode, int nChildPivot, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
//{
//	throw new exception("should occur!");
//	return ErrorCode::ChildSplitCalledOnLeafNode;
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//bool LeafNode<KeyType, ValueType, CacheType>::requireSplit()
//{
//	if (m_vtKeys.size() > m_nDegree)
//		return true;
//	
//	return false;
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//ErrorCode LeafNode<KeyType, ValueType, CacheType>::split(CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
//{
//	std::cout << "LeafNode::split" << std::endl;
//
//	int nOffset = m_vtKeys.size() / 2;
//
//	ptrSiblingNode = ptrCache->createObjectOfType<LeafNode<KeyType, ValueType, CacheType>>(5, this, nOffset);
//	if (!ptrSiblingNode)
//	{
//		return ErrorCode::Error;
//	}
//
//	nSiblingPivot = this->m_vtKeys[nOffset];
//	
//	m_vtKeys.resize(nOffset);
//	m_vtValues.resize(nOffset);
//
//	std::ostringstream oss;
//	std::copy(m_vtKeys.begin(), m_vtKeys.end() , std::ostream_iterator<int>(oss, ","));
//	std::cout << "LeafNode::split - after - current node - pivots: " << oss.str() << std::endl;
//
//	std::cout << "LeafNode::split pivot " << nSiblingPivot << std::endl;
//
//	return ErrorCode::Success;
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//ErrorCode LeafNode<KeyType, ValueType, CacheType>::remove(const KeyType& key) 
//{
//	auto it = std::lower_bound(m_vtKeys.begin(), m_vtKeys.end(), key);
//
//	if (it != m_vtKeys.end() && *it == key) 
//	{
//		int index = it - m_vtKeys.begin();
//		m_vtKeys.erase(it);
//		m_vtValues.erase(m_vtValues.begin() + index);
//
//		return ErrorCode::Success;
//	}
//
//	return ErrorCode::KeyDoesNotExist;
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//ErrorCode LeafNode<KeyType, ValueType, CacheType>::search(CacheTypePtr ptrCache, const KeyType& key, ValueType& value)
//{
//	auto it = std::lower_bound(m_vtKeys.begin(), m_vtKeys.end(), key);
//	if (it != m_vtKeys.end() && *it == key) {
//		int index = it - m_vtKeys.begin();
//		value = m_vtValues[index];
//
//		return ErrorCode::Success;
//	}
//
//	return ErrorCode::KeyDoesNotExist;
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//size_t LeafNode<KeyType, ValueType, CacheType>::getChildCount()
//{
//	return m_vtKeys.size();
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//ErrorCode LeafNode<KeyType, ValueType, CacheType>::moveAnEntityFromLHSSibling(LeafNodePtr ptrLHSSibling, KeyType& oKey)
//{
//	LeafNodePtr lhsibling = static_cast<LeafNodePtr>(ptrLHSSibling);
//
//	KeyType key = lhsibling->m_vtKeys.back();
//	ValueType value = lhsibling->m_vtValues.back();
//
//	lhsibling->m_vtKeys.pop_back();
//	lhsibling->m_vtValues.pop_back();
//
//	m_vtKeys.insert(m_vtKeys.begin(), key);
//	m_vtValues.insert(m_vtValues.begin(), value);
//
//	oKey = key;
//
//	return ErrorCode::Success;
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//ErrorCode LeafNode<KeyType, ValueType, CacheType>::moveAnEntityFromRHSSibling(LeafNodePtr ptrRHSSibling, KeyType& oKey)
//{
//	LeafNodePtr rhssibling = static_cast<LeafNodePtr>(ptrRHSSibling);
//
//	KeyType key = rhssibling->m_vtKeys.front();
//	ValueType value = rhssibling->m_vtValues.front();
//
//	rhssibling->m_vtKeys.erase(rhssibling->m_vtKeys.begin());
//	rhssibling->m_vtValues.erase(rhssibling->m_vtValues.begin());
//
//	m_vtKeys.push_back(key);
//	m_vtValues.push_back(value);
//
//	oKey = rhssibling->m_vtKeys.front();
//
//	return ErrorCode::Success;
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//ErrorCode LeafNode<KeyType, ValueType, CacheType>::mergeNodes(LeafNodePtr ptrSiblingToMerge) 
//{
//	LeafNodePtr ptrNode = static_cast<LeafNodePtr>(ptrSiblingToMerge);
//
//	m_vtKeys.insert(m_vtKeys.end(), ptrNode->m_vtKeys.begin(), ptrNode->m_vtKeys.end());
//	m_vtValues.insert(m_vtValues.end(), ptrNode->m_vtValues.begin(), ptrNode->m_vtValues.end());
//
//	//delete ptrNode; todo..
//
//	return ErrorCode::Success;
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//void LeafNode<KeyType, ValueType, CacheType>::print(CacheTypePtr ptrCache, int nLevel)
//{
//	for (int i = 0; i < m_vtKeys.size(); i++)
//	{
//		printf("%s> level: %d, key: %d, value: %d\n", std::string(nLevel, '.').c_str(), nLevel, m_vtKeys[i], m_vtValues[i]);
//	}
//}