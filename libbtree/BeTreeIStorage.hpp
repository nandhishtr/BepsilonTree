#pragma once

#include "BeTreeNode.hpp"
#include <cstdint>
#include <memory>
#include <string>

// Forward declarations
template <typename KeyType, typename ValueType> class BeTreeNode;
template <typename KeyType, typename ValueType> class BeTreeInternalNode;
template <typename KeyType, typename ValueType> class BeTreeLeafNode;
template <typename KeyType, typename ValueType> struct Message;

template <typename KeyType, typename ValueType>
class BeTreeIStorage {
    /* Interface for storage classes to store nodes */
protected:
    size_t blockSize;
    size_t storageSize;
    std::string filename;

public:
    using NodePtr = std::shared_ptr<BeTreeNode<KeyType, ValueType>>;

    BeTreeIStorage(size_t blockSize, size_t storageSize, const std::string& filename) : blockSize(blockSize), storageSize(storageSize), filename(filename) {}
    virtual ~BeTreeIStorage() = default;
    virtual bool init(uint64_t& rootNodeOffset, uint16_t& fanout, uint16_t& maxBufferSize) = 0; // returns true if the storage was initialized successfully
    virtual uint64_t saveNode(uint64_t id, NodePtr node) = 0; // returns the new id of the node if it was moved/created or 0 if it was not moved
    virtual void loadNode(uint64_t id, NodePtr node) = 0;
    virtual void removeNode(uint64_t id, NodePtr node) = 0;
    virtual void updateRootNode(uint64_t rootNodeOffset) = 0;

    virtual void flush() = 0;
};