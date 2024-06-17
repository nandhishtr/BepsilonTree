#include <iostream>

#include "BeTree.hpp"
#include "BPlusStore.hpp"
#include "NoCache.hpp"


#include "DataNode.hpp"
#include "IndexNode.hpp"

#include <cassert>
#include <chrono>

#include "NoCacheObject.hpp"
//#include "PMemStorage.hpp"

#include "TypeUID.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vadefs.h>
#include <ErrorCodes.h>



#ifdef __CONCURRENT__

template <typename BPlusStoreType, typename IndexNodeType, typename DataNodeType>
void insert_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
    for (size_t nCntr = nRangeStart; nCntr < nRangeEnd; nCntr++) {
        ptrTree->insert(nCntr, nCntr);
    }
}

template <typename BPlusStoreType, typename IndexNodeType, typename DataNodeType>
void reverse_insert_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
    for (int nCntr = nRangeEnd - 1; nCntr >= nRangeStart; nCntr--) {
        ptrTree->insert(nCntr, nCntr);
    }
}

template <typename BPlusStoreType, typename IndexNodeType, typename DataNodeType>
void search_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
    for (size_t nCntr = nRangeStart; nCntr < nRangeEnd; nCntr++) {
        int nValue = 0;
        ErrorCode code = ptrTree->search(nCntr, nValue);

        assert(nCntr == nValue);
    }
}

template <typename BPlusStoreType, typename IndexNodeType, typename DataNodeType>
void search_not_found_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
    for (size_t nCntr = nRangeStart; nCntr < nRangeEnd; nCntr++) {
        int nValue = 0;
        ErrorCode errCode = ptrTree->search(nCntr, nValue);

        assert(errCode == ErrorCode::KeyDoesNotExist);
    }
}

template <typename BPlusStoreType, typename IndexNodeType, typename DataNodeType>
void delete_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
    for (size_t nCntr = nRangeStart; nCntr < nRangeEnd; nCntr++) {
        ErrorCode code = ptrTree->remove(nCntr);
    }
}

template <typename BPlusStoreType, typename IndexNodeType, typename DataNodeType>
void reverse_delete_concurent(BPlusStoreType* ptrTree, int nRangeStart, int nRangeEnd) {
    for (int nCntr = nRangeEnd - 1; nCntr >= nRangeStart; nCntr--) {
        ErrorCode code = ptrTree->remove(nCntr);
    }
}


