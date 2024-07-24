#pragma once

#include <cstdint>

#include "BeTreeInternalNode.hpp"
#include "BeTreeIStorage.hpp"
#include "BeTreeLeafNode.hpp"
#include "BeTreeLRUCache.hpp"
#include <cassert>
#include <fstream>
#include <iosfwd>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MAGIC_NUMBER 11647106966631696989 // 8 bytes
// the header is 8 + 8 + 2 + 2 = 20 bytes long
// magic number (8 bytes) + root node offset (8 bytes) + fanout (2 bytes) + max buffer size (2 bytes)
#define HEADER_SIZE 20

template <typename KeyType, typename ValueType>
class BeTreeFileMapStorage : public BeTreeIStorage<KeyType, ValueType> {
    /* Storage class that stores nodes in a memory-mapped file */
private:
    using NodePtr = BeTreeIStorage<KeyType, ValueType>::NodePtr;
    using CachePtr = std::weak_ptr<BeTreeLRUCache<KeyType, ValueType>>;
    uint64_t numBlocks = 0;
    int fd;
    char* mappedAddr;
    std::vector<bool> allocationTable;
    uint16_t fanout;
    uint16_t maxBufferSize;
    CachePtr cache;

public:
    BeTreeFileMapStorage(size_t blockSize, size_t storageSize, const std::string& filename, CachePtr cache)
        : BeTreeIStorage<KeyType, ValueType>(blockSize, storageSize, filename), cache(cache) {
        numBlocks = storageSize / blockSize;
        allocationTable.resize(numBlocks, false);

        // reserve space for the header and the allocation table
        size_t blocksForHeader = HEADER_SIZE / blockSize + (HEADER_SIZE % blockSize != 0);
        size_t blocksForAllocationTable = numBlocks / blockSize + (numBlocks % blockSize != 0);
        for (size_t i = 0; i < blocksForHeader + blocksForAllocationTable; i++) {
            allocationTable[i] = true;
        }
    }

    ~BeTreeFileMapStorage() {
        this->flush();
        if (mappedAddr != nullptr) {
            munmap(mappedAddr, this->storageSize);
        }
        if (fd != -1) {
            close(fd);
        }
    }

    // returns true if the storage was initialized successfully
    bool init(uint64_t& rootNodeOffset, uint16_t& fanout, uint16_t& maxBufferSize) override {
        // if fanout and maxBufferSizes are 0, then read the file
        if (fanout == 0 && maxBufferSize == 0) {
            fd = open(this->filename.c_str(), O_RDWR, S_IRUSR | S_IWUSR);
            if (fd == -1) {
                return false;
            }

            // map the file
            mappedAddr = static_cast<char*>(mmap(nullptr, this->storageSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
            if (mappedAddr == MAP_FAILED) {
                close(fd);
                return false;
            }

            // read the header
            uint64_t magic;
            size_t offset = 0;
            memcpy(&magic, mappedAddr + offset, sizeof(uint64_t));
            offset += sizeof(uint64_t);
            if (magic != MAGIC_NUMBER) {
                munmap(mappedAddr, this->storageSize);
                close(fd);
                return false;
            }

            memcpy(&rootNodeOffset, mappedAddr + offset, sizeof(uint64_t));
            offset += sizeof(uint64_t);

            memcpy(&fanout, mappedAddr + offset, sizeof(uint16_t));
            offset += sizeof(uint16_t);

            memcpy(&maxBufferSize, mappedAddr + offset, sizeof(uint16_t));
            offset += sizeof(uint16_t);

            readAllocationTable();
        } else {
            // write the header
            fd = open(this->filename.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
            if (fd == -1) {
                return false;
            }

            mappedAddr = static_cast<char*>(mmap(nullptr, this->storageSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
            if (mappedAddr == MAP_FAILED) {
                close(fd);
                return false;
            }

            uint64_t magic = MAGIC_NUMBER;
            size_t offset = 0;
            memcpy(mappedAddr + offset, &magic, sizeof(uint64_t));
            offset += sizeof(uint64_t);

            memcpy(mappedAddr + offset, &rootNodeOffset, sizeof(uint64_t));
            offset += sizeof(uint64_t);

            memcpy(mappedAddr + offset, &fanout, sizeof(uint16_t));
            offset += sizeof(uint16_t);

            memcpy(mappedAddr + offset, &maxBufferSize, sizeof(uint16_t));
            offset += sizeof(uint16_t);

            writeAllocationTable();
        }
    }

    // returns the new id of the node if it was moved/created or 0 if it was not moved
    uint64_t saveNode(uint64_t id, NodePtr node) {
        assert(mappedAddr != nullptr);
        assert(node != nullptr);

        // if the node is not in the file, then write it to the file
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

            // write the node to the file
            size_t offset = start * this->blockSize + HEADER_SIZE;
            size_t bytesWritten = node->serialize(mappedAddr + offset);

            // update the allocation table
            for (uint64_t i = start; i <= end; i++) {
                allocationTable[i] = true;
            }

            return start;
        } else {
            // if the node is in the file, then overwrite it
            size_t offset = id * this->blockSize + HEADER_SIZE;
            size_t bytesWritten = node->serialize(mappedAddr + offset);

            return 0;
        }
    }

    NodePtr loadNode(uint64_t id) {
        assert(mappedAddr != nullptr);
        assert(id != 0);

        // read the first byte to determine the type of the node
        size_t offset = id * this->blockSize + HEADER_SIZE;
        uint8_t type;
        memcpy(&type, mappedAddr + offset, sizeof(uint8_t));
        NodePtr node;
        if (type == 0) { // internal node
            node = std::make_shared<BeTreeInternalNode<KeyType, ValueType>>(this->fanout, this->cache.lock(), this->maxBufferSize);
            node->deserialize(mappedAddr + offset);
        } else if (type == 1) { // leaf node
            node = std::make_shared<BeTreeLeafNode<KeyType, ValueType>>(this->fanout, this->cache.lock());
            node->deserialize(mappedAddr + offset);
        } else {
            throw std::runtime_error("Invalid node type");
        }
        node->id = id;

        return node;
    }

    void removeNode(uint64_t id, NodePtr node) {
        assert(mappedAddr != nullptr);
        assert(node != nullptr);
        assert(id != 0);

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
        memcpy(mappedAddr + offset, &rootNodeOffset, sizeof(uint64_t));
    }

    void flush() {
        assert(mappedAddr != nullptr);
        writeAllocationTable();
        if (msync(mappedAddr, this->storageSize, MS_SYNC) == -1) {
            throw std::runtime_error("Failed to flush the memory-mapped file");
        }
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
            memcpy(mappedAddr + offset, &byte, sizeof(byte));
            offset += sizeof(byte);
        }
    }

    void readAllocationTable() {
        // NOTE: we can't read the allocation table in one go because std::vector<bool> is a specialization and it's not guaranteed to be contiguous
        size_t offset = HEADER_SIZE;
        for (size_t i = 0; i < numBlocks; i += 8) {
            unsigned char byte;
            memcpy(&byte, mappedAddr + offset, sizeof(byte));
            offset += sizeof(byte);
            for (size_t j = 0; j < 8 && i + j < numBlocks; j++) {
                allocationTable[i + j] = byte & (1 << j);
            }
        }
    }
};
