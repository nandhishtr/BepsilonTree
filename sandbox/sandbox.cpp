#include <iostream>

#include "BTree.h"
#include "BPlusStore.hpp"
#include "NoCache.h"
#include "glog/logging.h"
#include <type_traits>

#include <variant>
#include <typeinfo>

#include "DataNode.hpp"
#include "IndexNode.hpp"

#include <chrono>
#include <cassert>

#include "LRUCache.hpp"
#include "VolatileStorage.hpp"
#include "NoCacheObject.hpp"
#include "LRUCacheObject.hpp"


typedef int KeyType;
typedef int ValueType;
typedef uintptr_t CacheKeyType;

typedef DataNode<KeyType, ValueType> DataNodeType;
typedef IndexNode<KeyType, ValueType, CacheKeyType> IndexNodeType;

//typedef BPlusStore<KeyType, ValueType, NoCache<CacheKeyType, NoCacheObject, shared_ptr<DataNodeType>, shared_ptr<IndexNodeType>>> BPlusStoreType;

typedef BPlusStore<KeyType, ValueType, LRUCache<VolatileStorage, CacheKeyType, LRUCacheObject, shared_ptr<DataNodeType>, shared_ptr<IndexNodeType>>> BPlusStoreType;

void insert_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
    for (size_t nCntr = nRangeStart; nCntr < nRangeEnd; nCntr++)
    {
        ptrTree->template insert<IndexNodeType, DataNodeType>(nCntr, nCntr);
    }
}

void reverse_insert_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
    for (int nCntr = nRangeEnd - 1; nCntr >= nRangeStart; nCntr--)
    {
        ptrTree->template insert<IndexNodeType, DataNodeType>(nCntr, nCntr);
    }
}

void search_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
    for (size_t nCntr = nRangeStart; nCntr < nRangeEnd; nCntr++)
    {
        int nValue = 0;
        ErrorCode code = ptrTree->template search<IndexNodeType, DataNodeType>(nCntr, nValue);

        assert(nCntr == nValue);
    }
}

void search_not_found_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
    for (size_t nCntr = nRangeStart; nCntr < nRangeEnd; nCntr++)
    {
        int nValue = 0;
        ErrorCode errCode = ptrTree->template search<IndexNodeType, DataNodeType>(nCntr, nValue);

        assert(errCode == ErrorCode::KeyDoesNotExist);
    }
}

void delete_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
    for (size_t nCntr = nRangeStart; nCntr < nRangeEnd; nCntr++)
    {
        ErrorCode code = ptrTree->template remove<IndexNodeType, DataNodeType>(nCntr);
    }
}

void reverse_delete_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
    for (int nCntr = nRangeEnd - 1; nCntr >= nRangeStart; nCntr--)
    {
        ErrorCode code = ptrTree->template remove<IndexNodeType, DataNodeType>(nCntr);
    }
}


