#pragma once
#include <iostream>
#include "LeafNode.h"
#include "InternalNode.h"

class ICacheManager
{
//	typedef std::shared_ptr<CacheType<CacheKeyType, CacheValueType>> CacheTypePtr;
//	typedef std::shared_ptr<INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>> INodePtr;
//	typedef std::shared_ptr<LeafNode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>> LeafNodePtr;
//	typedef std::shared_ptr<InternalNode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>> InternalNodePtr;
//
//
//public:
//	virtual createInternalNode() = 0;
//	virtual createLeafNode() = 0;
//	virtual getInternalNode() = 0;
//	virtual getLeafNode() = 0;
//
//	template<typename KeyType, class ClassType>
//	std::shared_ptr<ClassType> getObjectOfType(KeyType& objKey/*, Hints for nvm!!*/)
//	{
//		std::cout << "ICacheManager::getObjectOfType" << std::endl;
//
//		return nullptr;
//	}
//
//	template<typename KeyType, class ClassType, typename... ClassArgsType>
//	KeyType createObjectOfType(ClassArgsType ... args)
//	{
//		std::cout << "ICacheManager::createObjectOfType" << std::endl;
//
//		return NULL;
//	}
};