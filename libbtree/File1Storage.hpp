#pragma once

#include <fstream>
#include <string>
#include <memory>
#include <vector>
#include "BeTreeNode.hpp"
#include "BeTreeInternalNode.hpp"
#include "BeTreeLeafNode.hpp"

template <typename KeyType, typename ValueType>
class File1Storage {
public:
    using NodePtr = std::shared_ptr<BeTreeNode<KeyType, ValueType>>;
    using NodeId = uint64_t;

    File1Storage(const std::string& filename) : filename_(filename) {}

    void saveNode(NodeId id, NodePtr node) {
        std::ofstream file(filename_, std::ios::binary | std::ios::app);
        if (file.is_open()) {
            file.write(reinterpret_cast<const char*>(&id), sizeof(NodeId));

            // Write node type (0 for leaf, 1 for internal)
            bool isLeaf = node->isLeaf();
            file.write(reinterpret_cast<const char*>(&isLeaf), sizeof(bool));

            // Serialize common node data
            serializeCommonData(file, node);

            // Serialize specific node data
            if (isLeaf) {
                serializeLeafNode(file, std::static_pointer_cast<BeTreeLeafNode<KeyType, ValueType>>(node));
            }
            else {
                serializeInternalNode(file, std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(node));
            }

            file.close();
        }
    }

    NodePtr loadNode(NodeId id) {
        std::ifstream file(filename_, std::ios::binary);
        if (file.is_open()) {
            NodeId readId;
            while (file.read(reinterpret_cast<char*>(&readId), sizeof(NodeId))) {
                if (readId == id) {
                    bool isLeaf;
                    file.read(reinterpret_cast<char*>(&isLeaf), sizeof(bool));

                    if (isLeaf) {
                        return deserializeLeafNode(file);
                    }
                    else {
                        return deserializeInternalNode(file);
                    }
                }
                else {
                    // Skip this node's data
                    skipNodeData(file);
                }
            }
            file.close();
        }
        return nullptr;
    }

private:
    std::string filename_;

    void serializeCommonData(std::ofstream& file, const NodePtr& node) {
        file.write(reinterpret_cast<const char*>(&node->fanout), sizeof(uint16_t));
        file.write(reinterpret_cast<const char*>(&node->level), sizeof(uint16_t));

        size_t keySize = node->keys.size();
        file.write(reinterpret_cast<const char*>(&keySize), sizeof(size_t));
        file.write(reinterpret_cast<const char*>(node->keys.data()), keySize * sizeof(KeyType));
    }

    void serializeLeafNode(std::ofstream& file, const std::shared_ptr<BeTreeLeafNode<KeyType, ValueType>>& leaf) {
        size_t valueSize = leaf->values.size();
        file.write(reinterpret_cast<const char*>(&valueSize), sizeof(size_t));
        file.write(reinterpret_cast<const char*>(leaf->values.data()), valueSize * sizeof(ValueType));
    }

    void serializeInternalNode(std::ofstream& file, const std::shared_ptr<BeTreeInternalNode<KeyType, ValueType>>& internal) {
        size_t bufferSize = internal->messageBuffer.size();
        file.write(reinterpret_cast<const char*>(&bufferSize), sizeof(size_t));
        for (const auto& [key, message] : internal->messageBuffer) {
            file.write(reinterpret_cast<const char*>(&key), sizeof(KeyType));
            file.write(reinterpret_cast<const char*>(&message->type), sizeof(MessageType));
            file.write(reinterpret_cast<const char*>(&message->key), sizeof(KeyType));
            file.write(reinterpret_cast<const char*>(&message->value), sizeof(ValueType));
        }
    }

    NodePtr deserializeLeafNode(std::ifstream& file) {
        uint16_t fanout, level;
        file.read(reinterpret_cast<char*>(&fanout), sizeof(uint16_t));
        file.read(reinterpret_cast<char*>(&level), sizeof(uint16_t));

        auto leaf = std::make_shared<BeTreeLeafNode<KeyType, ValueType>>(fanout);
        leaf->level = level;

        size_t keySize;
        file.read(reinterpret_cast<char*>(&keySize), sizeof(size_t));
        leaf->keys.resize(keySize);
        file.read(reinterpret_cast<char*>(leaf->keys.data()), keySize * sizeof(KeyType));

        size_t valueSize;
        file.read(reinterpret_cast<char*>(&valueSize), sizeof(size_t));
        leaf->values.resize(valueSize);
        file.read(reinterpret_cast<char*>(leaf->values.data()), valueSize * sizeof(ValueType));

        return leaf;
    }

    NodePtr deserializeInternalNode(std::ifstream& file) {
        uint16_t fanout, level;
        file.read(reinterpret_cast<char*>(&fanout), sizeof(uint16_t));
        file.read(reinterpret_cast<char*>(&level), sizeof(uint16_t));

        auto internal = std::make_shared<BeTreeInternalNode<KeyType, ValueType>>(fanout, level);

        size_t keySize;
        file.read(reinterpret_cast<char*>(&keySize), sizeof(size_t));
        internal->keys.resize(keySize);
        file.read(reinterpret_cast<char*>(internal->keys.data()), keySize * sizeof(KeyType));

        size_t bufferSize;
        file.read(reinterpret_cast<char*>(&bufferSize), sizeof(size_t));
        for (size_t i = 0; i < bufferSize; ++i) {
            KeyType key;
            file.read(reinterpret_cast<char*>(&key), sizeof(KeyType));

            MessageType messageType;
            KeyType key;
            ValueType value;
            file.read(reinterpret_cast<char*>(&messageType), sizeof(MessageType));
            file.read(reinterpret_cast<char*>(&key), sizeof(KeyType));
            file.read(reinterpret_cast<char*>(&value), sizeof(ValueType));
            
            auto message = std::make_unique<Message<KeyType, ValueType>>(messageType, key, value);

            internal->messageBuffer.insert_or_assign(key, std::move(message));
        }

        return internal;
    }

    void skipNodeData(std::ifstream& file) {
        bool isLeaf;
        file.read(reinterpret_cast<char*>(&isLeaf), sizeof(bool));

        uint16_t fanout, level;
        file.read(reinterpret_cast<char*>(&fanout), sizeof(uint16_t));
        file.read(reinterpret_cast<char*>(&level), sizeof(uint16_t));

        size_t keySize;
        file.read(reinterpret_cast<char*>(&keySize), sizeof(size_t));
        file.seekg(keySize * sizeof(KeyType), std::ios::cur);

        if (isLeaf) {
            size_t valueSize;
            file.read(reinterpret_cast<char*>(&valueSize), sizeof(size_t));
            file.seekg(valueSize * sizeof(ValueType), std::ios::cur);
        }
        else {
            size_t bufferSize;
            file.read(reinterpret_cast<char*>(&bufferSize), sizeof(size_t));
            file.seekg(bufferSize * (sizeof(KeyType) + sizeof(MessageType) + sizeof(KeyType) + sizeof(ValueType)), std::ios::cur);
        }
    }
};