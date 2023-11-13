#include "pch.h"

#include "gtest/gtest.h"
#include "BTree.h"
#include "NVRAMLRUCache.h"
#include "NVRAMCacheObject.h"

#include "NVRAMLRUCache.h"
#include "NVRAMCacheObject.h"
#include "NVRAMCacheObjectKey.h"

class BPlusTree_TestSuite1 : public ::testing::Test {
protected:
    //BTree<int, int, NVRAMLRUCache, uintptr_t, NVRAMCacheObject>* m_objBPlusTree;
    BTree<int, int, NVRAMLRUCache, uintptr_t, NVRAMCacheObject, INVRAMCacheObject>* m_objBPlusTree;


    void SetUp() override {
        //m_objBPlusTree = new BTree<int, int, NVRAMLRUCache, uintptr_t, NVRAMCacheObject>();
        m_objBPlusTree = new BTree<int, int, NVRAMLRUCache, uintptr_t, NVRAMCacheObject, INVRAMCacheObject>();
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
    for (size_t element = 0; element < 100; element++)
    {
        m_objBPlusTree->insert(element, element);
    }

    for (size_t element = 1; element < 100; element++)
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