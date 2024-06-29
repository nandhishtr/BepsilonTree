#pragma once

#include "BeTreeMessage.hpp"
#include "BeTreeNode.hpp"
#include "ErrorCodes.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iosfwd>
#include <iostream>
#include <utility>
#include <vector>

// Forward declarations
template <typename KeyType, typename ValueType> class BeTreeLeafNode;
template <typename KeyType, typename ValueType> class BeTreeNode;
template <typename KeyType, typename ValueType> class BeTreeInternalNode;
template <typename KeyType, typename ValueType> struct Message;
enum class ErrorCode;
enum class MessageType;

template <typename KeyType, typename ValueType>
class BeTreeLeafNode : public BeTreeNode<KeyType, ValueType> {
public:
    using typename BeTreeNode<KeyType, ValueType>::MessagePtr;
    using typename BeTreeNode<KeyType, ValueType>::BeTreeNodePtr;
    using typename BeTreeNode<KeyType, ValueType>::InternalNodePtr;
    using typename BeTreeNode<KeyType, ValueType>::LeafNodePtr;
    using typename BeTreeNode<KeyType, ValueType>::ChildChange;
    using typename BeTreeNode<KeyType, ValueType>::ChildChangeType;

    std::vector<ValueType> values;

    BeTreeLeafNode(uint16_t fanout, InternalNodePtr parent = nullptr)
        : BeTreeNode<KeyType, ValueType>(fanout, 0, parent) {}

    KeyType getLowestSearchKey() const override {
        return this->keys[0];
    }

    ErrorCode applyMessage(MessagePtr message, uint16_t indexInParent, ChildChange& childChange) override;
    ErrorCode insert(MessagePtr message, ChildChange& newChild) override;
    ErrorCode remove(MessagePtr message, uint16_t indexInParent, ChildChange& oldChild) override;
    std::pair<ValueType, ErrorCode> search(MessagePtr message) override;

    ErrorCode split(ChildChange& newChild);
    ErrorCode handleUnderflow(uint16_t indexInParent, ChildChange& oldChild);
    ErrorCode redistribute(uint16_t indexInParent, bool withLeft, ChildChange& oldChild);
    ErrorCode merge(ChildChange& oldChild, bool withLeft);

    void printNode(std::ostream& out) const override;
};

// Implementations

