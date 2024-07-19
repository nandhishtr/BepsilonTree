#pragma once

#include "BeTreeFileStorage.hpp"
#include "BeTreeInternalNode.hpp"
#include "BeTreeIStorage.hpp"
#include "BeTreeLeafNode.hpp"
#include "BeTreeLRUCache.hpp"
#include "BeTreeMessage.hpp"
#include "BeTreeNode.hpp"
#include "ErrorCodes.h"
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>

// Forward declarations
template <typename KeyType, typename ValueType> class BeTree;
template <typename KeyType, typename ValueType> class BeTreeNode;
template <typename KeyType, typename ValueType> class BeTreeInternalNode;
template <typename KeyType, typename ValueType> class BeTreeLeafNode;
template <typename KeyType, typename ValueType> class BeTreeLRUCache;
template <typename KeyType, typename ValueType> class BeTreeIStorage;
template <typename KeyType, typename ValueType> struct Message;
enum class ErrorCode;
enum class MessageType;

template <typename KeyType, typename ValueType>
class BeTree {
private:
    using MessagePtr = std::unique_ptr<Message<KeyType, ValueType>>;
    using BeTreeNodePtr = std::shared_ptr<BeTreeNode<KeyType, ValueType>>;
    using InternalNodePtr = std::shared_ptr<BeTreeInternalNode<KeyType, ValueType>>;
    using LeafNodePtr = std::shared_ptr<BeTreeLeafNode<KeyType, ValueType>>;
    using childChange = typename BeTreeNode<KeyType, ValueType>::ChildChange;
    using ChildChangeType = typename BeTreeNode<KeyType, ValueType>::ChildChangeType;
    using CachePtr = std::shared_ptr<BeTreeLRUCache<KeyType, ValueType>>;
    using StoragePtr = std::shared_ptr<BeTreeIStorage<KeyType, ValueType>>;

    uint16_t fanout;
    uint16_t maxBufferSize;
    uint64_t rootNode;
    std::shared_ptr<BeTreeLRUCache<KeyType, ValueType>> cache;
    StoragePtr storage;

    ErrorCode applyMessage(MessagePtr message);
public:
    ~BeTree() = default;
    BeTree() = delete;
    BeTree(StoragePtr storage, size_t cache_capacity) : fanout(0), maxBufferSize(0), rootNode(0) {
        this->storage = storage;
        this->storage.init(this->rootNode, this->fanout, this->maxBufferSize);
        this->cache = std::make_shared<BeTreeLRUCache<KeyType, ValueType>>(cache_capacity, storage);
    }
    BeTree(uint16_t fanout, uint16_t maxBufferSize, size_t blockSize, size_t storageSize, const std::string& filename, size_t cache_capacity)
        : fanout(fanout), maxBufferSize(maxBufferSize), rootNode(0) {
        this->cache = std::make_shared<BeTreeLRUCache<KeyType, ValueType>>(cache_capacity);
        this->storage = std::make_shared<BeTreeFileStorage<KeyType, ValueType>>(blockSize, storageSize, filename, this->cache);
        this->storage->init(this->rootNode, this->fanout, this->maxBufferSize);
        this->cache->setStorage(this->storage);
    }

    ErrorCode insert(const KeyType& key, const ValueType& value);
    ErrorCode remove(const KeyType& key);
    std::pair<ValueType, ErrorCode> search(const KeyType& key);
    ErrorCode flush();
    void printTree(std::ostream& out);
};

// Implementations

template <typename KeyType, typename ValueType>
ErrorCode BeTree<KeyType, ValueType>::insert(const KeyType& key, const ValueType& value) {
    // If the tree is empty, create a new leaf node
    if (rootNode == 0) {
        auto temp = std::make_shared<BeTreeLeafNode<KeyType, ValueType>>(fanout, this->cache);
        cache->create(temp);
        storage->updateRootNode(temp->id);
        rootNode = temp->id;
    }

    MessagePtr message = std::make_unique<Message<KeyType, ValueType>>(MessageType::Insert, key, value);

    return applyMessage(std::move(message));
}

