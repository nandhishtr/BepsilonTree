#pragma once

// Forward declarations
template <typename KeyType, typename ValueType> class BeTree;
template <typename KeyType, typename ValueType> class BeTreeNode;
template <typename KeyType, typename ValueType> class BeTreeInternalNode;
template <typename KeyType, typename ValueType> class BeTreeLeafNode;
template <typename KeyType, typename ValueType> struct Message;
enum class ErrorCode;
enum class MessageType;

#include "BeTreeInternalNode.hpp"
#include "BeTreeLeafNode.hpp"
#include "BeTreeMessage.hpp"
#include "BeTreeNode.hpp"
#include "ErrorCodes.h"
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <utility>

template <typename KeyType, typename ValueType>
class BeTree {
private:
    using MessagePtr = std::unique_ptr<Message<KeyType, ValueType>>;
    using BeTreeNodePtr = std::shared_ptr<BeTreeNode<KeyType, ValueType>>;
    using InternalNodePtr = std::shared_ptr<BeTreeInternalNode<KeyType, ValueType>>;
    using LeafNodePtr = std::shared_ptr<BeTreeLeafNode<KeyType, ValueType>>;
    using childChange = typename BeTreeNode<KeyType, ValueType>::ChildChange;

    uint32_t fanout;
    BeTreeNodePtr rootNode;

    ErrorCode applyMessage(MessagePtr message) {
        childChange childChange = { KeyType(), nullptr, false };
        ErrorCode err;
        if (message->type == MessageType::Insert) {
            err = rootNode->insert(std::move(message), childChange);
        } else if (message->type == MessageType::Remove) {
            err = rootNode->remove(std::move(message), 0, childChange);
        }

        // Because of the buffering both insert and remove messages are handled in the same way
        // meaning that the tree can split and merge on both insert and remove operations

        // Handle root node split
        if (childChange.node) {
            auto newRoot = std::make_shared<BeTreeInternalNode<KeyType, ValueType>>(fanout, rootNode->level + 1);
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
                rootNode = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(rootNode)->children[0];
            }
        }

        return err;
    }
public:
    BeTree(uint32_t fanout) : fanout(fanout), rootNode(nullptr) {}

    ErrorCode insert(const KeyType& key, const ValueType& value) {
        // If the tree is empty, create a new leaf node
        if (!rootNode) {
            rootNode = std::make_shared<BeTreeLeafNode<KeyType, ValueType>>(fanout);
        }

        MessagePtr message = std::make_unique<Message<KeyType, ValueType>>(MessageType::Insert, key, value);

        return applyMessage(std::move(message));
    }

    ErrorCode remove(const KeyType& key) {
        if (!rootNode) {
            return ErrorCode::KeyDoesNotExist;
        }
        MessagePtr message = std::make_unique<Message<KeyType, ValueType>>(MessageType::Remove, key);

        return applyMessage(std::move(message));
    }

    std::pair<ValueType, ErrorCode> search(const KeyType& key) {
        if (!rootNode) {
            return { ValueType(), ErrorCode::KeyDoesNotExist };
        }
        MessagePtr message = std::make_unique<Message<KeyType, ValueType>>(MessageType::Search, key);
        return rootNode->search(std::move(message));
    }

    void printTree(std::ostream& out) {
        if (!rootNode) {
            out << "Empty tree\n";
            return;
        }
        rootNode->printNode(out);
    }

};

