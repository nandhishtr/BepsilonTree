
#include "BeTree.hpp"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ErrorCodes.h>
#include <map>
#include <string>
#include <vadefs.h>
#include <iostream>

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

bool testBeTree(int fanout, int bufferSize, int testSize, int step = 1, bool stochastic = false) {
    std::cout << "Testing BeTree with fanout=" << fanout << " bufferSize=" << bufferSize << " testSize=" << testSize << std::endl;
    typedef int KeyType;
    typedef int ValueType;

    // BeTree<KeyType, ValueType> tree(fanout, bufferSize);
    // BeTree(uint16_t fanout, uint16_t maxBufferSize, size_t blockSize, size_t storageSize, const std::string& filename, size_t cache_capacity)
    BeTree<KeyType, ValueType> tree(fanout, bufferSize, 4192, 1024 * 1024 * 1024, "./filestore.hdb", 10);

    int* arr = new int[testSize];
    for (int i = 0; i < testSize; i++) {
        arr[i] = i;
    }

    shuffle(arr, testSize);
    auto start = std::chrono::high_resolution_clock::now();
    //cout << "Testing insert..." << endl;
    for (int i = 0; i < testSize; i++) {
        //print_progress(i, testSize, step);
        //cout << "Inserting " << arr[i] << endl;
        tree.insert(arr[i], arr[i]);
        // print the tree
        //tree.printTree(cout);

        if (!stochastic) {
            // all inserted keys should be in the tree
            for (int j = 0; j <= i; j++) {
                auto [value, err] = tree.search(arr[j]);
                ASSERT_WITH_PRINT(err == ErrorCode::Success && value == arr[j], "insert failed: i=" << i << " j=" << j << " arr[j]=" << arr[j] << " value=" << value);
            }
        } else {
            // only test the last inserted key and some random keys
            auto [value, err] = tree.search(arr[i]);
            ASSERT_WITH_PRINT(err == ErrorCode::Success && value == arr[i], "insert failed: i=" << i << " arr[i]=" << arr[i] << " value=" << value);

            for (int j = 0; j < 10; j++) {
                int k = rand() % (i + 1);
                auto [value, err] = tree.search(arr[k]);
                ASSERT_WITH_PRINT(err == ErrorCode::Success && value == arr[k], "insert failed: i=" << i << " j=" << j << " k=" << k << " arr[k]=" << arr[k] << " value=" << value);
            }
        }

    }

    shuffle(arr, testSize);
    // remove
    int breakon = 490;
    for (int i = 0; i < testSize; i++) {
        //print_progress(i, testSize, step);
        tree.remove(arr[i]);

        auto [value, err] = tree.search(arr[i]);
        assert(err == ErrorCode::KeyDoesNotExist);

        if (!stochastic) {
            // all not removed keys should still be in the tree
            for (int j = i + 1; j < testSize; j++) {
                auto [value, err] = tree.search(arr[j]);
                ASSERT_WITH_PRINT(err == ErrorCode::Success && value == arr[j], "remove failed: i=" << i << " j=" << j << " arr[j]=" << arr[j] << " value=" << value);
            }
        } else {
            // only test the last removed key and some random keys
            auto [value, err] = tree.search(arr[i]);
            ASSERT_WITH_PRINT(err == ErrorCode::KeyDoesNotExist, "remove failed: i=" << i << " arr[i]=" << arr[i] << " value=" << value);

            for (int j = 0; j < 10; j++) {
                if (i + j >= testSize - 1) break;
                int k = rand() % (testSize - i - 1) + i + 1;
                auto [value, err] = tree.search(arr[k]);
                ASSERT_WITH_PRINT(err == ErrorCode::Success && value == arr[k], "remove failed: i=" << i << " j=" << j << " k=" << k << " arr[k]=" << arr[k] << " value=" << value);
            }
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

    //cout << endl << "Testing interleaved insert and remove..." << endl;
    int repeat = 10;
    int i;
    for (int j = 0; j < testSize * repeat;j++) {
        //print_progress(j, testSize * repeat, step);
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
    std::cout << "Time taken: " << duration << "ms" << std::endl;

    delete[] arr;
    delete[] insertArr;
    delete[] removeArr;

    return true;
}

int main(int argc, char* argv[]) {

    int fanouts[] = { 2, 3, 4, 5, 6, 7, 8, 16, 32, 64, 128, 256 };
    int bufferSizes[] = { 2, 4, 8, 16, 32, 64, 128, 256 };
    int size = 100;
    for (int i = 0; i < sizeof(fanouts) / sizeof(fanouts[0]); i++) {
        for (int j = 0; j < sizeof(bufferSizes) / sizeof(bufferSizes[0]); j++) {
            //int localSize = (fanouts[i] + bufferSizes[j]) * size;
            int localSize = 100;
            bool stochastic = localSize > 1500;
            int step = localSize / 10;
            testBeTree(fanouts[i], bufferSizes[j], localSize, step, stochastic);
        }
    }
    std::cout << "All tests passed!" << std::endl;
    char ch = getchar();
    return 0;
}