template <typename BPlusStoreType, typename IndexNodeType, typename DataNodeType>
void threaded_test(BPlusStoreType* ptrTree, int degree, int total_entries, int thread_count) {
    vector<std::thread> vtThreads;

    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    int i = 0;
    while (i++ < 10) {
        std::cout << i << std::endl;

        for (int nIdx = 0; nIdx < thread_count; nIdx++) {
            int nTotal = total_entries / thread_count;
            vtThreads.push_back(std::thread(insert_concurent<BPlusStoreType, IndexNodeType, DataNodeType>, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        auto it = vtThreads.begin();
        while (it != vtThreads.end()) {
            (*it).join();
            it++;
        }

        vtThreads.clear();

        for (int nIdx = 0; nIdx < thread_count; nIdx++) {
            int nTotal = total_entries / thread_count;
            vtThreads.push_back(std::thread(search_concurent<BPlusStoreType, IndexNodeType, DataNodeType>, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        it = vtThreads.begin();
        while (it != vtThreads.end()) {
            (*it).join();
            it++;
        }

        vtThreads.clear();

        for (int nIdx = 0; nIdx < thread_count; nIdx++) {
            int nTotal = total_entries / thread_count;
            vtThreads.push_back(std::thread(delete_concurent<BPlusStoreType, IndexNodeType, DataNodeType>, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        it = vtThreads.begin();
        while (it != vtThreads.end()) {
            (*it).join();
            it++;
        }

        vtThreads.clear();

        for (int nIdx = 0; nIdx < thread_count; nIdx++) {
            int nTotal = total_entries / thread_count;
            vtThreads.push_back(std::thread(search_not_found_concurent<BPlusStoreType, IndexNodeType, DataNodeType>, ptrTree, nIdx * nTotal, nIdx * nTotal + nTotal));
        }

        it = vtThreads.begin();
        while (it != vtThreads.end()) {
            (*it).join();
            it++;
        }

        vtThreads.clear();

        size_t lru, map;
        ptrTree->getCacheState(lru, map);
        assert(lru == 1 && map == 1);

    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[�s]" << std::endl;
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << "[ns]" << std::endl;
}

#endif __CONCURRENT__

template <typename BPlusStoreType, typename IndexNodeType, typename DataNodeType>
void int_test(BPlusStoreType* ptrTree, int degree, int total_entries) {
    std::cout << degree << ".";
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    int i = 0;
    while (i++ < 10) {
        //std::cout << i << ",";;
        for (size_t nCntr = 0; nCntr < total_entries; nCntr = nCntr + 1) {
            //std::cout << nCntr << ",";
            ptrTree->insert(nCntr, nCntr);
        }

        for (size_t nCntr = 0; nCntr < total_entries; nCntr++) {
            int nValue = 0;
            ErrorCode code = ptrTree->search(nCntr, nValue);

            assert(nValue == nCntr);
        }

        for (size_t nCntr = 0; nCntr < total_entries; nCntr = nCntr + 2) {
            ErrorCode code = ptrTree->remove(nCntr);
        }
        for (size_t nCntr = 1; nCntr < total_entries; nCntr = nCntr + 2) {
            ErrorCode code = ptrTree->remove(nCntr);
        }

        for (int nCntr = 0; nCntr < total_entries; nCntr++) {
            int nValue = 0;
            ErrorCode code = ptrTree->search(nCntr, nValue);

            assert(code == ErrorCode::KeyDoesNotExist);
        }

        size_t lru, map;
        ptrTree->getCacheState(lru, map);
        assert(lru == 1 && map == 1);

    }
    i = 0;
    while (i++ < 10) {
        std::cout << "rev:" << i << ",";
        for (int nCntr = total_entries; nCntr >= 0; nCntr = nCntr - 2) {
            ptrTree->insert(nCntr, nCntr);
        }
        for (int nCntr = total_entries - 1; nCntr >= 0; nCntr = nCntr - 2) {
            ptrTree->insert(nCntr, nCntr);
        }

        for (int nCntr = 0; nCntr < total_entries; nCntr++) {
            int nValue = 0;
            ErrorCode code = ptrTree->search(nCntr, nValue);

            assert(nValue == nCntr);
        }

        for (int nCntr = total_entries; nCntr >= 0; nCntr = nCntr - 2) {
            ErrorCode code = ptrTree->remove(nCntr);
        }
        for (int nCntr = total_entries - 1; nCntr >= 0; nCntr = nCntr - 2) {
            ErrorCode code = ptrTree->remove(nCntr);
        }

        for (int nCntr = 0; nCntr < total_entries; nCntr++) {
            int nValue = 0;
            ErrorCode code = ptrTree->search(nCntr, nValue);

            assert(code == ErrorCode::KeyDoesNotExist);
        }

        size_t lru, map;
        ptrTree->getCacheState(lru, map);
        assert(lru == 1 && map == 1);
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[�s]" << std::endl;
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << "[ns]" << std::endl;
}

template <typename BPlusStoreType, typename IndexNodeType, typename DataNodeType>
void string_test(BPlusStoreType* ptrTree, int degree, int total_entries) {
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

    int i = 0;

    while (i++ < 10) {
        std::cout << i << ",";;
        for (size_t nCntr = 0; nCntr < total_entries; nCntr = nCntr + 2) {
            ptrTree->insert(to_string(nCntr), to_string(nCntr));
        }
        for (size_t nCntr = 1; nCntr < total_entries; nCntr = nCntr + 2) {
            ptrTree->insert(to_string(nCntr), to_string(nCntr));
        }

        for (size_t nCntr = 0; nCntr < total_entries; nCntr++) {
            string nValue = "";
            ErrorCode code = ptrTree->search(to_string(nCntr), nValue);

            assert(nValue == to_string(nCntr));
        }

        for (size_t nCntr = 0; nCntr < total_entries; nCntr = nCntr + 2) {
            ErrorCode code = ptrTree->remove(to_string(nCntr));
        }
        for (size_t nCntr = 1; nCntr < total_entries; nCntr = nCntr + 2) {
            ErrorCode code = ptrTree->remove(to_string(nCntr));
        }

        for (size_t nCntr = 0; nCntr < total_entries; nCntr++) {
            string nValue = "";
            ErrorCode code = ptrTree->search(to_string(nCntr), nValue);

            assert(code == ErrorCode::KeyDoesNotExist);
        }

        size_t lru, map;
        ptrTree->getCacheState(lru, map);
        assert(lru == 1 && map == 1);
    }

    i = 0;
    while (i++ < 10) {
        std::cout << "rev:" << i << ",";

        for (int nCntr = total_entries - 1; nCntr >= 0; nCntr = nCntr - 2) {
            ptrTree->insert(to_string(nCntr), to_string(nCntr));
        }
        for (int nCntr = total_entries; nCntr >= 0; nCntr = nCntr - 2) {
            ptrTree->insert(to_string(nCntr), to_string(nCntr));
        }

        for (int nCntr = 0; nCntr < total_entries; nCntr++) {
            string nValue = "";
            ErrorCode code = ptrTree->search(to_string(nCntr), nValue);

            assert(nValue == to_string(nCntr));
        }

        for (int nCntr = total_entries; nCntr >= 0; nCntr = nCntr - 2) {
            ErrorCode code = ptrTree->remove(to_string(nCntr));
        }
        for (int nCntr = total_entries - 1; nCntr >= 0; nCntr = nCntr - 2) {
            ErrorCode code = ptrTree->remove(to_string(nCntr));
        }

        for (int nCntr = 0; nCntr < total_entries; nCntr++) {
            string nValue = "";
            ErrorCode code = ptrTree->search(to_string(nCntr), nValue);

            assert(code == ErrorCode::KeyDoesNotExist);
        }
        size_t lru, map;
        ptrTree->getCacheState(lru, map);
        assert(lru == 1 && map == 1);
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[�s]" << std::endl;
    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << "[ns]" << std::endl;
}

void test_for_ints() {
    for (int idx = 1000; idx < 2000; idx = idx + 200) {
        std::cout << "test_for_ints idx:" << idx << "| ";
        {
#ifndef __TREE_WITH_CACHE__
            std::cout << "....";
            typedef int KeyType;
            typedef int ValueType;
            typedef uintptr_t ObjectUIDType;

            typedef DataNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::DATA_NODE_INT_INT> DataNodeType;
            typedef IndexNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::INDEX_NODE_INT_INT> IndexNodeType;

            typedef BPlusStore<KeyType, ValueType, NoCache<ObjectUIDType, NoCacheObject, DataNodeType, IndexNodeType>> BPlusStoreType;
            BPlusStoreType ptrTree(idx);
            ptrTree.template init<DataNodeType>();

            int_test<BPlusStoreType, IndexNodeType, DataNodeType>(&ptrTree, idx, 50000000);
#endif __TREE_WITH_CACHE__
        }
        {
#ifdef __TREE_WITH_CACHE__
            typedef int KeyType;
            typedef int ValueType;
            typedef ObjectFatUID ObjectUIDType;

            typedef DataNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::DATA_NODE_INT_INT> DataNodeType;
            typedef IndexNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::INDEX_NODE_INT_INT> IndexNodeType;

            typedef NVMRODataNode<KeyType, ValueType, ObjectUIDType, DataNodeType, DataNodeType, TYPE_UID::DATA_NODE_INT_INT> NVMRODataNodeType;
            typedef NVMROIndexNode<KeyType, ValueType, ObjectUIDType, IndexNodeType, IndexNodeType, TYPE_UID::INDEX_NODE_INT_INT> NVMROIndexNodeType;

            typedef LRUCacheObject<TypeMarshaller, NVMRODataNodeType, NVMROIndexNodeType> ObjectType;
            typedef IFlushCallback<ObjectUIDType, ObjectType> ICallback;

            typedef BPlusStore<ICallback, KeyType, ValueType, LRUCache<ICallback, VolatileStorage<ICallback, ObjectUIDType, LRUCacheObject, TypeMarshaller, NVMRODataNodeType, NVMROIndexNodeType>>> BPlusStoreType;
            BPlusStoreType ptrTree(idx, 1000, 1024, 1024 * 1024 * 1024);
            ptrTree.template init<NVMRODataNodeType>();

            int_test<BPlusStoreType, NVMROIndexNodeType, NVMRODataNodeType>(&ptrTree, idx, 5000);
#endif __TREE_WITH_CACHE__
        }
        {
#ifdef __TREE_WITH_CACHE__
            typedef int KeyType;
            typedef int ValueType;
            typedef ObjectFatUID ObjectUIDType;

            typedef DataNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::DATA_NODE_INT_INT> DataNodeType;
            typedef IndexNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::INDEX_NODE_INT_INT> IndexNodeType;

            typedef LRUCacheObject<TypeMarshaller, DataNodeType, IndexNodeType> ObjectType;
            typedef IFlushCallback<ObjectUIDType, ObjectType> ICallback;

            typedef BPlusStore<ICallback, KeyType, ValueType, LRUCache<ICallback, VolatileStorage<ICallback, ObjectUIDType, LRUCacheObject, TypeMarshaller, DataNodeType, IndexNodeType>>> BPlusStoreType;
            BPlusStoreType ptrTree(idx, 1000, 1024, 1024 * 1024 * 1024);
            ptrTree.template init<DataNodeType>();

            int_test<BPlusStoreType, IndexNodeType, DataNodeType>(&ptrTree, idx, 5000);
#endif __TREE_WITH_CACHE__
        }

        {
#ifdef __TREE_WITH_CACHE__
            typedef int KeyType;
            typedef int ValueType;
            typedef ObjectFatUID ObjectUIDType;

            typedef DataNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::DATA_NODE_INT_INT> DataNodeType;
            typedef IndexNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::INDEX_NODE_INT_INT> IndexNodeType;

            typedef LRUCacheObject<TypeMarshaller, DataNodeType, IndexNodeType> ObjectType;
            typedef IFlushCallback<ObjectUIDType, ObjectType> ICallback;

            typedef BPlusStore<ICallback, KeyType, ValueType, LRUCache<ICallback, FileStorage<ICallback, ObjectUIDType, LRUCacheObject, TypeMarshaller, DataNodeType, IndexNodeType>>> BPlusStoreType;
            BPlusStoreType* ptrTree = new BPlusStoreType(idx, 1000, 1024, 1024 * 1024 * 1024, "D:\\filestore.hdb");
            ptrTree->init<DataNodeType>();

            int_test<BPlusStoreType, IndexNodeType, DataNodeType>(ptrTree, idx, 10000);
#endif __TREE_WITH_CACHE__
        }
        std::cout << std::endl;
    }
}

void test_for_string() {
    for (int idx = 3; idx < 40; idx++) {
        std::cout << "test_for_string idx:" << idx << "| ";
        {
#ifndef __TREE_WITH_CACHE__
            typedef string KeyType;
            typedef string ValueType;
            typedef uintptr_t ObjectUIDType;

            typedef DataNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::DATA_NODE_STRING_STRING> DataNodeType;
            typedef IndexNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::INDEX_NODE_STRING_STRING> IndexNodeType;

            typedef BPlusStore<KeyType, ValueType, NoCache<ObjectUIDType, NoCacheObject, DataNodeType, IndexNodeType>> BPlusStoreType;
            BPlusStoreType* ptrTree1 = new BPlusStoreType(idx);
            ptrTree1->init<DataNodeType>();

            string_test<BPlusStoreType, IndexNodeType, DataNodeType>(ptrTree1, idx, 10000);
#endif __TREE_WITH_CACHE__
        }
        {
#ifdef __TREE_WITH_CACHE__
            /*typedef string KeyType;
            typedef string ValueType;
            typedef ObjectFatUID ObjectUIDType;

            typedef DataNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::DATA_NODE_STRING_STRING> DataNodeType;
            typedef IndexNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::INDEX_NODE_STRING_STRING> IndexNodeType;

            typedef LRUCacheObject<TypeMarshaller, DataNodeType, IndexNodeType> ObjectType;
            typedef IFlushCallback<ObjectUIDType, ObjectType> ICallback;

            typedef BPlusStore<ICallback, KeyType, ValueType, LRUCache<ICallback, VolatileStorage<ICallback, ObjectUIDType, LRUCacheObject, TypeMarshaller, DataNodeType, IndexNodeType>>> BPlusStoreType;
            BPlusStoreType* ptrTree2 = new BPlusStoreType(idx, 1000, 100000000);
            ptrTree2->init<DataNodeType>();

            string_test<BPlusStoreType, IndexNodeType, DataNodeType>(ptrTree2, idx, 10000);*/
#endif __TREE_WITH_CACHE__
        }
        {
#ifdef __TREE_WITH_CACHE__
            /*typedef string KeyType;
            typedef string ValueType;
            typedef ObjectFatUID ObjectUIDType;

            typedef DataNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::DATA_NODE_STRING_STRING> DataNodeType;
            typedef IndexNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::INDEX_NODE_STRING_STRING> IndexNodeType;

            typedef LRUCacheObject<TypeMarshaller, DataNodeType, IndexNodeType> ObjectType;
            typedef IFlushCallback<ObjectUIDType, ObjectType> ICallback;

            typedef BPlusStore<ICallback, KeyType, ValueType, LRUCache<ICallback, FileStorage<ICallback, ObjectUIDType, LRUCacheObject, TypeMarshaller, DataNodeType, IndexNodeType>>> BPlusStoreType;
            BPlusStoreType* ptrTree = new BPlusStoreType(3, 100, 1024, 1024 * 1024 * 1024, "filestore.hdb");
            ptrTree->init<DataNodeType>();

            string_test<BPlusStoreType, IndexNodeType, DataNodeType>(ptrTree, idx, 10000);*/
#endif __TREE_WITH_CACHE__
        }
        std::cout << std::endl;
    }
}

void test_for_threaded() {
#ifdef __CONCURRENT__
    for (int idx = 3; idx < 20; idx++) {
        std::cout << "iteration.." << idx << std::endl;
        {
#ifndef __TREE_WITH_CACHE__
            typedef int KeyType;
            typedef int ValueType;
            typedef uintptr_t ObjectUIDType;

            typedef DataNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::DATA_NODE_INT_INT> DataNodeType;
            typedef IndexNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::INDEX_NODE_INT_INT> IndexNodeType;

            typedef BPlusStore<KeyType, ValueType, NoCache<ObjectUIDType, NoCacheObject, DataNodeType, IndexNodeType>> BPlusStoreType;
            BPlusStoreType ptrTree(idx);
            ptrTree.template init<DataNodeType>();

            threaded_test<BPlusStoreType, IndexNodeType, DataNodeType>(&ptrTree, idx, 3 * 10000, 10);
#endif __TREE_WITH_CACHE__

        }
        {
#ifdef __TREE_WITH_CACHE__
            typedef int KeyType;
            typedef int ValueType;
            typedef ObjectFatUID ObjectUIDType;

            typedef DataNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::DATA_NODE_INT_INT> DataNodeType;
            typedef IndexNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::INDEX_NODE_INT_INT> IndexNodeType;

            typedef NVMRODataNode<KeyType, ValueType, ObjectUIDType, DataNodeType, DataNodeType, TYPE_UID::DATA_NODE_INT_INT> NVMRODataNodeType;
            typedef NVMROIndexNode<KeyType, ValueType, ObjectUIDType, IndexNodeType, IndexNodeType, TYPE_UID::INDEX_NODE_INT_INT> NVMROIndexNodeType;

            typedef LRUCacheObject<TypeMarshaller, NVMRODataNodeType, NVMROIndexNodeType> ObjectType;
            typedef IFlushCallback<ObjectUIDType, ObjectType> ICallback;


            typedef BPlusStore<ICallback, KeyType, ValueType, LRUCache<ICallback, VolatileStorage<ICallback, ObjectUIDType, LRUCacheObject, TypeMarshaller, NVMRODataNodeType, NVMROIndexNodeType>>> BPlusStoreType;
            BPlusStoreType ptrTree(idx, 1000, 1024, 1024 * 1024 * 1024);
            ptrTree.template init<NVMRODataNodeType>();

            threaded_test<BPlusStoreType, NVMROIndexNodeType, NVMRODataNodeType>(&ptrTree, idx, 3 * 10000, 10);
#endif __TREE_WITH_CACHE__
        }
        {
#ifdef __TREE_WITH_CACHE__
            typedef int KeyType;
            typedef int ValueType;
            typedef ObjectFatUID ObjectUIDType;

            typedef DataNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::DATA_NODE_INT_INT> DataNodeType;
            typedef IndexNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::INDEX_NODE_INT_INT> IndexNodeType;

            typedef LRUCacheObject<TypeMarshaller, DataNodeType, IndexNodeType> ObjectType;
            typedef IFlushCallback<ObjectUIDType, ObjectType> ICallback;


            typedef BPlusStore<ICallback, KeyType, ValueType, LRUCache<ICallback, VolatileStorage<ICallback, ObjectUIDType, LRUCacheObject, TypeMarshaller, DataNodeType, IndexNodeType>>> BPlusStoreType;
            BPlusStoreType ptrTree(idx, 1000, 1024, 1024 * 1024 * 1024);
            ptrTree.template init<DataNodeType>();

            threaded_test<BPlusStoreType, IndexNodeType, DataNodeType>(&ptrTree, idx, 3 * 10000, 10);
#endif __TREE_WITH_CACHE__
        }
        {
#ifdef __TREE_WITH_CACHE__
            typedef int KeyType;
            typedef int ValueType;
            typedef ObjectFatUID ObjectUIDType;

            typedef DataNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::DATA_NODE_INT_INT> DataNodeType;
            typedef IndexNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::INDEX_NODE_INT_INT> IndexNodeType;

            typedef LRUCacheObject<TypeMarshaller, DataNodeType, IndexNodeType> ObjectType;
            typedef IFlushCallback<ObjectUIDType, ObjectType> ICallback;

            typedef BPlusStore<ICallback, KeyType, ValueType, LRUCache<ICallback, FileStorage<ICallback, ObjectUIDType, LRUCacheObject, TypeMarshaller, DataNodeType, IndexNodeType>>> BPlusStoreType;
            BPlusStoreType ptrTree(idx, 1000, 1024, 1024 * 1024 * 1024, "D:\\filestore.hdb");
            ptrTree.template init<DataNodeType>();

            threaded_test<BPlusStoreType, IndexNodeType, DataNodeType>(&ptrTree, idx, 3 * 10000, 10);
#endif __TREE_WITH_CACHE__
        }
    }
#endif __CONCURRENT__
}

#define ASSERT_WITH_PRINT(expr, message) \
    do { \
        if (!(expr)) { \
            std::cerr << "Assertion failed: (" << #expr << ") " \
                      << "at " << __FILE__ << ":" << __LINE__ << std::endl; \
            std::cerr << message << std::endl; \
            std::abort(); \
        } \
    } while (0)

void print_progress(int iteration, int total, int step = 1) {
    iteration++;
    if (step == 0) return;
    if (iteration % step != 0 && iteration != total) return; // Print only at step intervals (and at the end)

    int barWidth = 70; // Width of the progress bar
    float progress = (float)iteration / total;
    std::cout << "[";
    int pos = barWidth * progress;

    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << "% (" << iteration << "/" << total << ")\r";
    //std::cout.flush(); // Important to flush, so the progress bar is updated in the same line
}

void shuffle(int* arr, int size) {
    for (int i = 0; i < size; i++) {
        int j = rand() % size;
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

bool testBeTree(int fanout, int bufferSize, int testSize, int step = 1) {
    cout << "Testing BeTree with fanout=" << fanout << " bufferSize=" << bufferSize << " testSize=" << testSize << endl;
    typedef int KeyType;
    typedef int ValueType;

    BeTree<KeyType, ValueType> tree(fanout, bufferSize);

    int* arr = new int[testSize];
    for (int i = 0; i < testSize; i++) {
        arr[i] = i;
    }

    shuffle(arr, testSize);
    auto start = std::chrono::high_resolution_clock::now();
    cout << "Testing insert..." << endl;
    for (int i = 0; i < testSize; i++) {
        print_progress(i, testSize, step);
        tree.insert(arr[i], arr[i]);

        // all inserted keys should be in the tree
        for (int j = 0; j <= i; j++) {
            auto [value, err] = tree.search(arr[j]);
            ASSERT_WITH_PRINT(err == ErrorCode::Success && value == arr[j], "insert failed: i=" << i << " j=" << j << " arr[j]=" << arr[j] << " value=" << value);
        }
    }

    shuffle(arr, testSize);
    cout << endl << "Testing remove..." << endl;
    // remove
    int breakon = 490;
    for (int i = 0; i < testSize; i++) {
        print_progress(i, testSize, step);
        tree.remove(arr[i]);

        auto [value, err] = tree.search(arr[i]);
        assert(err == ErrorCode::KeyDoesNotExist);

        // all not removed keys should still be in the tree
        for (int j = i + 1; j < testSize; j++) {
            auto [value, err] = tree.search(arr[j]);
            ASSERT_WITH_PRINT(err == ErrorCode::Success && value == arr[j], "remove failed: i=" << i << " j=" << j << " arr[j]=" << arr[j] << " value=" << value);
        }
    }

    // Interleaved insert and remove
    int* insertArr = new int[testSize];
    int* removeArr = new int[testSize];
    for (int i = 0; i < testSize; i++) {
        insertArr[i] = i;
        removeArr[i] = i;
    }
    shuffle(insertArr, testSize);
    shuffle(removeArr, testSize);

    cout << endl << "Testing interleaved insert and remove..." << endl;
    int repeat = 10;
    int i;
    for (int j = 0; j < testSize * repeat;j++) {
        print_progress(j, testSize * repeat, step);
        i = j % testSize;

        // Insert
        tree.insert(insertArr[i], insertArr[i]);
        auto [value, err] = tree.search(insertArr[i]);
        ASSERT_WITH_PRINT(err == ErrorCode::Success && value == insertArr[i], "insert failed: j=" << j << " arr[i]=" << insertArr[i] << " value=" << value);

        // Delete
        if (j > testSize * 2) {
            tree.remove(removeArr[i]);
            auto [value, err] = tree.search(removeArr[i]);
            ASSERT_WITH_PRINT(err == ErrorCode::KeyDoesNotExist, "remove failed: j=" << j << " arr[i]=" << removeArr[i] << " value=" << value);
        }
    }


    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    cout << endl << "Time taken: " << duration << "ms" << endl;

    delete[] arr;
    delete[] insertArr;
    delete[] removeArr;

    return true;
}

int main(int argc, char* argv[]) {
    //test_for_ints();
    //test_for_string();
    //test_for_threaded();

    int size = 10000;
    int step = size / 10;
    testBeTree(2, 0, size, step);
    testBeTree(3, 0, size, step);
    testBeTree(4, 0, size, step);
    testBeTree(5, 0, size, step);
    testBeTree(6, 0, size, step);
    testBeTree(7, 0, size, step);
    testBeTree(8, 0, size, step);
    testBeTree(16, 0, size, step);
    testBeTree(32, 0, size, step);
    testBeTree(64, 0, size, step);
    testBeTree(128, 0, size, step);
    testBeTree(256, 0, size, step);

    cout << "All tests passed!" << endl;


#ifdef __TREE_WITH_CACHE__
    typedef ObjectFatUID ObjectUIDType;

    typedef DataNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::DATA_NODE_INT_INT> DataNodeType;
    typedef IndexNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::INDEX_NODE_INT_INT> IndexNodeType;

    typedef LRUCacheObject<TypeMarshaller, DataNodeType, IndexNodeType> ObjectType;
    typedef IFlushCallback<ObjectUIDType, ObjectType> ICallback;
    typedef BPlusStore<ICallback, KeyType, ValueType, LRUCache<ICallback, FileStorage<ICallback, ObjectUIDType, LRUCacheObject, TypeMarshaller, DataNodeType, IndexNodeType>>> BPlusStoreType;
    BPlusStoreType* ptrTree = new BPlusStoreType(3, 100, 512, 1024 * 1024 * 1024, "D:\\filestore.hdb");
    ptrTree->init<DataNodeType>();


    /*typedef NVMRODataNode<KeyType, ValueType, ObjectUIDType, DataNodeType, DataNodeType, TYPE_UID::NVMRODATA_NODE_INT_INT> NVMRODataNodeType;
    typedef NVMROIndexNode<KeyType, ValueType, ObjectUIDType, IndexNodeType, IndexNodeType, TYPE_UID::NVMROINDEX_NODE_INT_INT> NVMROIndexNodeType;
    typedef LRUCacheObject<TypeMarshaller, NVMRODataNodeType, NVMROIndexNodeType> ObjectType;
    typedef IFlushCallback<ObjectUIDType, ObjectType> ICallback;
    typedef BPlusStore<ICallback, KeyType, ValueType, LRUCache<ICallback, FileStorage<ICallback, ObjectUIDType, LRUCacheObject, TypeMarshaller, NVMRODataNodeType, NVMROIndexNodeType>>> BPlusStoreType;
    BPlusStoreType* ptrTree = new BPlusStoreType(3, 100, 512, 1024 * 1024 * 1024, "D:\\filestore.hdb");
    ptrTree->init<NVMRODataNodeType>();*/


    //typedef ObjectFatUID ObjectUIDType;
    //typedef DataNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::DATA_NODE_INT_INT> DataNodeType;
    //typedef IndexNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::INDEX_NODE_INT_INT> IndexNodeType;

    //typedef DataNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::DATA_NODE_INT_INT> NVMRODataNodeType;
    //typedef IndexNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::INDEX_NODE_INT_INT> NVMROIndexNodeType;

    ////typedef NVMRODataNode<KeyType, ValueType, ObjectUIDType, DataNodeType, DataNodeType, TYPE_UID::DATA_NODE_INT_INT> NVMRODataNodeType;
    ////typedef NVMROIndexNode<KeyType, ValueType, ObjectUIDType, IndexNodeType, IndexNodeType, TYPE_UID::INDEX_NODE_INT_INT> NVMROIndexNodeType;

    //typedef LRUCacheObject<TypeMarshaller, NVMRODataNodeType, NVMROIndexNodeType> ObjectType;
    //typedef IFlushCallback<ObjectUIDType, ObjectType> ICallback;

    //typedef BPlusStore<ICallback, KeyType, ValueType, LRUCache<ICallback, VolatileStorage<ICallback, ObjectUIDType, LRUCacheObject, TypeMarshaller, NVMRODataNodeType, NVMROIndexNodeType>>> BPlusStoreType;
    //BPlusStoreType* ptrTree = new BPlusStoreType(3, 100, 1024, 1024 * 1024 * 1024);
    //ptrTree->init<NVMRODataNodeType>();

#else //__TREE_WITH_CACHE__
    typedef int KeyType;
    typedef int ValueType;
    typedef uintptr_t ObjectUIDType;

    typedef DataNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::DATA_NODE_INT_INT> DataNodeType;
    typedef IndexNode<KeyType, ValueType, ObjectUIDType, TYPE_UID::INDEX_NODE_INT_INT> IndexNodeType;

    typedef BPlusStore<KeyType, ValueType, NoCache<ObjectUIDType, NoCacheObject, DataNodeType, IndexNodeType>> BPlusStoreType;
    BPlusStoreType* ptrTree = new BPlusStoreType(3);
    ptrTree->template init<DataNodeType>();

#endif __TREE_WITH_CACHE__


    /*    for (size_t nCntr = 0; nCntr <= 99999; nCntr = nCntr + 2)
        {
            ptrTree->insert(nCntr, nCntr);
        }

        for (size_t nCntr = 0 + 1; nCntr <= 99999; nCntr = nCntr + 2)
        {
            ptrTree->insert(nCntr, nCntr);
        }
    */

    for (size_t nCntr = 0; nCntr < 10000; nCntr++) {
        ptrTree->insert(nCntr, nCntr);
    }

#ifdef __TREE_WITH_CACHE__    
    ptrTree->flush();
#endif __TREE_WITH_CACHE__
    for (size_t nCntr = 0; nCntr < 10000; nCntr++) {
        int nValue = 0;
        ErrorCode code = ptrTree->search(nCntr, nValue);

        assert(nValue == nCntr);
}

#ifdef __TREE_WITH_CACHE__
    ptrTree->flush();
#endif __TREE_WITH_CACHE__
    for (size_t nCntr = 0; nCntr < 10000; nCntr++) {
        ErrorCode code = ptrTree->remove(nCntr);
    }

    for (size_t nCntr = 0; nCntr < 10000; nCntr++) {
        int nValue = 0;
        ErrorCode code = ptrTree->search(nCntr, nValue);

        assert(code == ErrorCode::KeyDoesNotExist);
    }
    std::cout << "End.";
    char ch = getchar();
    return 0;
}

