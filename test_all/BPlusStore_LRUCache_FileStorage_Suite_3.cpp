#include "pch.h"
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <typeinfo>
#include <type_traits>

#include "glog/logging.h"

#include "LRUCache.hpp"
#include "IndexNode.hpp"
#include "DataNode.hpp"
#include "BPlusStore.hpp"
#include "LRUCacheObject.hpp"
#include "FileStorage.hpp"
#include "TypeMarshaller.hpp"

#include "TypeUID.h"
#include "ObjectFatUID.h"

#ifdef __POSITION_AWARE_ITEMS__
namespace BPlusStore_LRUCache_FileStorage_Suite
{
    typedef int KeyType;
    typedef int ValueType;
    typedef ObjectFatUID ObjectUIDType;

    typedef IFlushCallback<ObjectUIDType> ICallback;

    typedef DataNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::DATA_NODE_INT_INT > DataNodeType;
    typedef IndexNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::INDEX_NODE_INT_INT> InternalNodeType;

    typedef IFlushCallback<ObjectUIDType> ICallback;
    
    typedef BPlusStore<ICallback, KeyType, ValueType, LRUCache<ICallback, FileStorage<ICallback, ObjectUIDType, LRUCacheObject, TypeMarshaller, DataNodeType, InternalNodeType>>> BPlusStoreType;

    class BPlusStore_LRUCache_FileStorage_Suite_3 : public ::testing::TestWithParam<std::tuple<int, int, int, int, int, int, string>>
    {
    protected:
        BPlusStoreType* m_ptrTree;

        void SetUp() override
        {
            std::tie(nDegree, nThreadCount, nTotalEntries, nCacheSize, nBlockSize, nFileSize, stFileName) = GetParam();

            //m_ptrTree = new BPlusStoreType(3);
            //m_ptrTree->template init<DataNodeType>();
        }

        void TearDown() override {
            //delete m_ptrTree;
        }

        int nDegree;
        int nThreadCount;
        int nTotalEntries;
        int nCacheSize;
        int nBlockSize;
        int nFileSize;
        string stFileName;
    };

