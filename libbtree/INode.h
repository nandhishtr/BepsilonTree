#pragma once
#include "ErrorCodes.h"
#include "ICacheObject.h"
#include <optional>
#include <memory>
#include "DRAMCacheObject.h"

template <typename KeyType, typename ValueType, template <typename, typename> typename CacheType, typename CacheKeyType, typename CacheValueType>
class INode : public DRAMCacheObject
{
	typedef std::shared_ptr<CacheType<CacheKeyType, CacheValueType>> CacheTypePtr;
	typedef std::shared_ptr<INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>> INodePtr;

public:
	virtual void wieHiestDu() = 0;

	/*virtual int insert(int key, int value, INode*& ptrChild, int& nPivot) = 0;
	virtual int insert(INode*& ptrChild, int nChildPivot, INode*& ptrSibling, int& nSiblingPivot) = 0;

	virtual void print(int level) = 0;*/

	//virtual ErrorCode remove(const KeyType& key) = 0;
	virtual ErrorCode search(CacheTypePtr ptrCache, const KeyType& key, ValueType& value) = 0;

	virtual ErrorCode insert(const KeyType& key, const ValueType& value, CacheTypePtr ptrCache, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot) = 0;
	virtual ErrorCode insert(CacheTypePtr ptrCache, CacheKeyType ptrChildNode, int nChildPivot, std::optional<CacheKeyType>& ptrSiblingNode, int& nSiblingPivot) = 0;
	//virtual ErrorCode insert(const KeyType& key, const ValueType& value, CacheType* ptrCache, INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>*& ptrSiblingNode, int& nSiblingPivot) = 0;
	//virtual ErrorCode insert(INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>*& ptrChildNode, int nChildPivot, INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>*& ptrSiblingNode, int& nSiblingPivot) = 0;

public:
	//virtual size_t getChildCount() = 0;	
	//virtual ErrorCode moveAnEntityFromLHSSibling(INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>* ptrLHSSibling, KeyType& oKey) = 0;
	//virtual ErrorCode moveAnEntityFromRHSSibling(INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>* ptrRHSSibling, KeyType& oKey) = 0;

	//virtual ErrorCode mergeNodes(INode<KeyType, ValueType, CacheType, CacheKeyType, CacheValueType>* ptrSiblingToMerge) = 0;

	virtual void print(CacheTypePtr ptrCache, int nLevel) = 0;
};

//template <typename KeyType, typename ValueType, typename CacheType>
