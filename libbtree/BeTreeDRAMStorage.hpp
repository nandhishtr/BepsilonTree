#pragma once

#include "BeTreeInternalNode.hpp"
#include "BeTreeIStorage.hpp"
#include "BeTreeLeafNode.hpp"
#include "BeTreeLRUCache.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <vector>

#define MAGIC_NUMBER 11647106966631696989 // 8 bytes
// the header is 8 + 8 + 2 + 2 = 20 bytes long
// magic number (8 bytes) + root node offset (8 bytes) + fanout (2 bytes) + max buffer size (2 bytes)
#define HEADER_SIZE 20

template <typename KeyType, typename ValueType>
class BeTreeDRAMStorage : public BeTreeIStorage<KeyType, ValueType> {
    /* Storage class that stores nodes in a big memory block */
private:
    using NodePtr = BeTreeIStorage<KeyType, ValueType>::NodePtr;
    using CachePtr = std::weak_ptr<BeTreeLRUCache<KeyType, ValueType>>;
    uint64_t numBlocks = 0;
    char* mappedAddr;
    std::vector<bool> allocationTable;
    uint16_t fanout;
    uint16_t maxBufferSize;
    CachePtr cache;

public:
    BeTreeDRAMStorage(size_t blockSize, size_t storageSize, CachePtr cache)
        : BeTreeIStorage<KeyType, ValueType>(blockSize, storageSize, ""), cache(cache), mappedAddr(nullptr) {
        numBlocks = storageSize / blockSize;
        allocationTable.resize(numBlocks, false);

        // reserve space for the header and the allocation table
        size_t blocksForHeader = HEADER_SIZE / blockSize + (HEADER_SIZE % blockSize != 0);
        size_t blocksForAllocationTable = numBlocks / blockSize + (numBlocks % blockSize != 0);
        for (size_t i = 0; i < blocksForHeader + blocksForAllocationTable; i++) {
            allocationTable[i] = true;
        }
    }

    ~BeTreeDRAMStorage() {
        this->flush();
        if (mappedAddr != nullptr) {
            delete[] mappedAddr;
        }
    }

    bool init(uint64_t& rootNodeOffset, uint16_t& fanout, uint16_t& maxBufferSize) override {
        assert(mappedAddr == nullptr);
        assert(rootNodeOffset == 0);
        assert(fanout != 0 && maxBufferSize != 0 && "fanout and maxBufferSize must be set before calling init on dram storage");

        mappedAddr = new char[this->storageSize];
        uint64_t magic = MAGIC_NUMBER;
        size_t offset = 0;
        std::memcpy(mappedAddr + offset, &magic, sizeof(magic));
        offset += sizeof(magic);
        std::memcpy(mappedAddr + offset, &rootNodeOffset, sizeof(rootNodeOffset));
        offset += sizeof(rootNodeOffset);
        std::memcpy(mappedAddr + offset, &fanout, sizeof(fanout));
        offset += sizeof(fanout);
        std::memcpy(mappedAddr + offset, &maxBufferSize, sizeof(maxBufferSize));
        offset += sizeof(maxBufferSize);

        writeAllocationTable();

        this->fanout = fanout;
        this->maxBufferSize = maxBufferSize;

        return true;
    }

    // returns the new id of the node if it was moved/created or 0 if it was not moved
    uint64_t saveNode(uint64_t id, NodePtr node) override {
        assert(mappedAddr != nullptr);
        assert(node != nullptr);

        // if the node is not created, then it is a new node
        if (id == 0) {
            auto size = node->getSerializedSize();
            auto blocksNeeded = size / this->blockSize + (size % this->blockSize != 0);

            // find blocksNeeded free contiguous blocks
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

            //size_t padding = this->blockSize - (size % this->blockSize);

            // write the node to the buffer
            size_t offset = start * this->blockSize;
            size_t bytesWritten = node->serialize(mappedAddr + offset);
            node->id = start;
            //memset(mappedAddr + offset + bytesWritten, 0, padding);

            // update the allocation table
            for (uint64_t i = start; i <= end; i++) {
                allocationTable[i] = true;
            }

            return start;
        } else {
            // if the node is in the buffer, then update it
            size_t offset = id * this->blockSize;
            size_t bytesWritten = node->serialize(mappedAddr + offset);

            return 0;
        }
    }

    NodePtr loadNode(uint64_t id) {
        assert(mappedAddr != nullptr);
        assert(id != 0);

        size_t offset = id * this->blockSize;
        // read the first byte to determine the type of the node
        char type = mappedAddr[offset];
        NodePtr node;
        if (type == 0) { // internal node
            node = std::make_shared<BeTreeInternalNode<KeyType, ValueType>>(this->fanout, this->cache.lock(), this->maxBufferSize);
        } else if (type == 1) {
            node = std::make_shared<BeTreeLeafNode<KeyType, ValueType>>(this->fanout, this->cache.lock());
        } else {
            throw std::runtime_error("Invalid node type");
        }
        node->deserialize(mappedAddr + offset);
        node->id = id;
        return node;
    }

    void removeNode(uint64_t id, NodePtr node) {
        assert(mappedAddr != nullptr);
        assert(id != 0);
        assert(node != nullptr);

        // update the allocation table
        size_t size = node->getSerializedSize();
        size_t blocksNeeded = size / this->blockSize + (size % this->blockSize != 0);
        for (uint64_t i = id; i < id + blocksNeeded; i++) {
            allocationTable[i] = false;
        }
    }

    void updateRootNode(uint64_t rootNodeOffset) {
        assert(mappedAddr != nullptr);
        assert(rootNodeOffset != 0);

        size_t offset = sizeof(uint64_t);
        std::memcpy(mappedAddr + offset, &rootNodeOffset, sizeof(rootNodeOffset));
    }

    void flush() {
        assert(mappedAddr != nullptr);
        writeAllocationTable();
    }

private:
    void writeAllocationTable() {
        // NOTE: we can't write the allocation table in one go because std::vector<bool> is a specialization and it's not guaranteed to be contiguous
        assert(mappedAddr != nullptr);

        size_t offset = HEADER_SIZE;
        for (size_t i = 0; i < numBlocks; i += 8) {
            unsigned char byte = 0;
            for (size_t j = 0; j < 8 && i + j < numBlocks; j++) {
                byte |= allocationTable[i + j] << j;
            }
            mappedAddr[offset] = byte;
            offset += sizeof(byte);
        }
    }

    void readAllocationTable() {
        // NOTE: we can't read the allocation table in one go because std::vector<bool> is a specialization and it's not guaranteed to be contiguous
        assert(mappedAddr != nullptr);

        size_t offset = HEADER_SIZE;
        for (size_t i = 0; i < numBlocks; i += 8) {
            unsigned char byte = mappedAddr[offset];
            for (size_t j = 0; j < 8 && i + j < numBlocks; j++) {
                allocationTable[i + j] = byte & (1 << j);
            }
            offset += sizeof(byte);
        }
    }
};
