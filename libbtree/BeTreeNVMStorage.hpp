#pragma once

#include <cstdint>
#include <libpmem.h>
#include <memory>
#include <string>
#include <vector>

#include "BeTreeInternalNode.hpp"
#include "BeTreeIStorage.hpp"
#include "BeTreeLeafNode.hpp"
#include "BeTreeLRUCache.hpp"

#define MAGIC_NUMBER 11647106966631696989 // 8 bytes
#define HEADER_SIZE 20 // magic number (8 bytes) + root node offset (8 bytes) + fanout (2 bytes) + max buffer size (2 bytes)

template <typename KeyType, typename ValueType>
class BeTreeNVMStorage : public BeTreeIStorage<KeyType, ValueType> {
private:
    using NodePtr = typename BeTreeIStorage<KeyType, ValueType>::NodePtr;
    using CachePtr = std::shared_ptr<BeTreeLRUCache<KeyType, ValueType>>;

    void* pmemAddr;
    size_t mappedLen;
    int isPmem;
    uint64_t numBlocks;
    std::vector<bool> allocationTable;
    uint16_t fanout;
    uint16_t maxBufferSize;
    CachePtr cache;

public:
    BeTreeNVMStorage(size_t blockSize, size_t storageSize, const std::string& filename, CachePtr cache)
        : BeTreeIStorage<KeyType, ValueType>(blockSize, storageSize, filename), cache(cache) {
        numBlocks = storageSize / blockSize;
        allocationTable.resize(numBlocks, false);

        // Reserve space for the header and the allocation table
        size_t blocksForHeader = HEADER_SIZE / blockSize + (HEADER_SIZE % blockSize != 0);
        size_t blocksForAllocationTable = numBlocks / blockSize + (numBlocks % blockSize != 0);
        for (size_t i = 0; i < blocksForHeader + blocksForAllocationTable; i++) {
            allocationTable[i] = true;
        }
    }

    ~BeTreeNVMStorage() {
        this->flush();
        if (pmemAddr != nullptr) {
            pmem_unmap(pmemAddr, mappedLen);
        }
    }

    bool init(uint64_t& rootNodeOffset, uint16_t& fanout, uint16_t& maxBufferSize) override {
        if (fanout == 0 && maxBufferSize == 0) {
            // Try to open existing file
            pmemAddr = pmem_map_file(this->filename.c_str(), 0, 0, 0666, &mappedLen, &isPmem);
            if (pmemAddr == nullptr) {
                return false;
            }

            // Read header
            uint64_t magic;
            memcpy(&magic, pmemAddr, sizeof(magic));
            if (magic != MAGIC_NUMBER) {
                pmem_unmap(pmemAddr, mappedLen);
                return false;
            }

            memcpy(&rootNodeOffset, static_cast<char*>(pmemAddr) + sizeof(magic), sizeof(rootNodeOffset));
            memcpy(&fanout, static_cast<char*>(pmemAddr) + sizeof(magic) + sizeof(rootNodeOffset), sizeof(fanout));
            memcpy(&maxBufferSize, static_cast<char*>(pmemAddr) + sizeof(magic) + sizeof(rootNodeOffset) + sizeof(fanout), sizeof(maxBufferSize));
            readAllocationTable();
        }
        else {
            // Create new file
            pmemAddr = pmem_map_file(this->filename.c_str(), this->storageSize, PMEM_FILE_CREATE, 0666, &mappedLen, &isPmem);
            if (pmemAddr == nullptr) {
                return false;
            }

            // Write header
            uint64_t magic = MAGIC_NUMBER;
            pmem_memcpy_persist(pmemAddr, &magic, sizeof(magic));
            pmem_memcpy_persist(static_cast<char*>(pmemAddr) + sizeof(magic), &rootNodeOffset, sizeof(rootNodeOffset));
            pmem_memcpy_persist(static_cast<char*>(pmemAddr) + sizeof(magic) + sizeof(rootNodeOffset), &fanout, sizeof(fanout));
            pmem_memcpy_persist(static_cast<char*>(pmemAddr) + sizeof(magic) + sizeof(rootNodeOffset) + sizeof(fanout), &maxBufferSize, sizeof(maxBufferSize));
            writeAllocationTable();
        }

        this->fanout = fanout;
        this->maxBufferSize = maxBufferSize;

        return true;
    }