    void insert_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
        for (size_t nCntr = nRangeStart; nCntr < nRangeEnd; nCntr++)
        {
            ptrTree->insert(nCntr, nCntr);
        }
    }

    void reverse_insert_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
        for (int nCntr = nRangeEnd - 1; nCntr >= nRangeStart; nCntr--)
        {
            ptrTree->insert(nCntr, nCntr);
        }
    }

    void search_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
        for (size_t nCntr = nRangeStart; nCntr < nRangeEnd; nCntr++)
        {
            int nValue = 0;
            ErrorCode code = ptrTree->search(nCntr, nValue);

            ASSERT_EQ(nCntr, nValue);
        }
    }

    void search_not_found_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
        for (size_t nCntr = nRangeStart; nCntr < nRangeEnd; nCntr++)
        {
            int nValue = 0;
            ErrorCode errCode = ptrTree->search(nCntr, nValue);

            ASSERT_EQ(errCode, ErrorCode::KeyDoesNotExist);
        }
    }

    void delete_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
        for (size_t nCntr = nRangeStart; nCntr < nRangeEnd; nCntr++)
        {
            ErrorCode code = ptrTree->remove(nCntr);
        }
    }

    void reverse_delete_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
        for (int nCntr = nRangeEnd - 1; nCntr >= nRangeStart; nCntr--)
        {
            ErrorCode code = ptrTree->remove(nCntr);
        }
    }

    TEST_P(BPlusStore_LRUCache_FileStorage_Suite_3, Bulk_Insert_v1) {

        BPlusStoreType* ptrTree = new BPlusStoreType(nDegree, nCacheSize, nBlockSize, nFileSize, stFileName);
        ptrTree->template init<DataNodeType>();

        std::vector<std::thread> vtThreads;

        for (int nIdx = 0; nIdx < nThreadCount; nIdx++)
        {
            int nTotal = nTotalEntries / nThreadCount;
            vtThreads.push_back(std::thread(insert_concurent, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        auto it = vtThreads.begin();
        while (it != vtThreads.end())
        {
            (*it).join();
            it++;
        }

        delete ptrTree;
    }

    TEST_P(BPlusStore_LRUCache_FileStorage_Suite_3, Bulk_Insert_v2) {

        BPlusStoreType* ptrTree = new BPlusStoreType(nDegree, nCacheSize, nBlockSize, nFileSize, stFileName);
        ptrTree->template init<DataNodeType>();

        std::vector<std::thread> vtThreads;

        for (int nIdx = 0; nIdx < nThreadCount; nIdx++)
        {
            int nTotal = nTotalEntries / nThreadCount;
            vtThreads.push_back(std::thread(reverse_insert_concurent, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        auto it = vtThreads.begin();
        while (it != vtThreads.end())
        {
            (*it).join();
            it++;
        }

        delete ptrTree;
    }

    TEST_P(BPlusStore_LRUCache_FileStorage_Suite_3, Bulk_Search_v1) {

        BPlusStoreType* ptrTree = new BPlusStoreType(nDegree, nCacheSize, nBlockSize, nFileSize, stFileName);
        ptrTree->template init<DataNodeType>();

        std::vector<std::thread> vtThreads;

        for (int nIdx = 0; nIdx < nThreadCount; nIdx++)
        {
            int nTotal = nTotalEntries / nThreadCount;
            vtThreads.push_back(std::thread(insert_concurent, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        auto it = vtThreads.begin();
        while (it != vtThreads.end())
        {
            (*it).join();
            it++;
        }

        vtThreads.clear();

        for (int nIdx = 0; nIdx < nThreadCount; nIdx++)
        {
            int nTotal = nTotalEntries / nThreadCount;
            vtThreads.push_back(std::thread(search_concurent, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        it = vtThreads.begin();
        while (it != vtThreads.end())
        {
            (*it).join();
            it++;
        }

        delete ptrTree;
    }


    TEST_P(BPlusStore_LRUCache_FileStorage_Suite_3, Bulk_Search_v2) {

        BPlusStoreType* ptrTree = new BPlusStoreType(nDegree, nCacheSize, nBlockSize, nFileSize, stFileName);
        ptrTree->template init<DataNodeType>();

        std::vector<std::thread> vtThreads;

        for (int nIdx = 0; nIdx < nThreadCount; nIdx++)
        {
            int nTotal = nTotalEntries / nThreadCount;
            vtThreads.push_back(std::thread(reverse_insert_concurent, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        auto it = vtThreads.begin();
        while (it != vtThreads.end())
        {
            (*it).join();
            it++;
        }

        vtThreads.clear();

        for (int nIdx = 0; nIdx < nThreadCount; nIdx++)
        {
            int nTotal = nTotalEntries / nThreadCount;
            vtThreads.push_back(std::thread(search_concurent, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        it = vtThreads.begin();
        while (it != vtThreads.end())
        {
            (*it).join();
            it++;
        }

        delete ptrTree;
    }

    TEST_P(BPlusStore_LRUCache_FileStorage_Suite_3, Bulk_Delete_v1) {

        BPlusStoreType* ptrTree = new BPlusStoreType(nDegree, nCacheSize, nBlockSize, nFileSize, stFileName);
        ptrTree->template init<DataNodeType>();

        std::vector<std::thread> vtThreads;

        for (int nIdx = 0; nIdx < nThreadCount; nIdx++)
        {
            int nTotal = nTotalEntries / nThreadCount;
            vtThreads.push_back(std::thread(insert_concurent, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        auto it = vtThreads.begin();
        while (it != vtThreads.end())
        {
            (*it).join();
            it++;
        }

        vtThreads.clear();

        for (int nIdx = 0; nIdx < nThreadCount; nIdx++)
        {
            int nTotal = nTotalEntries / nThreadCount;
            vtThreads.push_back(std::thread(delete_concurent, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        it = vtThreads.begin();
        while (it != vtThreads.end())
        {
            (*it).join();
            it++;
        }

        vtThreads.clear();

        for (int nIdx = 0; nIdx < nThreadCount; nIdx++)
        {
            int nTotal = nTotalEntries / nThreadCount;
            vtThreads.push_back(std::thread(search_not_found_concurent, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        it = vtThreads.begin();
        while (it != vtThreads.end())
        {
            (*it).join();
            it++;
        }

        delete ptrTree;
    }

    TEST_P(BPlusStore_LRUCache_FileStorage_Suite_3, Bulk_Delete_v2) {

        BPlusStoreType* ptrTree = new BPlusStoreType(nDegree, nCacheSize, nBlockSize, nFileSize, stFileName);
        ptrTree->template init<DataNodeType>();

        std::vector<std::thread> vtThreads;

        for (int nIdx = 0; nIdx < nThreadCount; nIdx++)
        {
            int nTotal = nTotalEntries / nThreadCount;
            vtThreads.push_back(std::thread(reverse_insert_concurent, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        auto it = vtThreads.begin();
        while (it != vtThreads.end())
        {
            (*it).join();
            it++;
        }

        vtThreads.clear();

        for (int nIdx = 0; nIdx < nThreadCount; nIdx++)
        {
            int nTotal = nTotalEntries / nThreadCount;
            vtThreads.push_back(std::thread(reverse_delete_concurent, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        it = vtThreads.begin();
        while (it != vtThreads.end())
        {
            (*it).join();
            it++;
        }

        vtThreads.clear();

        for (int nIdx = 0; nIdx < nThreadCount; nIdx++)
        {
            int nTotal = nTotalEntries / nThreadCount;
            vtThreads.push_back(std::thread(search_not_found_concurent, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        it = vtThreads.begin();
        while (it != vtThreads.end())
        {
            (*it).join();
            it++;
        }

        delete ptrTree;
    }

#ifdef __CONCURRENT__
    INSTANTIATE_TEST_CASE_P(
        Bulk_Insert_Search_Delete,
        BPlusStore_LRUCache_FileStorage_Suite_3,
        ::testing::Values(
            std::make_tuple(3, 10, 99999, 100, 1024, 1024, ""),
            std::make_tuple(4, 10, 99999, 100, 1024, 1024, ""),
            std::make_tuple(5, 10, 99999, 100, 1024, 1024, ""),
            std::make_tuple(6, 10, 99999, 100, 1024, 1024, ""),
            std::make_tuple(7, 10, 99999, 100, 1024, 1024, ""),
            std::make_tuple(8, 10, 99999, 100, 1024, 1024, ""),
            std::make_tuple(15, 10, 199999, 100, 1024, 1024, ""),
            std::make_tuple(16, 10, 199999, 100, 1024, 1024, ""),
            std::make_tuple(32, 10, 199999, 100, 1024, 1024, ""),
            std::make_tuple(64, 10, 199999, 100, 1024, 1024, "")));
#endif __CONCURRENT__
}
#endif __POSITION_AWARE_ITEMS__