template <typename KeyType, typename ValueType>
ErrorCode BeTree<KeyType, ValueType>::remove(const KeyType& key) {
    if (rootNode == 0) {
        return ErrorCode::KeyDoesNotExist;
    }
    MessagePtr message = std::make_unique<Message<KeyType, ValueType>>(MessageType::Remove, key);

    return applyMessage(std::move(message));
}

template <typename KeyType, typename ValueType>
std::pair<ValueType, ErrorCode> BeTree<KeyType, ValueType>::search(const KeyType& key) {
    if (rootNode == 0) {
        return { ValueType(), ErrorCode::KeyDoesNotExist };
    }
    MessagePtr message = std::make_unique<Message<KeyType, ValueType>>(MessageType::Search, key);
    BeTreeNodePtr node = cache->get(rootNode);
    return node->search(std::move(message));
}

template <typename KeyType, typename ValueType>
void BeTree<KeyType, ValueType>::printTree(std::ostream& out) {
    if (rootNode == 0) {
        out << "Empty tree\n";
        return;
    }
    out << "Tree:\n";
    BeTreeNodePtr node = cache->get(rootNode);
    node->printNode(out);
}

template <typename KeyType, typename ValueType>
ErrorCode BeTree<KeyType, ValueType>::applyMessage(MessagePtr message) {
    childChange childChange = { KeyType(), 0, ChildChangeType::None };
    BeTreeNodePtr node = cache->get(rootNode);
    ErrorCode err = node->applyMessage(std::move(message), 0, childChange);

    // Because of the buffering both insert and remove messages are handled in the same way
    // meaning that the tree can split and merge on both insert and remove operations

    // Handle root node split
    if (childChange.node) {
        auto newRoot = std::make_shared<BeTreeInternalNode<KeyType, ValueType>>(this->fanout, this->cache, this->maxBufferSize);
        newRoot->keys.push_back(childChange.key);
        newRoot->children.push_back(rootNode);
        newRoot->children.push_back(childChange.node);


        cache->create(newRoot);
        storage->updateRootNode(newRoot->id);
        rootNode = newRoot->id;
        BeTreeNodePtr firstChild = cache->get(newRoot->children[0]);
        BeTreeNodePtr secondChild = cache->get(newRoot->children[1]);
        newRoot->lowestSearchKey = firstChild->lowestSearchKey;
        firstChild->parent = newRoot->id;
        secondChild->parent = newRoot->id;
    }

    // Handle root node merge
    if (node->size() == 0) {
        if (node->isLeaf()) {
            cache->deleteNode(rootNode, node);
            rootNode = 0;
        } else {
            InternalNodePtr oldRoot = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(node);
            BeTreeNodePtr firstChild = cache->get(oldRoot->children[0]);
            if (firstChild->isLeaf()) {
                // auto newRoot = std::static_pointer_cast<BeTreeLeafNode<KeyType, ValueType>>(oldRoot->children[0]);
                // newRoot->parent = nullptr;
                //
                // // TODO: handle message buffer of current rootNode to the newRoot because the old root is being deleted
                // // because the leaf node doesn't have a message buffer
                // rootNode = newRoot;
            } else {
                auto newRoot = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(firstChild);
                newRoot->parent = 0;

                // add message buffer of current rootNode to the newRoot because the old root is being deleted
                // for now just move the messages and don't call insert/remove because handling split/merge would be more complicated
                // NOTE: THIS BREAKS SERIALIZATION RIGHT NOW
                for (auto& msg : oldRoot->messageBuffer) {
                    newRoot->messageBuffer.insert_or_assign(msg.first, std::move(msg.second));
                }

                newRoot->leftSibling = 0;
                newRoot->rightSibling = 0;
                rootNode = oldRoot->children[0];
            }
        }
    }

    return err;
}

template <typename KeyType, typename ValueType>
ErrorCode BeTree<KeyType, ValueType>::flush() {
    // TODO: implement
    return ErrorCode::Error;
}
