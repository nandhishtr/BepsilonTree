#include "pch.h"

#include "gtest/gtest.h"
#include "BTree.h"
#include "NVRAMLRUCache.h"
#include "NVRAMCacheObject.h"

#include "NVRAMLRUCache.h"
#include "NVRAMCacheObject.h"
#include "NVRAMCacheObjectKey.h"
#include "NVRAMVolatileStorage.h"
#include <iostream>
//#include "NVRAMStorage.h"

template <typename X, typename... T>
class D {
public:
    const std::size_t n = sizeof...(T);

    D() {
        std::cout << n << std::endl;
    }
};

template <typename KeyType>
class C {
public:
    C() {
        std::cout << "C" << std::endl;
    }
};

template <typename KeyType, typename ValueCoreType>
class B {
public:
    B() {
        std::cout << "C" << std::endl;
    }

};

template <template <typename, typename> typename NVRAMStorage, typename KeyType, typename ValueCoreType, template <typename ValueCoreType, typename... > typename ValueType, typename... ValueCoreOtherTypes>
class A {
public:
    typedef KeyType TypeKeyType;

    NVRAMStorage<KeyType, ValueCoreType>* a;
    ValueType<ValueCoreType, ValueCoreOtherTypes...>* aa;

    A() {
        std::cout << "A" << std::endl;

         a = new    NVRAMStorage<KeyType, ValueCoreType>();
         aa = new   ValueType<ValueCoreType, ValueCoreOtherTypes...>();
    }
};

class BPlusTree_TestSuite1 : public ::testing::Test {
protected:
    //BTree<int, int, B, void, void, void, void> _a;
    //BTree<int, int, NVRAMLRUCache, uintptr_t, NVRAMCacheObject>* m_objBPlusTree;
    BTree<int, int, NVRAMLRUCache<NVRAMVolatileStorage, uintptr_t, INVRAMCacheObject, NVRAMCacheObject2, int>>* m_objBPlusTree;

    //BTree<int, int, NVRAMLRUCache<>, uintptr_t, INVRAMCacheObject, NVRAMVolatileStorage, NVRAMCacheObject2, ...>* m_objBPlusTree;

    void SetUp() override {

        A<B, int, int, C>* a = new A<B, int, int, C>();
        A<B, int, int, D, int>* aa = new A<B, int, int, D, int>();


        //m_objBPlusTree = new BTree<int, int, NVRAMLRUCache, uintptr_t, NVRAMCacheObject>();
        m_objBPlusTree = new BTree<int, int, NVRAMLRUCache<NVRAMVolatileStorage, uintptr_t, INVRAMCacheObject, NVRAMCacheObject2, int>>();
        m_objBPlusTree->init();
    }

    void TearDown() override {
        delete m_objBPlusTree;
    }
};

TEST_F(BPlusTree_TestSuite1, Insert1Element) {
    m_objBPlusTree->insert(1, 1);

    int value;
    ErrorCode code = m_objBPlusTree->search(1, value);

    ASSERT_EQ(value, 1);
}

TEST_F(BPlusTree_TestSuite1, Insert100Elements_V1) {
    for (size_t element = 0; element < 1000; element++)
    {
        m_objBPlusTree->insert(element, element);
    }

    for (size_t element = 1; element < 1000; element++)
    {
        int value = 0;
        ErrorCode code = m_objBPlusTree->search(element, value);

        ASSERT_EQ(value, element);
    }
}

TEST_F(BPlusTree_TestSuite1, Insert100Elements_V2) {
    for (size_t element = 50; element < 100; element++)
    {
        m_objBPlusTree->insert(element, element);
    }

    for (size_t element = 50; element < 100; element++)
    {
        int value = 0;
        ErrorCode code = m_objBPlusTree->search(element, value);

        ASSERT_EQ(value, element);
    }

    for (size_t element = 0; element < 50; element++)
    {
        m_objBPlusTree->insert(element, element);
    }

    for (size_t element = 1; element < 50; element++)
    {
        int value = 0;
        ErrorCode code = m_objBPlusTree->search(element, value);

        ASSERT_EQ(value, element);
    }
}