    uint64_t saveNode(uint64_t id, NodePtr node) override {
        assert(pmemAddr != nullptr);
        assert(node != nullptr);

        if (id == 0) {
            auto size = node->getSerializedSize();
            auto blocksNeeded = size / this->blockSize + (size % this->blockSize != 0);

            // Find blocksNeeded free contiguous blocks
            uint64_t start = 0;
            uint64_t end = 0;
            uint64_t count = 0;
            for (uint64_t i = 0; i < numBlocks; i++) {
                if (!allocationTable[i]) {
                    if (count == 0) {
                        start = i;
                    }
                    count++;
                    if (count == blocksNeeded) {
                        end = i;
                        break;
                    }
                }
                else {
                    count = 0;
                }
            }
            if (count < blocksNeeded) {
                return 0;
            }

            // Write the node to PMEM
            void* destAddr = static_cast<char*>(pmemAddr) + start * this->blockSize;
            size_t writtenSize = 0;
            node->serialize([&](const char* data, size_t length) {
                pmem_memcpy_persist(static_cast<char*>(destAddr) + writtenSize, data, length);
                writtenSize += length;
                });

            // Update the allocation table
            for (uint64_t i = start; i <= end; i++) {
                allocationTable[i] = true;
            }

            return start;
        }
        else {
            // Overwrite existing node
            void* destAddr = static_cast<char*>(pmemAddr) + id * this->blockSize;
            size_t writtenSize = 0;
            node->serialize([&](const char* data, size_t length) {
                pmem_memcpy_persist(static_cast<char*>(destAddr) + writtenSize, data, length);
                writtenSize += length;
                });
            return 0;
        }
    }

    void loadNode(uint64_t id, NodePtr& node) override {
        assert(pmemAddr != nullptr);
        assert(id != 0);

        void* srcAddr = static_cast<char*>(pmemAddr) + id * this->blockSize;

        // Read the first byte to determine the type of the node
        uint8_t type;
        memcpy(&type, srcAddr, sizeof(type));

        if (type == 0) { // internal node
            node = std::make_shared<BeTreeInternalNode<KeyType, ValueType>>(this->fanout, this->cache, this->maxBufferSize);
        }
        else if (type == 1) { // leaf node
            node = std::make_shared<BeTreeLeafNode<KeyType, ValueType>>(this->maxBufferSize, this->cache);
        }
        else {
            throw std::runtime_error("Invalid node type");
        }

        node->deserialize([&](char* data, size_t length) {
            memcpy(data, srcAddr, length);
            srcAddr = static_cast<char*>(srcAddr) + length;
            });
    }

    void removeNode(uint64_t id, NodePtr node) override {
        assert(pmemAddr != nullptr);
        assert(id != 0);

        // Update the allocation table
        auto size = node->getSerializedSize();
        auto blocksNeeded = size / this->blockSize + (size % this->blockSize != 0);
        for (uint64_t i = id; i < id + blocksNeeded; i++) {
            allocationTable[i] = false;
        }
    }

    void updateRootNode(uint64_t rootNodeOffset) override {
        assert(pmemAddr != nullptr);

        pmem_memcpy_persist(static_cast<char*>(pmemAddr) + sizeof(uint64_t), &rootNodeOffset, sizeof(rootNodeOffset));
    }

    void flush() override {
        assert(pmemAddr != nullptr);

        // Write the allocation table to PMEM
        writeAllocationTable();
    }

private:
    void writeAllocationTable() {
        size_t allocationTableSize = (numBlocks + 7) / 8; // Round up to nearest byte
        void* destAddr = static_cast<char*>(pmemAddr) + HEADER_SIZE;

        for (size_t i = 0; i < allocationTableSize; i++) {
            uint8_t byte = 0;
            for (size_t j = 0; j < 8 && i * 8 + j < numBlocks; j++) {
                if (allocationTable[i * 8 + j]) {
                    byte |= (1 << j);
                }
            }
            pmem_memcpy_persist(static_cast<char*>(destAddr) + i, &byte, sizeof(uint8_t));
        }
    }

    void readAllocationTable() {
        size_t allocationTableSize = (numBlocks + 7) / 8; // Round up to nearest byte
        void* srcAddr = static_cast<char*>(pmemAddr) + HEADER_SIZE;

        for (size_t i = 0; i < allocationTableSize; i++) {
            uint8_t byte;
            memcpy(&byte, static_cast<char*>(srcAddr) + i, sizeof(uint8_t));
            for (size_t j = 0; j < 8 && i * 8 + j < numBlocks; j++) {
                allocationTable[i * 8 + j] = (byte & (1 << j)) != 0;
            }
        }
    }
};