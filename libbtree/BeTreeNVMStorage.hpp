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
#include <cassert>
#include <cstring>
#include <stdexcept>

#define MAGIC_NUMBER 11647106966631696989 // 8 bytes
#define HEADER_SIZE 20 // magic number (8 bytes) + root node offset (8 bytes) + fanout (2 bytes) + max buffer size (2 bytes)

template <typename KeyType, typename ValueType>
class BeTreeNVMStorage : public BeTreeIStorage<KeyType, ValueType> {
private:
    using NodePtr = typename BeTreeIStorage<KeyType, ValueType>::NodePtr;
    using CachePtr = std::weak_ptr<BeTreeLRUCache<KeyType, ValueType>>;

    char* mappedAddr;
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
        if (mappedAddr != nullptr) {
            pmem_unmap(mappedAddr, mappedLen);
        }
    }

    bool init(uint64_t& rootNodeOffset, uint16_t& fanout, uint16_t& maxBufferSize) override {
        if (fanout == 0 && maxBufferSize == 0) {
            // Try to open existing file
            mappedAddr = (char*)pmem_map_file(this->filename.c_str(), 0, 0, 0666, &mappedLen, &isPmem);
            if (mappedAddr == nullptr) {
                return false;
            }

            // Read header
            uint64_t magic;
            size_t offset = 0;
            memcpy(&magic, mappedAddr + offset, sizeof(magic));
            offset += sizeof(magic);
            if (magic != MAGIC_NUMBER) {
                pmem_unmap(mappedAddr, mappedLen);
                return false;
            }

            pmem_memcpy_persist(&rootNodeOffset, mappedAddr + offset, sizeof(rootNodeOffset));
            offset += sizeof(rootNodeOffset);
            pmem_memcpy_persist(&fanout, mappedAddr + offset, sizeof(fanout));
            offset += sizeof(fanout);
            pmem_memcpy_persist(&maxBufferSize, mappedAddr + offset, sizeof(maxBufferSize));
            offset += sizeof(maxBufferSize);
            readAllocationTable();
        } else {
            // Create new file
            mappedAddr = (char*)pmem_map_file(this->filename.c_str(), this->storageSize, PMEM_FILE_CREATE, 0666, &mappedLen, &isPmem);
            if (mappedAddr == nullptr) {
                return false;
            }

            // Write header
            uint64_t magic = MAGIC_NUMBER;
            size_t offset = 0;
            pmem_memcpy_persist(mappedAddr + offset, &magic, sizeof(magic));
            offset += sizeof(magic);
            pmem_memcpy_persist(mappedAddr + offset, &rootNodeOffset, sizeof(rootNodeOffset));
            offset += sizeof(rootNodeOffset);
            pmem_memcpy_persist(mappedAddr + offset, &fanout, sizeof(fanout));
            offset += sizeof(fanout);
            pmem_memcpy_persist(mappedAddr + offset, &maxBufferSize, sizeof(maxBufferSize));
            offset += sizeof(maxBufferSize);
            writeAllocationTable();
        }

        this->fanout = fanout;
        this->maxBufferSize = maxBufferSize;

        return true;
    }

    uint64_t saveNode(uint64_t id, NodePtr node) override {
        assert(mappedAddr != nullptr);
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
                } else {
                    count = 0;
                }
            }
            if (count < blocksNeeded) {
                return 0;
            }

            // Write the node to PMEM
            size_t offset = start * this->blockSize;
            size_t bytesWritten = node->serialize(mappedAddr + offset);
            pmem_persist(mappedAddr + offset, bytesWritten);

            // Update the allocation table
            for (uint64_t i = start; i <= end; i++) {
                allocationTable[i] = true;
            }

            return start;
        } else {
            // Overwrite existing node
            size_t offset = id * this->blockSize;
            size_t bytesWritten = node->serialize(mappedAddr + offset);
            pmem_persist(mappedAddr + offset, bytesWritten);

            return 0;
        }
    }

    NodePtr loadNode(uint64_t id) override {
        assert(mappedAddr != nullptr);
        assert(id != 0);

        size_t offset = id * this->blockSize;
        // Read the first byte to determine the type of the node
        uint8_t type;
        pmem_memcpy_persist(&type, mappedAddr + offset, sizeof(type));

        NodePtr node;
        if (type == 0) { // internal node
            node = std::make_shared<BeTreeInternalNode<KeyType, ValueType>>(this->fanout, this->cache.lock(), this->maxBufferSize);
        } else if (type == 1) { // leaf node
            node = std::make_shared<BeTreeLeafNode<KeyType, ValueType>>(this->fanout, this->cache.lock());
        } else {
            throw std::runtime_error("Invalid node type");
        }
        node->deserialize(mappedAddr + offset);
        node->id = id;
        return node;
    }

    void removeNode(uint64_t id, NodePtr node) override {
        assert(mappedAddr != nullptr);
        assert(node != nullptr);
        assert(id != 0);

        // Update the allocation table
        auto size = node->getSerializedSize();
        auto blocksNeeded = size / this->blockSize + (size % this->blockSize != 0);
        for (uint64_t i = id; i < id + blocksNeeded; i++) {
            allocationTable[i] = false;
        }
    }

    void updateRootNode(uint64_t rootNodeOffset) {
        assert(mappedAddr != nullptr);
        assert(rootNodeOffset != 0);

        size_t offset = sizeof(uint64_t);
        pmem_memcpy_persist(mappedAddr + offset, &rootNodeOffset, sizeof(rootNodeOffset));
    }

    void flush() override {
        assert(mappedAddr != nullptr);

        // Write the allocation table to PMEM
        writeAllocationTable();

        pmem_persist(mappedAddr, mappedLen);
    }

private:
    void writeAllocationTable() {
        // NOTE: we can't write the allocation table in one go because std::vector<bool> is a specialization and it's not guaranteed to be contiguous
        size_t offset = HEADER_SIZE;
        for (size_t i = 0; i < numBlocks; i += 8) {
            unsigned char byte = 0;
            for (size_t j = 0; j < 8 && i + j < numBlocks; j++) {
                byte |= allocationTable[i + j] << j;
            }
            pmem_memcpy_persist(mappedAddr + offset, &byte, sizeof(byte));
            offset += sizeof(byte);
        }
    }

    void readAllocationTable() {
        // NOTE: we can't read the allocation table in one go because std::vector<bool> is a specialization and it's not guaranteed to be contiguous
        size_t offset = HEADER_SIZE;
        for (size_t i = 0; i < numBlocks; i += 8) {
            unsigned char byte;
            pmem_memcpy_persist(&byte, mappedAddr + offset, sizeof(byte));
            offset += sizeof(byte);
            for (size_t j = 0; j < 8 && i + j < numBlocks; j++) {
                allocationTable[i + j] = byte & (1 << j);
            }
        }
    }
};
