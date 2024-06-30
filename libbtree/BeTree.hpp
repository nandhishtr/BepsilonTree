#pragma once

#include "BeTreeInternalNode.hpp"
#include "BeTreeLeafNode.hpp"
#include "BeTreeMessage.hpp"
#include "BeTreeNode.hpp"
#include "ErrorCodes.h"
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <utility>

// Forward declarations
template <typename KeyType, typename ValueType> class BeTree;
template <typename KeyType, typename ValueType> class BeTreeNode;
template <typename KeyType, typename ValueType> class BeTreeInternalNode;
template <typename KeyType, typename ValueType> class BeTreeLeafNode;
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

    uint16_t fanout;
    uint16_t maxBufferSize;
    BeTreeNodePtr rootNode;

    ErrorCode applyMessage(MessagePtr message);
public:
    ~BeTree() = default;
    BeTree(uint16_t fanout, uint16_t maxBufferSize = 0) : fanout(fanout), maxBufferSize(maxBufferSize), rootNode(nullptr) {}

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
    if (!rootNode) {
        rootNode = std::make_shared<BeTreeLeafNode<KeyType, ValueType>>(fanout);
    }

    MessagePtr message = std::make_unique<Message<KeyType, ValueType>>(MessageType::Insert, key, value);

    return applyMessage(std::move(message));
}

template <typename KeyType, typename ValueType>
ErrorCode BeTree<KeyType, ValueType>::remove(const KeyType& key) {
    if (!rootNode) {
        return ErrorCode::KeyDoesNotExist;
    }
    MessagePtr message = std::make_unique<Message<KeyType, ValueType>>(MessageType::Remove, key);

    return applyMessage(std::move(message));
}

template <typename KeyType, typename ValueType>
std::pair<ValueType, ErrorCode> BeTree<KeyType, ValueType>::search(const KeyType& key) {
    if (!rootNode) {
        return { ValueType(), ErrorCode::KeyDoesNotExist };
    }
    MessagePtr message = std::make_unique<Message<KeyType, ValueType>>(MessageType::Search, key);
    return rootNode->search(std::move(message));
}

template <typename KeyType, typename ValueType>
void BeTree<KeyType, ValueType>::printTree(std::ostream& out) {
    if (!rootNode) {
        out << "Empty tree\n";
        return;
    }
    rootNode->printNode(out);
}

template <typename KeyType, typename ValueType>
ErrorCode BeTree<KeyType, ValueType>::applyMessage(MessagePtr message) {
    childChange childChange = { KeyType(), nullptr, ChildChangeType::None };
    ErrorCode err = rootNode->applyMessage(std::move(message), 0, childChange);

    // Because of the buffering both insert and remove messages are handled in the same way
    // meaning that the tree can split and merge on both insert and remove operations

    // Handle root node split
    if (childChange.node) {
        auto newRoot = std::make_shared<BeTreeInternalNode<KeyType, ValueType>>(fanout, rootNode->level + 1, maxBufferSize);
        newRoot->keys.push_back(childChange.key);
        newRoot->children.push_back(rootNode);
        newRoot->children.push_back(childChange.node);
        rootNode->parent = newRoot;
        childChange.node->parent = newRoot;
        rootNode = newRoot;
    }

    // Handle root node merge
    if (rootNode->size() == 0) {
        if (rootNode->isLeaf()) {
            rootNode = nullptr;
        } else {
            InternalNodePtr oldRoot = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(rootNode);
            if (oldRoot->children[0]->isLeaf()) {
                // auto newRoot = std::static_pointer_cast<BeTreeLeafNode<KeyType, ValueType>>(oldRoot->children[0]);
                // newRoot->parent = nullptr;
                //
                // // TODO: handle message buffer of current rootNode to the newRoot because the old root is being deleted
                // // because the leaf node doesn't have a message buffer
                // rootNode = newRoot;
            } else {
                auto newRoot = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(oldRoot->children[0]);
                newRoot->parent.reset();

                // add message buffer of current rootNode to the newRoot because the old root is being deleted
                // for now just move the messages and don't call insert/remove because handling split/merge would be more complicated
                for (auto& msg : oldRoot->messageBuffer) {
                    newRoot->messageBuffer.insert_or_assign(msg.first, std::move(msg.second));
                }

                newRoot->leftSibling.reset();
                newRoot->rightSibling.reset();
                rootNode = newRoot;
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
