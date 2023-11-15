#include "pch.h"

#include "gtest/gtest.h"
#include "BTree.h"
#include "DRAMLRUCache.h"
#include "DRAMCacheObject.h"
#include "DRAMCacheObjectKey.h"

#include "NVRAMLRUCache.h"
#include "NVRAMCacheObject.h"
#include "NVRAMCacheObjectKey.h"

#include "INode.h"
//template <typename T>
//class A {
//public:
//    class A()
//    {}
//};
//
//TEST(a, b)
//{
//    // send betree as type.. to create objects...
//
//    //BTree<int, int, DRAMLRUCache, DRAMCacheObjectKey, INode<int, int, DRAMLRUCache, DRAMCacheObjectKey, INode<...>> >* btree_ = new BTree<int, int, DRAMLRUCache, DRAMCacheObjectKey, INode<int, int, DRAMLRUCache, DRAMCacheObjectKey, void>>();
//
//     
//    DRAMLRUCache<int, A, int> x(1);
//    
//    BTree<int, int, DRAMLRUCache, uintptr_t, DRAMCacheObject,IDRAMCacheObject>* btree = new BTree<int, int, DRAMLRUCache, uintptr_t, DRAMCacheObject, IDRAMCacheObject>();
//    //BTree<int, int, NVRAMLRUCache, NVRAMCacheObjectKey, NVRAMCacheObject>* btree = new BTree<int, int, NVRAMLRUCache, NVRAMCacheObjectKey, NVRAMCacheObject>();
//
//    btree->init();
//
//    for (size_t i = 50; i < 100; i++)
//    {
//        btree->insert(i, i);
//    }
//
//    //btree->print();
//
//    for (size_t i = 50; i < 100; i++)
//    {
//        int value = 0;
//        btree->search(i, value);
//
//        EXPECT_EQ(i, value);
//    }
//
//}
//
//TEST(TestCaseName, TestName) {
//  EXPECT_EQ(1, 1);
//  EXPECT_TRUE(true);
//}