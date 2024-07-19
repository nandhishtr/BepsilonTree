#pragma

#include <cstdint>

#include "BeTreeIStorage.hpp"

#define MAGIC_NUMBER 11647106966631696989 // 8 bytes
// the header is 8 + 8 + 2 + 2 = 20 bytes long
// magic number (8 bytes) + root node offset (8 bytes) + fanout (2 bytes) + max buffer size (2 bytes)
#define HEADER_SIZE 20

template <typename KeyType, typename ValueType>
class BeTreeFileStorage : public BeTreeIStorage<KeyType, ValueType> {
    /* Storage class that stores nodes in a file */
private:
    uint64_t numBlocks = 0;
    std::fstream file;
    std::vector<bool> allocationTable;
    using NodePtr = BeTreeIStorage<KeyType, ValueType>::NodePtr;

public:
    BeTreeFileStorage(size_t blockSize, size_t storageSize, const std::string& filename)
        : BeTreeIStorage<KeyType, ValueType>(blockSize, storageSize, filename) {
        numBlocks = storageSize / blockSize;
        allocationTable.resize(numBlocks, false);

        // reserve space for the header and the allocation table
        size_t blocksForHeader = HEADER_SIZE / blockSize + (HEADER_SIZE % blockSize != 0);
        size_t blocksForAllocationTable = numBlocks / blockSize + (numBlocks % blockSize != 0);
        for (size_t i = 0; i < blocksForHeader + blocksForAllocationTable; i++) {
            allocationTable[i] = true;
        }
    }

    ~BeTreeFileStorage() {
        this->flush();
        if (file.is_open()) {
            file.close();
        }
    }

    // returns true if the storage was initialized successfully
    bool init(uint64_t& rootNodeOffset, uint16_t& fanout, uint16_t& maxBufferSize) {
        // if fanout and maxBufferSizes are 0, then read the file
        if (fanout == 0 && maxBufferSize == 0) {
            file.open(this->filename, std::ios::in | std::ios::out | std::ios::binary);
            if (!file.is_open()) {
                return false;
            }

            uint64_t magic;
            file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
            if (magic != MAGIC_NUMBER) {
                return false;
            }

            file.read(reinterpret_cast<char*>(&rootNodeOffset), sizeof(rootNodeOffset));
            file.read(reinterpret_cast<char*>(&fanout), sizeof(fanout));
            file.read(reinterpret_cast<char*>(&maxBufferSize), sizeof(maxBufferSize));
            readAllocationTable(file);
        } else {
            // if fanout and maxBufferSizes are not 0, then overwrite the file
            file.open(this->filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
            if (!file.is_open()) {
                return false;
            }

            file.write(reinterpret_cast<char*>(&MAGIC_NUMBER), sizeof(uint64_t));
            file.write(reinterpret_cast<char*>(&rootNodeOffset), sizeof(rootNodeOffset));
            file.write(reinterpret_cast<char*>(&fanout), sizeof(fanout));
            file.write(reinterpret_cast<char*>(&maxBufferSize), sizeof(maxBufferSize));
            writeAllocationTable(file);
        }

        return true;
    }

    // returns the new id of the node if it was moved/created or 0 if it was not moved
    uint64_t saveNode(uint64_t id, NodePtr node) {
        assert(file.is_open());
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
            file.seekp(start * this->blockSize);
            node->serialize(file);

            // update the allocation table
            for (uint64_t i = start; i <= end; i++) {
                allocationTable[i] = true;

            }

            return start;
        } else {
            // if the node is in the file, then overwrite it
            node->serialize(file);

            return 0;
        }
    }

    void loadNode(uint64_t id, NodePtr node) {
        assert(file.is_open());
        assert(id != 0);

        file.seekg(id * this->blockSize);

        // read the first byte to determine the type of the node
        uint8_t type;
        file.read(reinterpret_cast<char*>(&type), sizeof(type));
        // seek back to the beginning of the node block
        file.seekg(id * this->blockSize);

        if (type == 0) { // internal node
            node = std::make_shared<BeTreeInternalNode<KeyType, ValueType>>(this->fanout, this->maxBufferSize);
            node->deserialize(file);
        } else if (type == 1) { // leaf node
            node = std::make_shared<BeTreeLeafNode<KeyType, ValueType>>(this->maxBufferSize);
            node->deserialize(file);
        } else {
            throw std::runtime_error("Invalid node type");
        }
    }

    void removeNode(uint64_t id, NodePtr node) {
        assert(file.is_open());
        assert(id != 0);

        // update the allocation table
        auto size = node->getSerializedSize();
        auto blocksNeeded = size / this->blockSize + (size % this->blockSize != 0);
        for (uint64_t i = id; i < id + blocksNeeded; i++) {
            allocationTable[i] = false;
        }
    }

    void flush() {
        assert(file.is_open());

        // write the allocation table to the file
        file.seekp(HEADER_SIZE);
        writeAllocationTable(file);
    }

private:
    void writeAllocationTable(std::fstream& file) {
        // NOTE: we can't write the allocation table in one go because std::vector<bool> is a specialization and it's not guaranteed to be contiguous
        for (size_t i = 0; i < numBlocks; i += 8) {
            unsigned char byte = 0;
            for (size_t j = 0; j < 8 && i + j < numBlocks; j++) {
                byte |= allocationTable[i + j] << j;
            }
            file.write(reinterpret_cast<char*>(&byte), sizeof(byte));
        }
    }

    void readAllocationTable(std::fstream& file) {
        // NOTE: we can't read the allocation table in one go because std::vector<bool> is a specialization and it's not guaranteed to be contiguous
        for (size_t i = 0; i < numBlocks; i += 8) {
            unsigned char byte = 0;
            file.read(reinterpret_cast<char*>(&byte), sizeof(byte));
            for (size_t j = 0; j < 8 && i + j < numBlocks; j++) {
                allocationTable[i + j] = byte & (1 << j);
            }
        }
    }
};
