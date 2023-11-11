#include "pch.h"

#include "gtest/gtest.h"
#include "BTree.h"
#include "DRAMLRUCache.h"
#include "DRAMCacheObject.h"

using namespace std;

class BPlusTree_TestSuite2 : public ::testing::Test {
protected:
    BTree<int, string, DRAMLRUCache, uintptr_t, DRAMCacheObject>* m_objBPlusTree;


    void SetUp() override {
        m_objBPlusTree = new BTree<int, string, DRAMLRUCache, uintptr_t, DRAMCacheObject>();
        m_objBPlusTree->init();
    }

    void TearDown() override {
        delete m_objBPlusTree;
    }
};

TEST_F(BPlusTree_TestSuite2, Insert1Element) {
    m_objBPlusTree->insert(1, "1");

    string value = "";
    ErrorCode code = m_objBPlusTree->search(1, value);

    ASSERT_EQ("1", value);
}

TEST_F(BPlusTree_TestSuite2, Insert100Elements_V1) {
    for (size_t element = 0; element < 1000; element++)
    {
        m_objBPlusTree->insert(element, to_string(element));
    }

    for (size_t element = 1; element < 1000; element++)
    {
        string value = "";
        ErrorCode code = m_objBPlusTree->search(element, value);

        ASSERT_EQ(to_string(element), value);
    }
}

TEST_F(BPlusTree_TestSuite2, Insert100Elements_V2) {
    for (size_t element = 50; element < 100; element++)
    {
        m_objBPlusTree->insert(element, to_string(element));
    }

    for (size_t element = 50; element < 100; element++)
    {
        string value = "";
        ErrorCode code = m_objBPlusTree->search(element, value);

        ASSERT_EQ(to_string(element), value);
    }

    for (size_t element = 0; element < 50; element++)
    {
        m_objBPlusTree->insert(element, to_string(element));
    }

    for (size_t element = 1; element < 50; element++)
    {
        string value = "";
        ErrorCode code = m_objBPlusTree->search(element, value);

        ASSERT_EQ(to_string(element), value);
    }
}