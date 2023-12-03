#include "pch.h"

#include <iostream>

#include "gtest/gtest.h"
#include "BPlusTree.hpp"
#include "NoCache.h"

class BPlusTree_NoCache_Suite_2 : public ::testing::Test
{
protected:
    BPlusTree<int, string, NoCache<uintptr_t, shared_ptr<LeafNodeEx<int, string>>, shared_ptr<InternalNodeEx<int, string, uintptr_t>>>>* m_ptrTree;

    void SetUp() override 
    {
        m_ptrTree = new BPlusTree<int, string, NoCache<uintptr_t, shared_ptr<LeafNodeEx<int, string>>, shared_ptr<InternalNodeEx<int, string, uintptr_t>>>>(5);
    }

    void TearDown() override {
        delete m_ptrTree;
    }
};

TEST_F(BPlusTree_NoCache_Suite_2, Insert_1_Element) {
    m_ptrTree->insert(1, "1");

    string nValue;
    ErrorCode code = m_ptrTree->search(1, nValue);

    ASSERT_EQ(nValue, "1");
}

TEST_F(BPlusTree_NoCache_Suite_2, Insert_100_Elements_v1) {
    for (size_t nCntr = 0; nCntr < 1000; nCntr++)
    {
        m_ptrTree->insert(nCntr, to_string(nCntr));
    }

    for (size_t nCntr = 0; nCntr < 1000; nCntr++)
    {
        string nValue;
        ErrorCode code = m_ptrTree->search(nCntr, nValue);

        ASSERT_EQ(nValue, to_string(nCntr));
    }
}

TEST_F(BPlusTree_NoCache_Suite_2, Insert_100_Elements_v2) {
    for (size_t nCntr = 50; nCntr < 100; nCntr++)
    {
        m_ptrTree->insert(nCntr, to_string(nCntr));
    }

    for (size_t nCntr = 50; nCntr < 100; nCntr++)
    {
        string nValue;
        ErrorCode code = m_ptrTree->search(nCntr, nValue);

        ASSERT_EQ(nValue, to_string(nCntr));
    }

    for (size_t nCntr = 0; nCntr < 50; nCntr++)
    {
        m_ptrTree->insert(nCntr, to_string(nCntr));
    }

    for (size_t nCntr = 0; nCntr < 50; nCntr++)
    {
        string nValue;
        ErrorCode code = m_ptrTree->search(nCntr, nValue);

        ASSERT_EQ(nValue, to_string(nCntr));
    }
}

TEST_F(BPlusTree_NoCache_Suite_2, Insert_100_Elements_v3) {
    for (size_t nCntr = 0; nCntr < 1000; nCntr++)
    {
        m_ptrTree->insert(nCntr, to_string(nCntr * 2));
    }

    for (size_t nCntr = 0; nCntr < 1000; nCntr++)
    {
        string nValue;
        ErrorCode code = m_ptrTree->search(nCntr, nValue);

        ASSERT_EQ(nValue, to_string(nCntr * 2));
    }
}

TEST_F(BPlusTree_NoCache_Suite_2, Insert_100_Elements_v4) {
    for (size_t nCntr = 0; nCntr < 1000; nCntr += 2)
    {
        m_ptrTree->insert(nCntr, to_string(nCntr * 2));
    }

    for (size_t nCntr = 0; nCntr < 1000; nCntr += 2)
    {
        string nValue;
        ErrorCode code = m_ptrTree->search(nCntr, nValue);

        ASSERT_EQ(nValue, to_string(nCntr * 2));
    }
}
