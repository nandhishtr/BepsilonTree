#pragma once

#include <fstream>
#include <string>
#include <memory>
#include <unordered_map>
#include "BeTreeNode.hpp"

template <typename KeyType, typename ValueType>
class File1Storage {
public:
    using NodePtr = std::shared_ptr<BeTreeNode<KeyType, ValueType>>;
    using NodeId = uint64_t;

    File1Storage(const std::string& filename) : filename_(filename) {}

    void saveNode(NodeId id, NodePtr node) {
        std::ofstream file(filename_, std::ios::binary | std::ios::app);
        if (file.is_open()) {
            // Implement serialization logic here
            // For simplicity, we'll just write the node's data
            file.write(reinterpret_cast<const char*>(&id), sizeof(NodeId));
            // Write node data (you'll need to implement proper serialization)
            // This is just a placeholder
            size_t keySize = node->keys.size();
            file.write(reinterpret_cast<const char*>(&keySize), sizeof(size_t));
            file.write(reinterpret_cast<const char*>(node->keys.data()), keySize * sizeof(KeyType));
            file.close();
        }
    }

    NodePtr loadNode(NodeId id) {
        std::ifstream file(filename_, std::ios::binary);
        if (file.is_open()) {
            NodeId readId;
            while (file.read(reinterpret_cast<char*>(&readId), sizeof(NodeId))) {
                if (readId == id) {
                    // Found the node, deserialize it
                    // This is a placeholder, implement proper deserialization
                    size_t keySize;
                    file.read(reinterpret_cast<char*>(&keySize), sizeof(size_t));
                    std::vector<KeyType> keys(keySize);
                    file.read(reinterpret_cast<char*>(keys.data()), keySize * sizeof(KeyType));

                    // Create and return the node (you'll need to implement proper deserialization)
                    auto node = std::make_shared<BeTreeNode<KeyType, ValueType>>(/* constructor parameters */);
                    node->keys = keys;
                    return node;
                }
                else {
                    // Skip this node's data
                    // You'll need to implement proper skipping based on your serialization format
                    size_t keySize;
                    file.read(reinterpret_cast<char*>(&keySize), sizeof(size_t));
                    file.seekg(keySize * sizeof(KeyType), std::ios::cur);
                }
            }
            file.close();
        }
        return nullptr;
    }

private:
    std::string filename_;
};