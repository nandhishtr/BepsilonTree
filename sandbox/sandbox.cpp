
#include "BeTree.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <ErrorCodes.h>
#include <exception>
#include <iostream>
#include <random>
#include <stdlib.h>
#include <vector>

using namespace std;

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
        // tree.printTree(cout);

        if (!stochastic) {
            // all inserted keys should be in the tree
            for (int j = 0; j <= i; j++) {
                auto [value, err] = tree.search(arr[j]);
                ASSERT_WITH_PRINT(err == ErrorCode::Success && value == arr[j], "insert failed: i=" << i << " j=" << j << " arr[j]=" << arr[j] << " value=" << value);
                // for release mode testing
                if (err != ErrorCode::Success || value != arr[j]) {
                    cout << "insert failed: i=" << i << " j=" << j << " arr[j]=" << arr[j] << " value=" << value << endl;
                    tree.printTree(cout);
                    return false;
            }
            }
        } else {
            // only test the last inserted key and some random keys
            auto [value, err] = tree.search(arr[i]);
            ASSERT_WITH_PRINT(err == ErrorCode::Success && value == arr[i], "insert failed: i=" << i << " arr[i]=" << arr[i] << " value=" << value);
            if (err != ErrorCode::Success || value != arr[i]) {
                cout << "insert failed: i=" << i << " arr[i]=" << arr[i] << " value=" << value << endl;
                tree.printTree(cout);
                return false;
            }

            for (int j = 0; j < 10; j++) {
                int k = rand() % (i + 1);
                auto [value, err] = tree.search(arr[k]);
                ASSERT_WITH_PRINT(err == ErrorCode::Success && value == arr[k], "insert failed: i=" << i << " j=" << j << " k=" << k << " arr[k]=" << arr[k] << " value=" << value);
                if (err != ErrorCode::Success || value != arr[k]) {
                    cout << "insert failed: i=" << i << " j=" << j << " k=" << k << " arr[k]=" << arr[k] << " value=" << value << endl;
                    tree.printTree(cout);
                    return false;
            }
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
                if (err != ErrorCode::Success || value != arr[j]) {
                    cout << "remove failed: i=" << i << " j=" << j << " arr[j]=" << arr[j] << " value=" << value << endl;
                    tree.printTree(cout);
                    return false;
            }
            }
        } else {
            // only test the last removed key and some random keys
            auto [value, err] = tree.search(arr[i]);
            ASSERT_WITH_PRINT(err == ErrorCode::KeyDoesNotExist, "remove failed: i=" << i << " arr[i]=" << arr[i] << " value=" << value);
            if (err != ErrorCode::KeyDoesNotExist) {
                cout << "remove failed: i=" << i << " arr[i]=" << arr[i] << " value=" << value << endl;
                tree.printTree(cout);
                return false;
            }

            for (int j = 0; j < 10; j++) {
                if (i + j >= testSize - 1) break;
                int k = rand() % (testSize - i - 1) + i + 1;
                auto [value, err] = tree.search(arr[k]);
                ASSERT_WITH_PRINT(err == ErrorCode::Success && value == arr[k], "remove failed: i=" << i << " j=" << j << " k=" << k << " arr[k]=" << arr[k] << " value=" << value);
                if (err != ErrorCode::Success || value != arr[k]) {
                    cout << "remove failed: i=" << i << " j=" << j << " k=" << k << " arr[k]=" << arr[k] << " value=" << value << endl;
                    tree.printTree(cout);
                    return false;
            }
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
        if (err != ErrorCode::Success || value != insertArr[i]) {
            cout << "insert failed: j=" << j << " arr[i]=" << insertArr[i] << " value=" << value << endl;
            tree.printTree(cout);
            return false;
        }

        // Delete
        if (j > testSize * 2) {
            tree.remove(removeArr[i]);
            auto [value, err] = tree.search(removeArr[i]);
            ASSERT_WITH_PRINT(err == ErrorCode::KeyDoesNotExist, "remove failed: j=" << j << " arr[i]=" << removeArr[i] << " value=" << value);
            if (err != ErrorCode::KeyDoesNotExist) {
                cout << "remove failed: j=" << j << " arr[i]=" << removeArr[i] << " value=" << value << endl;
                tree.printTree(cout);
                return false;
        }
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

class BeTreeParams {
public:
    uint16_t fanout;
    uint16_t maxBufferSize;
    size_t cacheCapacity;

    uint64_t fitness;

    BeTreeParams() : fanout(0), maxBufferSize(0), cacheCapacity(0), fitness(0) {};

    BeTreeParams(uint16_t fanout, uint16_t maxBufferSize, size_t cacheCapacity)
        : fanout(fanout), maxBufferSize(maxBufferSize), cacheCapacity(cacheCapacity), fitness(0) {};

    BeTreeParams(BeTreeParams minParams, BeTreeParams maxParams) : BeTreeParams(0, 0, 0) {
        // random instance
        std::random_device rd;
        std::mt19937 gen(rd());

        std::uniform_int_distribution<uint16_t> fanoutDist(minParams.fanout, maxParams.fanout);
        std::uniform_int_distribution<uint16_t> bufferDist(minParams.maxBufferSize, maxParams.maxBufferSize);
        std::uniform_int_distribution<size_t> cacheDist(minParams.cacheCapacity, maxParams.cacheCapacity);

        fanout = fanoutDist(gen);
        maxBufferSize = bufferDist(gen);
        cacheCapacity = cacheDist(gen);
    };


    // returns number of insertions and deletions performed in the time limit
    uint64_t evaluateFitness(const std::chrono::milliseconds& timeLimit, int* insertArr, int* removeArr, int testSize) {

        BeTree<int, int> tree(fanout, maxBufferSize, 4096, 1024 * 1024 * 1024, "./filestore.hdb", cacheCapacity);
        auto start = std::chrono::high_resolution_clock::now();

        uint64_t operations = 0;
        int i = 0;
        while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start) < timeLimit) {
            if (i >= testSize) {
                i = 0;
            }
            try {
                tree.insert(insertArr[i], insertArr[i]);
                tree.remove(removeArr[i]);
                operations += 2;
            } catch (std::exception& e) {
                goto finish;
            }
            i++;
        }
    finish:
        fitness = operations;
        return fitness;
    };
};

void geneticAlgorithm(const BeTreeParams& minParams, const BeTreeParams& maxParams, const std::chrono::milliseconds& timeLimit, int populationSize, int numGenerations) {
    std::vector<BeTreeParams> population(populationSize);
    std::generate(population.begin(), population.end(), [&]() { return BeTreeParams(minParams, maxParams); });

    for (int generation = 0; generation < numGenerations; ++generation) {
        int testSize = 1000000;
        int* insertArr = new int[testSize];
        int* removeArr = new int[testSize];
        for (int i = 0; i < testSize; i++) {
            insertArr[i] = i;
            removeArr[i] = i;
        }
        shuffle(insertArr, testSize);
        shuffle(removeArr, testSize);
        for (auto& params : population) {
            params.evaluateFitness(timeLimit, insertArr, removeArr, testSize);
        }
        delete[] insertArr;
        delete[] removeArr;

        // Sort population by fitness
        std::sort(population.begin(), population.end(), [](const BeTreeParams& a, const BeTreeParams& b) {
            return a.fitness > b.fitness;
            });
        std::cout << "Generation " << generation << " best fitness: " << population[0].fitness << std::endl;
        std::cout << "Parameters: " << population[0].fanout << " " << population[0].maxBufferSize << " " << population[0].cacheCapacity << std::endl;
        std::cout.flush();

        // Selection
        int eliteSize = populationSize / 5;
        population.resize(eliteSize);

        // Random individuals
        for (int i = 0; i < eliteSize; ++i) {
            population.push_back(BeTreeParams(minParams, maxParams));
        }

        // Crossover
        while (population.size() < populationSize) {
            int parent1 = rand() % eliteSize;
            int parent2 = rand() % eliteSize;
            // 1-point crossover
            int crossoverPoint = rand() % 4;
            BeTreeParams child = population[parent1];
            switch (crossoverPoint) {
                case 0:
                    child.fanout = population[parent2].fanout;
                    break;
                case 1:
                    child.maxBufferSize = population[parent2].maxBufferSize;
                    break;
                case 3:
                    child.cacheCapacity = population[parent2].cacheCapacity;
                    break;
            }
            population.push_back(child);
        };

        // Mutation
        for (int i = eliteSize; i < populationSize; ++i) {
            // add or subtract 1 if it is within bounds
            if (population[i].fanout > minParams.fanout && population[i].fanout < maxParams.fanout) {
                population[i].fanout += (rand() % 2) * 2 - 1;
            }
            if (population[i].maxBufferSize > minParams.maxBufferSize && population[i].maxBufferSize < maxParams.maxBufferSize) {
                population[i].maxBufferSize += (rand() % 2) * 2 - 1;
            }
            if (population[i].cacheCapacity > minParams.cacheCapacity && population[i].cacheCapacity < maxParams.cacheCapacity) {
                population[i].cacheCapacity += (rand() % 2) * 2 - 1;
            }
        }
        }

    // Print best parameters
    std::cout << "Best parameters: " << std::endl;
    std::cout << "Fanout: " << population[0].fanout << std::endl;
    std::cout << "MaxBufferSize: " << population[0].maxBufferSize << std::endl;
    std::cout << "CacheCapacity: " << population[0].cacheCapacity << std::endl;
    std::cout << "Operations: " << population[0].fitness << std::endl;
    }

int main(int argc, char* argv[]) {

    int fanouts[] = { 2, 3, 4, 5, 6, 7, 8, 16, 32, 64, 128, 256 };
    int bufferSizes[] = { 2, 4, 8, 16, 32, 64, 128, 256 };
    int size = 10;
    for (int i = 0; i < sizeof(fanouts) / sizeof(fanouts[0]); i++) {
        for (int j = 0; j < sizeof(bufferSizes) / sizeof(bufferSizes[0]); j++) {
            int localSize = (fanouts[i] + bufferSizes[j]) * size;
            //int localSize = 100;
            //bool stochastic = localSize > 1500;
            bool stochastic = true;
            int step = localSize / 10;
            testBeTree(fanouts[i], bufferSizes[j], localSize, step, stochastic);
        }
    }
    std::cout << "All tests passed!" << std::endl;
    std::cout << "Starting genetic algorithm..." << std::endl;

    // Genetic algorithm
    BeTreeParams minParams(32, 2, 10);
    BeTreeParams maxParams(1024, 1024, 100);
    std::chrono::milliseconds timeLimit(1000);
    geneticAlgorithm(minParams, maxParams, timeLimit, 25, 100);

    return 0;
}