void threaded_test(int degree, int total_entries, int thread_count)
{
    vector<std::thread> vtThreads;

    typedef int KeyType;
    typedef int ValueType;
    typedef uintptr_t CacheKeyType;

    typedef DataNode<KeyType, ValueType> DataNodeType;
    typedef IndexNode<KeyType, ValueType, CacheKeyType> IndexNodeType;

    //typedef BPlusStore<KeyType, ValueType, NoCache<CacheKeyType, NoCacheObject, shared_ptr<DataNodeType>, shared_ptr<IndexNodeType>>> BPlusStoreType;
    //BPlusStoreType* ptrTree = new BPlusStoreType(degree);

    typedef BPlusStore<KeyType, ValueType, LRUCache<VolatileStorage, CacheKeyType, LRUCacheObject, shared_ptr<DataNodeType>, shared_ptr<IndexNodeType>>> BPlusStoreType;
    BPlusStoreType* ptrTree = new BPlusStoreType(degree, 10000, 10000000);

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    ptrTree->template init<DataNodeType>();

    int i = 0;
    while (i++ < 10) {
        std::cout << i << std::endl;

        for (int nIdx = 0; nIdx < thread_count; nIdx++)
        {
            int nTotal = total_entries / thread_count;
            vtThreads.push_back(std::thread(insert_concurent, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        auto it = vtThreads.begin();
        while (it != vtThreads.end())
        {
            (*it).join();
            it++;
        }

        vtThreads.clear();

        for (int nIdx = 0; nIdx < thread_count; nIdx++)
        {
            int nTotal = total_entries / thread_count;
            vtThreads.push_back(std::thread(search_concurent, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        it = vtThreads.begin();
        while (it != vtThreads.end())
        {
            (*it).join();
            it++;
        }

        vtThreads.clear();

        for (int nIdx = 0; nIdx < thread_count; nIdx++)
        {
            int nTotal = total_entries / thread_count;
            vtThreads.push_back(std::thread(delete_concurent, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        it = vtThreads.begin();
        while (it != vtThreads.end())
        {
            (*it).join();
            it++;
        }

        vtThreads.clear();

        for (int nIdx = 0; nIdx < thread_count; nIdx++)
        {
            int nTotal = total_entries / thread_count;
            vtThreads.push_back(std::thread(search_not_found_concurent, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        it = vtThreads.begin();
        while (it != vtThreads.end())
        {
            (*it).join();
            it++;
        }

        vtThreads.clear();
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << "[ns]" << std::endl;
}

void int_test(int degree, int total_entries)
{
    typedef int KeyType;
    typedef int ValueType;
    typedef uintptr_t CacheKeyType;

    typedef DataNode<KeyType, ValueType> DataNodeType;
    typedef IndexNode<KeyType, ValueType, CacheKeyType> IndexNodeType;

    //typedef BPlusStore<KeyType, ValueType, NoCache<CacheKeyType, NoCacheObject, shared_ptr<DataNodeType>, shared_ptr<IndexNodeType>>> BPlusStoreType;
    //BPlusStoreType* ptrTree = new BPlusStoreType(degree);

    typedef BPlusStore<KeyType, ValueType, LRUCache<VolatileStorage, CacheKeyType, LRUCacheObject, shared_ptr<DataNodeType>, shared_ptr<IndexNodeType>>> BPlusStoreType;
    BPlusStoreType* ptrTree = new BPlusStoreType(degree, 10, 10000000);

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    ptrTree->template init<DataNodeType>();

    int i = 0;
    while (i++ < 10) {
        std::cout << i << std::endl;
        for (size_t nCntr = 0; nCntr < total_entries; nCntr = nCntr + 2)
        {
            ptrTree->template insert<IndexNodeType, DataNodeType>(nCntr, nCntr);
        }
        for (size_t nCntr = 1; nCntr < total_entries; nCntr = nCntr + 2)
        {
            ptrTree->template insert<IndexNodeType, DataNodeType>(nCntr, nCntr);
        }

        for (size_t nCntr = 0; nCntr < total_entries; nCntr++)
        {
            int nValue = 0;
            ErrorCode code = ptrTree->template search<IndexNodeType, DataNodeType>(nCntr, nValue);

            assert(nValue == nCntr);
        }

        for (size_t nCntr = 0; nCntr < total_entries; nCntr = nCntr + 2)
        {
            ErrorCode code = ptrTree->template remove<IndexNodeType, DataNodeType>(nCntr);
        }
        for (size_t nCntr = 1; nCntr < total_entries; nCntr = nCntr + 2)
        {
            ErrorCode code = ptrTree->template remove<IndexNodeType, DataNodeType>(nCntr);
        }

        for (int nCntr = 0; nCntr < total_entries; nCntr++)
        {
            int nValue = 0;
            ErrorCode code = ptrTree->template search<IndexNodeType, DataNodeType>(nCntr, nValue);

            assert(code == ErrorCode::KeyDoesNotExist);
        }
    }
/*    i = 0;
    while (i++ < 10) {
        std::cout << "rev:" << i << std::endl;
        for (int nCntr = total_entries; nCntr >= 0; nCntr = nCntr - 2)
        {
            ptrTree->template insert<IndexNodeType, DataNodeType>(nCntr, nCntr);
        }
        for (int nCntr = total_entries - 1; nCntr >= 0; nCntr = nCntr - 2)
        {
            ptrTree->template insert<IndexNodeType, DataNodeType>(nCntr, nCntr);
        }

        for (int nCntr = 0; nCntr < total_entries; nCntr++)
        {
            int nValue = 0;
            ErrorCode code = ptrTree->template search<IndexNodeType, DataNodeType>(nCntr, nValue);

            assert(nValue == nCntr);
        }

        for (int nCntr = total_entries; nCntr >= 0; nCntr = nCntr - 2)
        {
            ErrorCode code = ptrTree->template remove<IndexNodeType, DataNodeType>(nCntr);
        }
        for (int nCntr = total_entries - 1; nCntr >= 0; nCntr = nCntr - 2)
        {
            ErrorCode code = ptrTree->template remove<IndexNodeType, DataNodeType>(nCntr);
        }

        for (int nCntr = 0; nCntr < total_entries; nCntr++)
        {
            int nValue = 0;
            ErrorCode code = ptrTree->template search<IndexNodeType, DataNodeType>(nCntr, nValue);

            assert(code == ErrorCode::KeyDoesNotExist);
        }

    }*/
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << "[ns]" << std::endl;
}

void string_test(int degree, int total_entries)
{
    typedef string KeyType;
    typedef string ValueType;
    typedef uintptr_t CacheKeyType;

    typedef DataNode<KeyType, ValueType> DataNodeType;
    typedef IndexNode<KeyType, ValueType, CacheKeyType> IndexNodeType;

    //typedef BPlusStore<KeyType, ValueType, NoCache<CacheKeyType, NoCacheObject, shared_ptr<DataNodeType>, shared_ptr<IndexNodeType>>> BPlusStoreType;
    //BPlusStoreType* ptrTree = new BPlusStoreType(degree);

    typedef BPlusStore<KeyType, ValueType, LRUCache<VolatileStorage, CacheKeyType, LRUCacheObject, shared_ptr<DataNodeType>, shared_ptr<IndexNodeType>>> BPlusStoreType;
    BPlusStoreType* ptrTree = new BPlusStoreType(degree, 10, 10000000);

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    ptrTree->template init<DataNodeType>();

    int i = 0;

    while (i++ < 10) {
        std::cout << i << std::endl;
        for (size_t nCntr = 0; nCntr < total_entries; nCntr = nCntr + 2)
        {
            ptrTree->template insert<IndexNodeType, DataNodeType>(to_string(nCntr), to_string(nCntr));
        }
        for (size_t nCntr = 1; nCntr < total_entries; nCntr = nCntr + 2)
        {
            ptrTree->template insert<IndexNodeType, DataNodeType>(to_string(nCntr), to_string(nCntr));
        }

        for (size_t nCntr = 0; nCntr < total_entries; nCntr++)
        {
            string nValue = "";
            ErrorCode code = ptrTree->template search<IndexNodeType, DataNodeType>(to_string(nCntr), nValue);

            assert(nValue == to_string(nCntr));
        }

        for (size_t nCntr = 0; nCntr < total_entries; nCntr = nCntr + 2)
        {
            ErrorCode code = ptrTree->template remove<IndexNodeType, DataNodeType>(to_string(nCntr));
        }
        for (size_t nCntr = 1; nCntr < total_entries; nCntr = nCntr + 2)
        {
            ErrorCode code = ptrTree->template remove<IndexNodeType, DataNodeType>(to_string(nCntr));
        }

        for (size_t nCntr = 0; nCntr < total_entries; nCntr++)
        {
            string nValue = "";
            ErrorCode code = ptrTree->template search<IndexNodeType, DataNodeType>(to_string(nCntr), nValue);

            assert(code == ErrorCode::KeyDoesNotExist);
        }

    }

    i = 0;
    while (i++ < 10) {
        std::cout << "rev:" << i << std::endl;
        for (int nCntr = total_entries - 1; nCntr >= 0; nCntr = nCntr - 2)
        {
            ptrTree->template insert<IndexNodeType, DataNodeType>(to_string(nCntr), to_string(nCntr));
        }
        for (int nCntr = total_entries; nCntr >= 0; nCntr = nCntr - 2)
        {
            ptrTree->template insert<IndexNodeType, DataNodeType>(to_string(nCntr), to_string(nCntr));
        }

        for (int nCntr = 0; nCntr < total_entries; nCntr++)
        {
            string nValue = "";
            ErrorCode code = ptrTree->template search<IndexNodeType, DataNodeType>(to_string(nCntr), nValue);

            assert(nValue == to_string(nCntr));
        }

        for (int nCntr = total_entries; nCntr >= 0; nCntr = nCntr - 2)
        {
            ErrorCode code = ptrTree->template remove<IndexNodeType, DataNodeType>(to_string(nCntr));
        }
        for (int nCntr = total_entries - 1; nCntr >= 0; nCntr = nCntr - 2)
        {
            ErrorCode code = ptrTree->template remove<IndexNodeType, DataNodeType>(to_string(nCntr));
        }

        for (int nCntr = 0; nCntr < total_entries; nCntr++)
        {
            string nValue = "";
            ErrorCode code = ptrTree->template search<IndexNodeType, DataNodeType>(to_string(nCntr), nValue);

            assert(code == ErrorCode::KeyDoesNotExist);
        }

    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << "[ns]" << std::endl;
}

int main(int argc, char* argv[])
{
    
    typedef int KeyType;
    typedef int ValueType;
    typedef uintptr_t CacheKeyType;

    typedef DataNode<KeyType, ValueType> DataNodeType;
    typedef IndexNode<KeyType, ValueType, CacheKeyType> IndexNodeType;

    //typedef BPlusStore<KeyType, ValueType, NoCache<CacheKeyType, NoCacheObject, shared_ptr<DataNodeType>, shared_ptr<IndexNodeType>>> BPlusStoreType;
    //BPlusStoreType* ptrTree = new BPlusStoreType(3);

    typedef BPlusStore<KeyType, ValueType, LRUCache<VolatileStorage, CacheKeyType, LRUCacheObject, shared_ptr<DataNodeType>, shared_ptr<IndexNodeType>>> BPlusStoreType;
    BPlusStoreType* ptrTree = new BPlusStoreType(3, 100000, 100000);

    ptrTree->template init<DataNodeType>();

    for (size_t nCntr = 0; nCntr < 10000; nCntr++)
    {
        ptrTree->template insert<IndexNodeType, DataNodeType>(nCntr, nCntr);
    }

    for (size_t nCntr = 0; nCntr < 10000; nCntr++)
    {
        int nValue = 0;
        ErrorCode code = ptrTree->template search<IndexNodeType, DataNodeType>(nCntr, nValue);

        assert(nValue == nCntr);
    }

    for (size_t nCntr = 0; nCntr < 10000; nCntr++)
    {
        ErrorCode code = ptrTree->template remove<IndexNodeType, DataNodeType>(nCntr);
    }

    for (size_t nCntr = 0; nCntr < 10000; nCntr++)
    {
        int nValue = 0;
        ErrorCode code = ptrTree->template search<IndexNodeType, DataNodeType>(nCntr, nValue);

        assert(code == ErrorCode::KeyDoesNotExist);
    }
    

    for (int idx = 3; idx < 20; idx++) {
        std::cout << "iteration.." << idx << std::endl;
        int_test(idx, 10000);
        //string_test(idx, 10000);
        threaded_test(idx, 10000, 10);
    }

    char ch = getchar();
    return 0;
}
