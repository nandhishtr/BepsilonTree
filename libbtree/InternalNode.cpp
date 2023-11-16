#include "pch.h"
#include "InternalNode.h"
#include <optional>

//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//InternalNode<KeyType, ValueType, CacheType>::~InternalNode()
//{
//
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//InternalNode<KeyType, ValueType, CacheType>::InternalNode(uint32_t nDegree)
//	: m_nDegree(nDegree)
//{
//	std::cout << "InternalNode::InternalNode()" << std::endl;
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//InternalNode<KeyType, ValueType, CacheType>::InternalNode(uint32_t nDegree, int nPivot, CacheKeyType ptrLHSNode, CacheKeyType ptrRHSNode)
//	: m_nDegree(nDegree)
//{
//	std::cout << "InternalNode::InternalNode(pvt, lhs, rhs)" << std::endl;
//
//	m_vtPivots.push_back(nPivot);
//	m_vtChildren.push_back(ptrLHSNode);
//	m_vtChildren.push_back(ptrRHSNode);
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//InternalNode<KeyType, ValueType, CacheType>::InternalNode(uint32_t nDegree, InternalNode<KeyType, ValueType, CacheType>* ptrNodeSource, size_t nOffset)
//	: m_nDegree(nDegree)
//{
//	std::cout << "InternalNode::InternalNode(..due to split)" << std::endl;
//
//	this->m_vtPivots.assign(ptrNodeSource->m_vtPivots.begin() + nOffset, ptrNodeSource->m_vtPivots.end());
//	this->m_vtChildren.assign(ptrNodeSource->m_vtChildren.begin() + nOffset, ptrNodeSource->m_vtChildren.end());
//
//	std::ostringstream oss;
//	std::copy(m_vtPivots.begin(), m_vtPivots.end() , std::ostream_iterator<int>(oss, ","));
//	std::cout << "InternalNode::split - after - new node - pivots: " << oss.str() << std::endl;
//
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//ErrorCode InternalNode<KeyType, ValueType, CacheType>::insert(const KeyType& key, const ValueType& value, CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
//{
//	int nChildIdx = 0;
//	while (nChildIdx < m_vtPivots.size() && key > m_vtPivots[nChildIdx]) {
//		nChildIdx++;
//	}
//
//	std::cout << "InternalNode::insert(" << key << "," << value << ") Idx: " << nChildIdx << std::endl;
//
//	INodePtr ptrParentNode = ptrCache->getObjectOfType<INode<KeyType, ValueType, CacheType>>(m_vtChildren[nChildIdx]);
//	if (ptrParentNode == NULL)
//		return ErrorCode::Error;
//
//	int nChildPivot = 0;
//	std::optional<CacheKeyType> ptrChildNode;
//	ErrorCode retCode = ptrParentNode->insert(key, value, ptrCache, ptrChildNode, nChildPivot);
//
//	if (ptrChildNode)
//	{
//		return insert(ptrCache, *ptrChildNode, nChildPivot, ptrSiblingNode, nSiblingPivot);
//	}
//
//	return ErrorCode::Success;
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//ErrorCode InternalNode<KeyType, ValueType, CacheType>::insert(CacheTypePtr ptrCache, CacheKeyType ptrChildNode, int nChildPivot, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
//{
//	int nChildIdx = m_vtPivots.size();
//	for (int nIdx = 0; nIdx < m_vtPivots.size(); ++nIdx) {
//		if (nChildPivot < m_vtPivots[nIdx]) {
//			nChildIdx = nIdx;
//			break;
//		}
//	}
//
//	std::cout << "InternalNode::insert_nodes at " << nChildIdx << std::endl;
//
//	m_vtPivots.insert(m_vtPivots.begin() + nChildIdx, nChildPivot);
//	m_vtChildren.insert(m_vtChildren.begin() + nChildIdx + 1, ptrChildNode);
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
//bool InternalNode<KeyType, ValueType, CacheType>::requireSplit()
//{
//	if (m_vtPivots.size() > m_nDegree)
//		return true;
//
//	return false;
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//ErrorCode InternalNode<KeyType, ValueType, CacheType>::split(CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot)
//{
//	std::cout << "InternalNode::split" << std::endl;
//
//	int nOffset = m_vtPivots.size() / 2;
//
//	nSiblingPivot = this->m_vtPivots[nOffset];
//	ptrSiblingNode = ptrCache->createObjectOfType<InternalNode<KeyType, ValueType, CacheType>>(5, this, nOffset);
//
//	//m_vtPivots.erase(m_vtPivots.begin() + nOffset, m_vtPivots.end());
//	//m_vtChildren.erase(m_vtChildren.begin() + nOffset, m_vtChildren.end());
//
//	m_vtPivots.resize(nOffset);
//	m_vtChildren.resize(nOffset+1);
//
//
//	std::ostringstream oss;
//	std::copy(m_vtPivots.begin(), m_vtPivots.end(), std::ostream_iterator<int>(oss, ","));
//	std::cout << "InternalNode::split - after - current node - pivots: " << oss.str() << "child count: " << m_vtChildren.size() << std::endl;
//
//	std::cout << "InternalNode::split pivot " << nSiblingPivot << std::endl;
//	return ErrorCode::Success;
//}
//
////template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
////ErrorCode InternalNode<KeyType, ValueType, CacheType>::remove(const KeyType& key) 
////{
////	size_t nIdx = getChildNodeIndex(key);
////	ErrorCode retCode = m_vtChildren[nIdx]->remove(key);
////
////	if (retCode == ErrorCode::Success)
////	{
////		size_t nChildCount = getChildCount();
////		if (nChildCount >= (m_nDegree / 2) - 1) {
////			return ErrorCode::Success;
////		}
////
////		rebalance(nChildCount);
////	}
////
////	return retCode;
////}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//size_t InternalNode<KeyType, ValueType, CacheType>::getChildNodeIndex(const KeyType& key)
//{
//	for (size_t nIdx = 0; nIdx < m_vtPivots.size(); ++nIdx) {
//		if (key < m_vtPivots[nIdx]) {
//			return nIdx;
//		}
//	}
//	return m_vtPivots.size();
//}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//ErrorCode InternalNode<KeyType, ValueType, CacheType>::search(CacheTypePtr ptrCache, const KeyType& key, ValueType& value)
//{
//	int nIdx= getChildNodeIndex(key);
//
//	INodePtr ptrChildNode = ptrCache->getObjectOfType<INode<KeyType, ValueType, CacheType>>(m_vtChildren[nIdx]);
//	if (ptrChildNode == NULL)
//		return ErrorCode::Error;
//
//	return ptrChildNode->search(ptrCache, key, value);
//}
//
////template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
////ErrorCode InternalNode<KeyType, ValueType, CacheType>::rebalance(size_t nChildNodeIdx) {
////	//if (m_vtPivots.size() >= (m_nDegree / 2) - 1) {
////	//	return ErrorCode::Success;
////	//}
////
////	//// Try to borrow a key from a sibling node.
////	//Node<KeyType, ValueType>* parent = node->parent;
////	//int index = getChildIndex(parent, node);
////
////	if (nChildNodeIdx > 0 && m_vtChildren[nChildNodeIdx - 1]->getChildCount() > (m_nDegree / 2) - 1) 
////	{
////		KeyType key;
////		//ErrorCode retCode = m_vtChildren[nChildNodeIdx]->moveAnEntityFromLHSSibling(m_vtChildren[nChildNodeIdx - 1], key);
////
////		//m_vtPivots[nChildNodeIdx - 1] = key;
////	}
////	else if (nChildNodeIdx < m_vtChildren.size() - 1 && m_vtChildren[nChildNodeIdx + 1]->getChildCount() > (m_nDegree / 2) - 1)
////	{
////		KeyType key;
////		//ErrorCode retCode = m_vtChildren[nChildNodeIdx]->moveAnEntityFromLHSSibling(m_vtChildren[nChildNodeIdx - 1], key);
////
////		//m_vtPivots[nChildNodeIdx - 1] = key;		
////	}
////	else {
////		if (nChildNodeIdx > 0) {
////			//m_vtChildren[nChildNodeIdx - 1]->mergeNodes(m_vtChildren[nChildNodeIdx]);
////
////			m_vtPivots.erase(m_vtPivots.begin() + nChildNodeIdx);
////			m_vtChildren.erase(m_vtChildren.begin() + nChildNodeIdx);
////		}
////		else {
////			//m_vtChildren[nChildNodeIdx]->mergeNodes(m_vtChildren[nChildNodeIdx + 1]);
////
////			m_vtPivots.erase(m_vtPivots.begin() + nChildNodeIdx + 1);
////			m_vtChildren.erase(m_vtChildren.begin() + nChildNodeIdx + 1);
////		}
////	}
////
////	return ErrorCode::Success;
////}
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//size_t InternalNode<KeyType, ValueType, CacheType>::getChildCount()
//{
//	return m_vtPivots.size();
//}
//
////template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
////ErrorCode InternalNode<KeyType, ValueType, CacheType>::moveAnEntityFromLHSSibling(INodePtr ptrLHSSibling, KeyType& oKey)
////{
////	InternalNode<KeyType, ValueType, CacheType>* lhsibling = static_cast<InternalNode<KeyType, ValueType, CacheType>*>(ptrLHSSibling);
////
////	KeyType key = lhsibling->m_vtPivots.back();
////	INodePtr node= lhsibling->m_vtChildren.back();
////
////	lhsibling->m_vtPivots.pop_back();
////	lhsibling->m_vtChildren.pop_back();
////
////	m_vtPivots.insert(m_vtPivots.begin(), key);
////	m_vtChildren.insert(m_vtChildren.begin(), node);
////
////	oKey = key;
////
////	return ErrorCode::Success;
////}
//
////template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
////ErrorCode InternalNode<KeyType, ValueType, CacheType>::moveAnEntityFromRHSSibling(INodePtr ptrRHSSibling, KeyType& oKey)
////{
////	InternalNode<KeyType, ValueType, CacheType>* rhssibling = static_cast<InternalNode<KeyType, ValueType, CacheType>*>(ptrRHSSibling);
////
////	KeyType key = rhssibling->m_vtPivots.front();
////	INodePtr node = rhssibling->m_vtChildren.front();
////
////	rhssibling->m_vtPivots.erase(rhssibling->m_vtPivots.begin());
////	rhssibling->m_vtChildren.erase(rhssibling->m_vtChildren.begin());
////
////	m_vtPivots.push_back(key);
////	m_vtChildren.push_back(node);
////
////	oKey = rhssibling->m_vtPivots.front();
////
////	return ErrorCode::Success;
////}
//
////template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
////ErrorCode InternalNode<KeyType, ValueType, CacheType>::mergeNodes(INodePtr ptrSiblingToMerge)
////{
////	InternalNode<KeyType, ValueType, CacheType>* ptrNode = static_cast<InternalNode<KeyType, ValueType, CacheType>*>(ptrSiblingToMerge);
////
////	m_vtPivots.insert(m_vtPivots.end(), ptrNode->m_vtPivots.begin(), ptrNode->m_vtPivots.end());
////	m_vtChildren.insert(m_vtChildren.end(), ptrNode->m_vtChildren.begin(), ptrNode->m_vtChildren.end());
////
////	delete ptrNode;
////
////	return ErrorCode::Success;
////}
//
//
//template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
//void InternalNode<KeyType, ValueType, CacheType>::print(CacheTypePtr ptrCache, int nLevel)
//{
//	std::ostringstream oss;
//
//	if (!m_vtPivots.empty())
//	{
//		// Convert all but the last element to avoid a trailing ","
//		std::copy(m_vtPivots.begin(), m_vtPivots.end() - 1,
//			std::ostream_iterator<int>(oss, ","));
//
//		// Now add the last element with no delimiter
//		oss << m_vtPivots.back();
//	}
//
//	printf("%s> level: %d\n", std::string(nLevel, '.').c_str(), nLevel);
//	printf("%s  pivots: %s\n", std::string(nLevel, ' ').c_str(), oss.str().c_str());
//
//	for (int i = 0; i < m_vtChildren.size(); i++)
//	{
//		printf("%s  child: %d\n", std::string(nLevel, ' ').c_str(), i);
//
//
//		INodePtr ptrChildNode = ptrCache->getObjectOfType<INode<KeyType, ValueType, CacheType>>(m_vtChildren[i]);
//		if (ptrChildNode == NULL)
//			return;
//
//		ptrChildNode->print(ptrCache, nLevel + 1);
//	}
//}