template <typename KeyType, typename ValueType>
ErrorCode BeTreeLeafNode<KeyType, ValueType>::applyMessage(MessagePtr message, uint16_t indexInParent, ChildChange& childChange) {
    switch (message->type) {
        case MessageType::Insert:
            return insert(std::move(message), childChange);
        case MessageType::Remove:
            return remove(std::move(message), indexInParent, childChange);
        default:
            assert(false && "Invalid message type for leaf node");
    }
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeLeafNode<KeyType, ValueType>::insert(MessagePtr message, ChildChange& newChild) {
    assert(message->type == MessageType::Insert);
    auto it = this->getIndex(message->key);
    auto idx = std::distance(this->keys.begin(), it);

    // Check if the key already exists
    if (it != this->keys.end() && *it == message->key) {
        // Key exists, update the value
        this->values[idx] = message->value;
    } else {
        // Key does not exist, insert the key-value pair
        this->keys.insert(it, message->key);
        this->values.insert(this->values.begin() + idx, message->value);
    }

    newChild = { KeyType(), nullptr, ChildChangeType::None };

    if (this->isOverflowing()) {
        return this->split(newChild);
    } else {
        return ErrorCode::Success;
    }
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeLeafNode<KeyType, ValueType>::remove(MessagePtr message, uint16_t indexInParent, ChildChange& oldChild) {
    assert(message->type == MessageType::Remove);
    auto it = this->getIndex(message->key);
    auto idx = it - this->keys.begin();
    if (it == this->keys.end() || *it != message->key) {
        // NOTE: this happens when we have an insert message followed by a remove message for the same key
        //       without the insert message being applied yet
        return ErrorCode::Success;
    }

    this->keys.erase(it);
    this->values.erase(this->values.begin() + idx);

    if (this->isUnderflowing()) {
        return handleUnderflow(indexInParent, oldChild);
    } else {
        return ErrorCode::Success;
    }
}

template <typename KeyType, typename ValueType>
std::pair<ValueType, ErrorCode> BeTreeLeafNode<KeyType, ValueType>::search(MessagePtr message) {
    assert(message->type == MessageType::Search);
    auto it = this->getIndex(message->key);
    auto idx = it - this->keys.begin();
    if (it != this->keys.end() && *it == message->key) {
        return { this->values[idx], ErrorCode::Success };
    }
    return { ValueType(), ErrorCode::KeyDoesNotExist };
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeLeafNode<KeyType, ValueType>::split(ChildChange& newChild) {
    // Split the node
    auto newLeaf = std::make_shared<BeTreeLeafNode<KeyType, ValueType>>(this->fanout, this->parent);
    auto mid = this->keys.size() / 2;
    KeyType newPivot = this->keys[mid];
    newLeaf->keys.insert(newLeaf->keys.begin(), this->keys.begin() + mid, this->keys.end());
    newLeaf->values.insert(newLeaf->values.begin(), this->values.begin() + mid, this->values.end());
    this->keys.erase(this->keys.begin() + mid, this->keys.end());
    this->values.erase(this->values.begin() + mid, this->values.end());

    // Set sibling pointers
    newLeaf->leftSibling = this->shared_from_this();
    newLeaf->rightSibling = this->rightSibling;
    if (this->rightSibling) {
        this->rightSibling->leftSibling = newLeaf;
    }
    this->rightSibling = newLeaf;

    // Update parent
    newChild = { newPivot, newLeaf, ChildChangeType::Split };
    return ErrorCode::FinishedMessagePassing;
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeLeafNode<KeyType, ValueType>::handleUnderflow(uint16_t indexInParent, ChildChange& oldChild) {
    if (this->isRoot()) {
        return ErrorCode::Success;
    }

    // redistribute or merge with sibling
    if (this->leftSibling && this->leftSibling->isBorrowable() && this->leftSibling->parent == this->parent) {
        return redistribute(indexInParent, true, oldChild);
    } else if (this->leftSibling && this->leftSibling->parent == this->parent) {
        return merge(oldChild, true);
    } else if (this->rightSibling && this->rightSibling->isBorrowable() && this->rightSibling->parent == this->parent) {
        return redistribute(indexInParent, false, oldChild);
    } else if (this->rightSibling && this->rightSibling->parent == this->parent) {
        return merge(oldChild, false);
    } else {
        return ErrorCode::Error;
    }
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeLeafNode<KeyType, ValueType>::redistribute(uint16_t indexInParent, bool withLeft, ChildChange& oldChild) {
    auto sibling = withLeft ?
        std::static_pointer_cast<BeTreeLeafNode<KeyType, ValueType>>(this->leftSibling) :
        std::static_pointer_cast<BeTreeLeafNode<KeyType, ValueType>>(this->rightSibling);

    auto sizeDiff = (sibling->size() - this->size()) / 2;
    assert(sizeDiff >= 0);

    // TODO: use childChange
    if (withLeft) {
        this->keys.insert(this->keys.begin(), sibling->keys.end() - sizeDiff, sibling->keys.end());
        this->values.insert(this->values.begin(), sibling->values.end() - sizeDiff, sibling->values.end());
        sibling->keys.erase(sibling->keys.end() - sizeDiff, sibling->keys.end());
        sibling->values.erase(sibling->values.end() - sizeDiff, sibling->values.end());
        // Update pivot key in parent
        this->parent->keys[indexInParent - 1] = this->getLowestSearchKey(); // TODO: handle with ChildChange
        oldChild = { this->getLowestSearchKey(), nullptr, ChildChangeType::RedistributeLeft };
    } else {
        this->keys.insert(this->keys.end(), sibling->keys.begin(), sibling->keys.begin() + sizeDiff);
        this->values.insert(this->values.end(), sibling->values.begin(), sibling->values.begin() + sizeDiff);
        sibling->keys.erase(sibling->keys.begin(), sibling->keys.begin() + sizeDiff);
        sibling->values.erase(sibling->values.begin(), sibling->values.begin() + sizeDiff);
        // Update pivot key in parent
        this->parent->keys[indexInParent] = sibling->getLowestSearchKey(); // TODO: handle with ChildChange
        oldChild = { sibling->getLowestSearchKey(), nullptr, ChildChangeType::RedistributeRight };
    }

    return ErrorCode::FinishedMessagePassing;
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeLeafNode<KeyType, ValueType>::merge(ChildChange& oldChild, bool withLeft) {
    auto sibling = withLeft ?
        std::static_pointer_cast<BeTreeLeafNode<KeyType, ValueType>>(this->leftSibling) :
        std::static_pointer_cast<BeTreeLeafNode<KeyType, ValueType>>(this->rightSibling);

    if (withLeft) {
        sibling->keys.insert(sibling->keys.end(), this->keys.begin(), this->keys.end());
        sibling->values.insert(sibling->values.end(), this->values.begin(), this->values.end());
        sibling->rightSibling = this->rightSibling;
        if (this->rightSibling) {
            this->rightSibling->leftSibling = sibling;
        }
        oldChild = { KeyType(), this->shared_from_this(), ChildChangeType::MergeLeft };
    } else {
        this->keys.insert(this->keys.end(), sibling->keys.begin(), sibling->keys.end());
        this->values.insert(this->values.end(), sibling->values.begin(), sibling->values.end());
        this->rightSibling = sibling->rightSibling;
        if (sibling->rightSibling) {
            sibling->rightSibling->leftSibling = this->shared_from_this();
        }
        oldChild = { KeyType(), sibling, ChildChangeType::MergeRight };
    }

    return ErrorCode::FinishedMessagePassing;
}

template <typename KeyType, typename ValueType>
void BeTreeLeafNode<KeyType, ValueType>::printNode(std::ostream& out) const {
    out << "Leaf: ";
    for (size_t i = 0; i < this->size(); ++i) {
        out << this->keys[i] << " ";
    }
    out << "\n";
}
