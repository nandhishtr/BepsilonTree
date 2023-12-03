#include "pch.h"

#include <iostream>

#include "gtest/gtest.h"
#include "BPlusTree.hpp"
#include "NoCache.h"

class BPlusTree_NoCache_Suite_1 : public ::testing::Test
{
protected:
    BPlusTree<int, int, NoCache<uintptr_t, shared_ptr<LeafNodeEx<int, int>>, shared_ptr<InternalNodeEx<int, int, uintptr_t>>>>* m_ptrTree;

    void SetUp() override 
    {
        m_ptrTree = new BPlusTree<int, int, NoCache<uintptr_t, shared_ptr<LeafNodeEx<int, int>>, shared_ptr<InternalNodeEx<int, int, uintptr_t>>>>(5);
    }

    void TearDown() override {
        delete m_ptrTree;
    }
};

TEST_F(BPlusTree_NoCache_Suite_1, Insert_1_Element) {
    m_ptrTree->insert(1, 1);

    int nValue;
    ErrorCode code = m_ptrTree->search(1, nValue);

    ASSERT_EQ(nValue, 1);
}

TEST_F(BPlusTree_NoCache_Suite_1, Insert_100_Elements_v1) {
    for (size_t nCntr = 0; nCntr < 1000; nCntr++)
    {
        m_ptrTree->insert(nCntr, nCntr);
    }

    for (size_t nCntr = 0; nCntr < 1000; nCntr++)
    {
        int nValue = 0;
        ErrorCode code = m_ptrTree->search(nCntr, nValue);

        ASSERT_EQ(nValue, nCntr);
    }
}

TEST_F(BPlusTree_NoCache_Suite_1, Insert_100_Elements_v2) {
    for (size_t nCntr = 50; nCntr < 100; nCntr++)
    {
        m_ptrTree->insert(nCntr, nCntr);
    }

    for (size_t nCntr = 50; nCntr < 100; nCntr++)
    {
        int nValue = 0;
        ErrorCode code = m_ptrTree->search(nCntr, nValue);

        ASSERT_EQ(nValue, nCntr);
    }

    for (size_t nCntr = 0; nCntr < 50; nCntr++)
    {
        m_ptrTree->insert(nCntr, nCntr);
    }

    for (size_t nCntr = 0; nCntr < 50; nCntr++)
    {
        int nValue = 0;
        ErrorCode code = m_ptrTree->search(nCntr, nValue);

        ASSERT_EQ(nValue, nCntr);
    }
}

TEST_F(BPlusTree_NoCache_Suite_1, Insert_100_Elements_v3) {
    for (size_t nCntr = 0; nCntr < 1000; nCntr++)
    {
        m_ptrTree->insert(nCntr, nCntr * 2);
    }

    for (size_t nCntr = 0; nCntr < 1000; nCntr++)
    {
        int nValue = 0;
        ErrorCode code = m_ptrTree->search(nCntr, nValue);

        ASSERT_EQ(nValue, nCntr * 2);
    }
}

TEST_F(BPlusTree_NoCache_Suite_1, Insert_100_Elements_v4) {
    for (size_t nCntr = 0; nCntr < 1000; nCntr+=2)
    {
        m_ptrTree->insert(nCntr, nCntr * 2);
    }

    for (size_t nCntr = 0; nCntr < 1000; nCntr+=2)
    {
        int nValue = 0;
        ErrorCode code = m_ptrTree->search(nCntr, nValue);

        ASSERT_EQ(nValue, nCntr * 2);
    }
}
