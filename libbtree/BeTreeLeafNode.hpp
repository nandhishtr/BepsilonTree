#pragma once

#include "BeTreeInternalNode.hpp"
#include "BeTreeMessage.hpp"
#include "BeTreeNode.hpp"
#include "ErrorCodes.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <ios>
#include <iosfwd>
#include <string>
#include <type_traits>
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
        : BeTreeNode<KeyType, ValueType>(fanout, parent) {}

    ~BeTreeLeafNode() {
        std::vector<KeyType>().swap(this->keys);
        std::vector<ValueType>().swap(this->values);
    }

    bool isLeaf() const override {
        return true;
    }

    ErrorCode applyMessage(MessagePtr message, uint16_t indexInParent, ChildChange& childChange) override;
    ErrorCode insert(MessagePtr message, ChildChange& newChild) override;
    ErrorCode remove(MessagePtr message, uint16_t indexInParent, ChildChange& oldChild) override;
    std::pair<ValueType, ErrorCode> search(MessagePtr message) override;

    ErrorCode split(ChildChange& newChild);
    ErrorCode handleUnderflow(uint16_t indexInParent, ChildChange& oldChild);
    ErrorCode redistribute(uint16_t indexInParent, bool withLeft, ChildChange& oldChild);
    ErrorCode merge(ChildChange& oldChild, bool withLeft);

    // When serializing we for now write the maximum amount of keys and values we can have
    // This is not optimal, but it is a start
    // The Header is in the order of:
    // 1. type (leaf = 1)
    // 2. numKeys
    // 3. parent
    // 4. leftSibling
    // 5. rightSibling
    // 6. lowestSearchKey
    // Then we write the keys in the order where first are all the used keys, then the space of the unused keys.
    // Then we write the values in the same way.
    size_t getSerializedSize() const {
        return ((this->fanout - 1) * sizeof(KeyType))
            + ((this->fanout - 1) * sizeof(ValueType))
            + 1 * sizeof(uint8_t) // type (leaf = 1)
            + 1 * sizeof(uint16_t) // numKeys
            + 3 * sizeof(size_t) // parent, leftSibling, rightSibling
            + 1 * sizeof(KeyType); // lowestSearchKey
    }
    size_t serialize(char*& buf) const; // returns the number of bytes written
    void serialize(std::ostream& os) const;
    size_t deserialize(char*& buf, size_t bufferSize); // returns the number of bytes read
    void deserialize(std::istream& is);

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

        // Update lowest search key
        if (this->keys.size() == 1 || message->key < this->lowestSearchKey) {
            this->lowestSearchKey = message->key;
        }
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

    // Update lowest search key
    if (this->keys.size() > 0) {
        this->lowestSearchKey = this->keys[0];
    }

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
    auto newLeaf = std::make_shared<BeTreeLeafNode<KeyType, ValueType>>(this->fanout, this->parent.lock());
    auto mid = this->keys.size() / 2;
    KeyType newPivot = this->keys[mid];
    newLeaf->keys.insert(newLeaf->keys.begin(), this->keys.begin() + mid, this->keys.end());
    newLeaf->values.insert(newLeaf->values.begin(), this->values.begin() + mid, this->values.end());
    this->keys.erase(this->keys.begin() + mid, this->keys.end());
    this->values.erase(this->values.begin() + mid, this->values.end());

    newLeaf->lowestSearchKey = newLeaf->keys[0];

    // Set sibling pointers
    newLeaf->leftSibling = this->shared_from_this();
    newLeaf->rightSibling = this->rightSibling.lock();
    if (this->rightSibling.lock()) {
        this->rightSibling.lock()->leftSibling = newLeaf;
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
    if (this->leftSibling.lock() && this->leftSibling.lock()->isBorrowable() && this->leftSibling.lock()->parent.lock() == this->parent.lock()) {
        return redistribute(indexInParent, true, oldChild);
    } else if (this->leftSibling.lock() && this->leftSibling.lock()->parent.lock() == this->parent.lock()) {
        return merge(oldChild, true);
    } else if (this->rightSibling.lock() && this->rightSibling.lock()->isBorrowable() && this->rightSibling.lock()->parent.lock() == this->parent.lock()) {
        return redistribute(indexInParent, false, oldChild);
    } else if (this->rightSibling.lock() && this->rightSibling.lock()->parent.lock() == this->parent.lock()) {
        return merge(oldChild, false);
    } else {
        return ErrorCode::Error;
    }
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeLeafNode<KeyType, ValueType>::redistribute(uint16_t indexInParent, bool withLeft, ChildChange& oldChild) {
    auto sibling = withLeft ?
        std::static_pointer_cast<BeTreeLeafNode<KeyType, ValueType>>(this->leftSibling.lock()) :
        std::static_pointer_cast<BeTreeLeafNode<KeyType, ValueType>>(this->rightSibling.lock());

    auto sizeDiff = (sibling->size() - this->size()) / 2;
    assert(sizeDiff >= 0);

    // TODO: use childChange
    if (withLeft) {
        this->keys.insert(this->keys.begin(), sibling->keys.end() - sizeDiff, sibling->keys.end());
        this->values.insert(this->values.begin(), sibling->values.end() - sizeDiff, sibling->values.end());
        sibling->keys.erase(sibling->keys.end() - sizeDiff, sibling->keys.end());
        sibling->values.erase(sibling->values.end() - sizeDiff, sibling->values.end());
        // Update pivot key in parent
        this->lowestSearchKey = this->keys[0];
        this->parent.lock()->keys[indexInParent - 1] = this->lowestSearchKey; // TODO: handle with ChildChange
        oldChild = { this->lowestSearchKey, nullptr, ChildChangeType::RedistributeLeft };
    } else {
        this->keys.insert(this->keys.end(), sibling->keys.begin(), sibling->keys.begin() + sizeDiff);
        this->values.insert(this->values.end(), sibling->values.begin(), sibling->values.begin() + sizeDiff);
        sibling->keys.erase(sibling->keys.begin(), sibling->keys.begin() + sizeDiff);
        sibling->values.erase(sibling->values.begin(), sibling->values.begin() + sizeDiff);
        // Update pivot key in parent
        sibling->lowestSearchKey = sibling->keys[0];
        this->parent.lock()->keys[indexInParent] = sibling->lowestSearchKey; // TODO: handle with ChildChange
        oldChild = { sibling->lowestSearchKey, nullptr, ChildChangeType::RedistributeRight };
    }

    return ErrorCode::FinishedMessagePassing;
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeLeafNode<KeyType, ValueType>::merge(ChildChange& oldChild, bool withLeft) {
    auto sibling = withLeft ?
        std::static_pointer_cast<BeTreeLeafNode<KeyType, ValueType>>(this->leftSibling.lock()) :
        std::static_pointer_cast<BeTreeLeafNode<KeyType, ValueType>>(this->rightSibling.lock());

    if (withLeft) {
        sibling->keys.insert(sibling->keys.end(), this->keys.begin(), this->keys.end());
        sibling->values.insert(sibling->values.end(), this->values.begin(), this->values.end());
        sibling->rightSibling = this->rightSibling.lock();
        if (this->rightSibling.lock()) {
            this->rightSibling.lock()->leftSibling = sibling;
        }
        oldChild = { KeyType(), this->shared_from_this(), ChildChangeType::MergeLeft };
    } else {
        this->keys.insert(this->keys.end(), sibling->keys.begin(), sibling->keys.end());
        this->values.insert(this->values.end(), sibling->values.begin(), sibling->values.end());
        this->rightSibling = sibling->rightSibling.lock();
        if (sibling->rightSibling.lock()) {
            sibling->rightSibling.lock()->leftSibling = this->shared_from_this();
        }
        oldChild = { KeyType(), sibling, ChildChangeType::MergeRight };
    }

    return ErrorCode::FinishedMessagePassing;
}

template <typename KeyType, typename ValueType>
size_t BeTreeLeafNode<KeyType, ValueType>::serialize(char*& buf) const {
    std::stringstream memoryStream{};
    this->serialize(memoryStream);

    std::string data = memoryStream.str();
    size_t bufferSize = data.size();
    buf = new char[bufferSize];
    memcpy(buf, data.c_str(), bufferSize);

    return bufferSize;
}

template <typename KeyType, typename ValueType>
void BeTreeLeafNode<KeyType, ValueType>::serialize(std::ostream& os) const {
    static_assert(
        std::is_trivial<KeyType>::value &&
        std::is_standard_layout<KeyType>::value &&
        std::is_trivial<ValueType>::value &&
        std::is_standard_layout<ValueType>::value,
        "Can only deserialize POD types with this function");

    assert(this->keys.size() == this->values.size() && "Keys and values must have the same size");
    uint16_t numKeys = this->keys.size(); // values = keys
    auto start = os.tellp();

    os.put(1); // type (leaf = 1)
    os.write(reinterpret_cast<const char*>(&numKeys), sizeof(uint16_t));
    os.write(reinterpret_cast<const char*>(&this->parent), sizeof(uint64_t));
    os.write(reinterpret_cast<const char*>(&this->leftSibling), sizeof(uint64_t));
    os.write(reinterpret_cast<const char*>(&this->rightSibling), sizeof(uint64_t));
    os.write(reinterpret_cast<const char*>(&this->lowestSearchKey), sizeof(KeyType));

    os.write(reinterpret_cast<const char*>(this->keys.data()), numKeys * sizeof(KeyType));
    size_t remainingBytes = ((this->fanout - 1) - numKeys) * sizeof(KeyType);
    for (size_t i = 0; i < remainingBytes; ++i) {
        os.put(0);
    }

    os.write(reinterpret_cast<const char*>(this->values.data()), numKeys * sizeof(ValueType));
    remainingBytes = ((this->fanout - 1) - numKeys) * sizeof(ValueType);
    for (size_t i = 0; i < remainingBytes; ++i) {
        os.put(0);
    }

    auto end = os.tellp();
    assert(this->getSerializedSize() == (end - start) && "Data size mismatch");
}

template <typename KeyType, typename ValueType>
size_t BeTreeLeafNode<KeyType, ValueType>::deserialize(char*& buf, size_t bufferSize) {
    std::stringstream memoryStream{};
    memoryStream.write(buf, bufferSize);
    this->deserialize(memoryStream);
    return memoryStream.tellg();
}

template <typename KeyType, typename ValueType>
void BeTreeLeafNode<KeyType, ValueType>::deserialize(std::istream& is) {
    static_assert(
        std::is_trivial<KeyType>::value &&
        std::is_standard_layout<KeyType>::value &&
        std::is_trivial<ValueType>::value &&
        std::is_standard_layout<ValueType>::value,
        "Can only deserialize POD types with this function");

    uint8_t type = 0;
    uint16_t numKeys = 0;
    is.read(reinterpret_cast<char*>(&type), sizeof(uint8_t));
    is.read(reinterpret_cast<char*>(&numKeys), sizeof(uint16_t));
    is.read(reinterpret_cast<char*>(&parent), sizeof(uint64_t));
    is.read(reinterpret_cast<char*>(&leftSibling), sizeof(uint64_t));
    is.read(reinterpret_cast<char*>(&rightSibling), sizeof(uint64_t));
    is.read(reinterpret_cast<char*>(&lowestSearchKey), sizeof(KeyType));

    this->keys.resize(numKeys);
    is.read(reinterpret_cast<char*>(this->keys.data()), numKeys * sizeof(KeyType));
    size_t remainingBytes = ((this->fanout - 1) - numKeys) * sizeof(KeyType);
    is.seekg(remainingBytes, std::ios_base::cur);

    this->values.resize(numKeys);
    is.read(reinterpret_cast<char*>(this->values.data()), numKeys * sizeof(ValueType));
    remainingBytes = ((this->fanout - 1) - numKeys) * sizeof(ValueType);
    is.seekg(remainingBytes, std::ios_base::cur);

    assert(this->keys.size() == this->values.size() && "Keys and values must have the same size");
    assert(this->getSerializedSize() == is.tellg() && "Data size mismatch");
}


template <typename KeyType, typename ValueType>
void BeTreeLeafNode<KeyType, ValueType>::printNode(std::ostream& out) const {
    out << "Leaf: ";
    for (size_t i = 0; i < this->size(); ++i) {
        out << this->keys[i] << " ";
    }
    out << "\n";
}
