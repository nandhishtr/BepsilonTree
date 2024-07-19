#pragma once

#include "pch.h"
#include "BeTreeIStorage.hpp"
#include "BeTreeMessage.hpp"
#include "BeTreeNode.hpp"
#include "ErrorCodes.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <ios>
#include <iosfwd>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// Forward declaration
template <typename KeyType, typename ValueType> class BeTreeNode;
template <typename KeyType, typename ValueType> class BeTreeLeafNode;

template <typename KeyType, typename ValueType>
class BeTreeInternalNode : public BeTreeNode<KeyType, ValueType> {
public:
    using typename BeTreeNode<KeyType, ValueType>::MessagePtr;
    using typename BeTreeNode<KeyType, ValueType>::BeTreeNodePtr;
    using typename BeTreeNode<KeyType, ValueType>::ChildChange;
    using typename BeTreeNode<KeyType, ValueType>::ChildChangeType;
    using typename BeTreeNode<KeyType, ValueType>::CachePtr;
    using InternalNodePtr = std::shared_ptr<BeTreeInternalNode<KeyType, ValueType>>;
    using LeafNodePtr = std::shared_ptr<BeTreeLeafNode<KeyType, ValueType>>;

    std::map<KeyType, MessagePtr> messageBuffer;
    uint16_t maxBufferSize;

    std::vector<uint64_t> children;

    BeTreeInternalNode(uint16_t fanout, CachePtr cache, uint16_t maxBufferSize = 0, uint64_t parent = 0)
        : BeTreeNode<KeyType, ValueType>(fanout, cache, parent) {
        if (maxBufferSize == 0) {
            this->maxBufferSize = fanout - 1;
        } else {
            this->maxBufferSize = maxBufferSize;
        }
    }

    ~BeTreeInternalNode() {
        std::vector<KeyType>().swap(this->keys);
        std::map<KeyType, MessagePtr>().swap(this->messageBuffer);
    }

    bool isLeaf() const override {
        return false;
    }
    bool isFlushable() const {
        return this->messageBuffer.size() >= maxBufferSize;
    }

    ErrorCode applyMessage(MessagePtr message, uint16_t indexInParent, ChildChange& childChange) override;
    ErrorCode insert(MessagePtr message, ChildChange& newChild) override;
    ErrorCode remove(MessagePtr message, uint16_t indexInParent, ChildChange& oldChild) override;
    std::pair<ValueType, ErrorCode> search(MessagePtr message) override;

    ErrorCode flushBuffer(ChildChange& childChange);

    ErrorCode split(ChildChange& newChild);
    ErrorCode handleUnderflow(uint16_t indexInParent, ChildChange& oldChild);
    ErrorCode redistribute(uint16_t indexInParent, InternalNodePtr sibling, ChildChange& oldChild);
    ErrorCode merge(uint16_t indexInParent, ChildChange& oldChild, InternalNodePtr sibling);

    // When serializing we for now write the maximum amount of keys and values we can have
    // This is not optimal, but it is a start
    // The Header is in the order of:
    // 1. type (internal = 0)
    // 2. numKeys
    // 3. numMessages
    // 4. parent
    // 5. leftSibling
    // 6. rightSibling
    // 7. lowestSearchKey
    // Then we write the keys in the order where first are all the used keys, then the space of the unused keys.
    // Then we write the children in the same way.
    // Then we write the messages in the same way.
    size_t getSerializedSize() const {
        return ((this->fanout - 1) * sizeof(KeyType))
            + (this->fanout * sizeof(uint64_t))
            + (this->maxBufferSize * Message<KeyType, ValueType>::getSerializedSize())
            + 1 * sizeof(uint8_t) // type
            + 2 * sizeof(uint16_t) // numKeys, numMessages
            + 3 * sizeof(uint64_t) // parent, leftSibling, rightSibling
            + sizeof(KeyType); // lowestSearchKey
    }
    size_t serialize(char*& buf) const; // returns the number of bytes written
    void serialize(std::ostream& os) const;
    size_t deserialize(char*& buf, size_t bufferSize); // returns the number of bytes read
    void deserialize(std::istream& is);

    void printNode(std::ostream& out) const override;
};

// Implementations

template <typename KeyType, typename ValueType>
ErrorCode BeTreeInternalNode<KeyType, ValueType>::applyMessage(MessagePtr message, uint16_t indexInParent, ChildChange& childChange) {
    this->lowestSearchKey = std::min(this->lowestSearchKey, message->key);

    this->messageBuffer.insert_or_assign(message->key, std::move(message));

    if (isFlushable()) {
        return flushBuffer(childChange);
    } else {
        return ErrorCode::Success;
    }
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeInternalNode<KeyType, ValueType>::insert(MessagePtr message, ChildChange& newChild) {
    return this->applyMessage(std::move(message), 0, newChild);
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeInternalNode<KeyType, ValueType>::remove(MessagePtr message, uint16_t indexInParent, ChildChange& oldChild) {
    return this->applyMessage(std::move(message), indexInParent, oldChild);
}

template <typename KeyType, typename ValueType>
std::pair<ValueType, ErrorCode> BeTreeInternalNode<KeyType, ValueType>::search(MessagePtr message) {
    // Search for the message in the buffer
    auto it = this->messageBuffer.find(message->key);
    if (it != this->messageBuffer.end()) {
        if (it->second->type != MessageType::Remove) {
            return { it->second->value, ErrorCode::Success };
        } else {
            return { ValueType(), ErrorCode::KeyDoesNotExist };
        }
    }

    // If not found in the buffer, search in the child node
    auto idx = std::upper_bound(this->keys.begin(), this->keys.end(), message->key) - this->keys.begin();
    BeTreeNodePtr child = this->cache->get(this->children[idx]);
    return child->search(std::move(message));
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeInternalNode<KeyType, ValueType>::flushBuffer(ChildChange& childChange) {
    assert(messageBuffer.size() > 0);
    // Find the child for that we have the most messages for
    auto maxChild = children[0];
    auto maxStart = this->messageBuffer.begin();
    auto maxEnd = this->messageBuffer.begin();
    auto currentStart = this->messageBuffer.begin();
    auto currentEnd = this->messageBuffer.begin();
    uint16_t maxCount = 0;
    uint16_t count = 0;
    uint16_t currentIdx = 0;
    for (auto it = this->messageBuffer.begin(); it != this->messageBuffer.end(); ++it) {
        // Move to the next child if necessary
        while (currentIdx < this->keys.size() && it->first >= this->keys[currentIdx]) {
            if (count > maxCount) {
                maxCount = count;
                maxChild = this->children[currentIdx];
                maxStart = currentStart;
                maxEnd = currentEnd;
            }
            count = 0;
            currentStart = currentEnd;
            currentIdx++;
        }
        currentEnd++;
        count++;
    }
    // Check if the last child has the most messages
    if (count > maxCount) {
        maxCount = count;
        maxChild = this->children[currentIdx];
        maxStart = currentStart;
        maxEnd = currentEnd;
    }

    // Flush the messages to the child node
    // TODO: use maxchild
    uint16_t idx = std::upper_bound(this->keys.begin(), this->keys.end(), maxStart->first) - this->keys.begin();
    ChildChange newChild = { KeyType(), 0 , ChildChangeType::None };
    BeTreeNodePtr child = this->cache->get(maxChild);

    // NOTE: For now we will stop flushing messages if the child node splits or merges or if we arrived at a leaf node
    //       This is to avoid the complexity of handling the index changes in the buffer
    // TODO: Right now this does not really work as intended, because when the child of a child splits or merges
    //       we may call that child again. Right now this isn't a problem, but when we are using files for storage
    //       this will be a latency issue.
    auto it = maxStart;
    ErrorCode err = ErrorCode::Success;
    for (it = maxStart; it != maxEnd; ++it) {
        err = child->applyMessage(std::move(it->second), idx, newChild);
        switch (newChild.type) {
            case ChildChangeType::Split:
                // Insert the new child
                this->keys.insert(this->keys.begin() + idx, newChild.key);
                this->children.insert(this->children.begin() + idx + 1, newChild.node);
                break;
            case ChildChangeType::RedistributeLeft:
                // Change pivot key in parent
                this->keys[idx - 1] = newChild.key;
                break;
            case ChildChangeType::RedistributeRight:
                this->keys[idx] = newChild.key;
                break;
            case ChildChangeType::MergeLeft:
                // Remove the child
                this->keys.erase(this->keys.begin() + idx - 1);
                this->children.erase(this->children.begin() + idx);
                break;
            case ChildChangeType::MergeRight:
                this->keys.erase(this->keys.begin() + idx);
                this->children.erase(this->children.begin() + idx + 1);
                break;
            default:
                break;
        }

        if (err != ErrorCode::Success) {
            break;
        }
    }
    // Remove the flushed messages from the buffer
    if (it != maxEnd) {
        it++;
    }
    messageBuffer.erase(maxStart, it);

    // Handle overflow and underflow of our node
    if (this->isOverflowing()) {
        if (this->split(childChange) == ErrorCode::Error) {
            return ErrorCode::Error;
        } else {
            return err;
        }
    } else if (this->isUnderflowing() && !this->isRoot()) {
        // OPTIMIZE: Pass the index of the parent down to avoid searching for it again
        InternalNodePtr parentPtr = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(this->cache->get(this->parent));
        uint16_t indexInParent = std::upper_bound(parentPtr->keys.begin(), parentPtr->keys.end(), this->lowestSearchKey) - parentPtr->keys.begin();
        // TODO: After a merge the message buffer might be overflown
        if (handleUnderflow(indexInParent, childChange) == ErrorCode::Error) {
            return ErrorCode::Error;
        } else {
            return err;
        }
    }

    return err;
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeInternalNode<KeyType, ValueType>::split(ChildChange& newChild) {
    // Split the node
    auto newInternal = std::make_shared<BeTreeInternalNode<KeyType, ValueType>>(this->fanout, this->cache, this->maxBufferSize, this->parent);
    auto mid = this->keys.size() / 2;
    KeyType newPivot = this->keys[mid];
    newInternal->keys.insert(newInternal->keys.begin(), this->keys.begin() + mid + 1, this->keys.end());
    newInternal->children.insert(newInternal->children.begin(), this->children.begin() + mid + 1, this->children.end());
    this->keys.erase(this->keys.begin() + mid, this->keys.end());
    this->children.erase(this->children.begin() + mid + 1, this->children.end());

    // Distribute the buffer messages to the new node
    // TODO: Use binary search to find the split point in the buffer instead of iterating through it
    for (auto it = this->messageBuffer.begin(); it != this->messageBuffer.end();) {
        if (it->first >= newPivot) {
            newInternal->messageBuffer.insert_or_assign(it->first, std::move(it->second));
            it = this->messageBuffer.erase(it);
        } else {
            ++it;
        }
    }

    BeTreeNodePtr firstChild = this->cache->get(newInternal->children[0]);
    if (newInternal->messageBuffer.size() > 0) {
        newInternal->lowestSearchKey = std::min(firstChild->lowestSearchKey, newInternal->messageBuffer.begin()->first);
    } else {
        newInternal->lowestSearchKey = firstChild->lowestSearchKey;
    }

    this->cache->create(newInternal);

    // Set sibling pointers
    newInternal->leftSibling = this->id;
    newInternal->rightSibling = this->rightSibling;
    if (this->rightSibling) {
        InternalNodePtr rightSiblingPtr = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(this->cache->get(this->rightSibling));
        rightSiblingPtr->leftSibling = newInternal->id;
    }
    this->rightSibling = newInternal->id;

    // Set parent pointers
    for (auto& child : newInternal->children) {
        BeTreeNodePtr childPtr = this->cache->get(child);
        childPtr->parent = newInternal->id;
    }

    // Update parent
    newChild = { newPivot, newInternal->id, ChildChangeType::Split };
    return ErrorCode::Success;
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeInternalNode<KeyType, ValueType>::handleUnderflow(uint16_t indexInParent, ChildChange& oldChild) {
    if (this->isRoot()) {
        return ErrorCode::Success;
    }
    // redistribute or merge with sibling
    // if (this->leftSibling.lock() && this->leftSibling.lock()->isBorrowable() && this->leftSibling.lock()->parent.lock() == this->parent.lock()) {
    //     return redistribute(indexInParent, true, oldChild);
    // } else if (this->leftSibling.lock() && this->leftSibling.lock()->parent.lock() == this->parent.lock()) {
    //     return merge(indexInParent, oldChild, true);
    // } else if (this->rightSibling.lock() && this->rightSibling.lock()->isBorrowable() && this->rightSibling.lock()->parent.lock() == this->parent.lock()) {
    //     return redistribute(indexInParent, false, oldChild);
    // } else if (this->rightSibling.lock() && this->rightSibling.lock()->parent.lock() == this->parent.lock()) {
    //     return merge(indexInParent, oldChild, false);
    // } else {
    //     return ErrorCode::Success;
    // }
    if (this->leftSibling) {
        InternalNodePtr leftSiblingPtr = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(this->cache->get(this->leftSibling));
        if (leftSiblingPtr->isBorrowable() && leftSiblingPtr->parent == this->parent) {
            return redistribute(indexInParent, leftSiblingPtr, oldChild);
        } else if (leftSiblingPtr->parent == this->parent) {
            return merge(indexInParent, oldChild, leftSiblingPtr);
        }
    } else if (this->rightSibling) {
        InternalNodePtr rightSiblingPtr = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(this->cache->get(this->rightSibling));
        if (rightSiblingPtr->isBorrowable() && rightSiblingPtr->parent == this->parent) {
            return redistribute(indexInParent, rightSiblingPtr, oldChild);
        } else if (rightSiblingPtr->parent == this->parent) {
            return merge(indexInParent, oldChild, rightSiblingPtr);
        }
    }

}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeInternalNode<KeyType, ValueType>::redistribute(uint16_t indexInParent, InternalNodePtr sibling, ChildChange& oldChild) {
    auto sizeDiff = (sibling->size() - this->size()) / 2;
    assert(sizeDiff >= 0);

    if (this->leftSibling == sibling->id) {
        this->keys.insert(this->keys.begin(), this->lowestSearchKey);

        this->keys.insert(this->keys.begin(), sibling->keys.end() - sizeDiff + 1, sibling->keys.end());
        this->children.insert(this->children.begin(), sibling->children.end() - sizeDiff, sibling->children.end());
        sibling->keys.erase(sibling->keys.end() - sizeDiff, sibling->keys.end());
        sibling->children.erase(sibling->children.end() - sizeDiff, sibling->children.end());

        BeTreeNodePtr child = this->cache->get(this->children[0]);
        this->lowestSearchKey = child->lowestSearchKey;

        // Move messages from sibling to this node
        // TODO: Use binary search to find the split point in the buffer instead of iterating through it
        for (auto it = sibling->messageBuffer.begin(); it != sibling->messageBuffer.end();) {
            if (it->first >= this->lowestSearchKey) {
                this->messageBuffer.insert_or_assign(it->first, std::move(it->second));
                it = sibling->messageBuffer.erase(it);
            } else {
                ++it;
            }
        }

        // Update parent of redistributed children
        for (int i = 0; i < sizeDiff; ++i) {
            BeTreeNodePtr child = this->cache->get(this->children[i]);
            child->parent = this->id;
        }

        // Update pivot key in parent
        oldChild = { this->lowestSearchKey, 0, ChildChangeType::RedistributeLeft };
    } else {
        this->keys.insert(this->keys.end(), sibling->lowestSearchKey);
        this->keys.insert(this->keys.end(), sibling->keys.begin(), sibling->keys.begin() + sizeDiff - 1);
        this->children.insert(this->children.end(), sibling->children.begin(), sibling->children.begin() + sizeDiff);
        sibling->keys.erase(sibling->keys.begin(), sibling->keys.begin() + sizeDiff);
        sibling->children.erase(sibling->children.begin(), sibling->children.begin() + sizeDiff);

        BeTreeNodePtr child = this->cache->get(sibling->children[0]);
        sibling->lowestSearchKey = child->lowestSearchKey;

        // Move messages from sibling to this node
        // TODO: Use binary search to find the split point in the buffer instead of iterating through it
        for (auto it = sibling->messageBuffer.begin(); it != sibling->messageBuffer.end();) {
            if (it->first < sibling->lowestSearchKey) {
                this->messageBuffer.insert_or_assign(it->first, std::move(it->second));
                it = sibling->messageBuffer.erase(it);
            } else {
                break; // The buffer is sorted so we can break here
            }
        }

        // Update parent of redistributed children
        for (int i = 0; i < sizeDiff; ++i) {
            BeTreeNodePtr child = this->cache->get(this->children[this->children.size() - sizeDiff + i]);
            child->parent = this->id;
        }

        // Update pivot key in parent
        oldChild = { sibling->lowestSearchKey, 0, ChildChangeType::RedistributeRight };
    }

    return ErrorCode::Success;
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeInternalNode<KeyType, ValueType>::merge(uint16_t indexInParent, ChildChange& oldChild, InternalNodePtr sibling) {
    InternalNodePtr parentPtr = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(this->cache->get(this->parent));
    if (this->leftSibling == sibling->id) {
        // pull the pivot key from the parent
        sibling->keys.insert(sibling->keys.end(), parentPtr->keys[indexInParent - 1]);

        // merge the keys and children
        sibling->keys.insert(sibling->keys.end(), this->keys.begin(), this->keys.end());
        sibling->children.insert(sibling->children.end(), this->children.begin(), this->children.end());

        // combine the message buffers
        for (auto& [key, message] : this->messageBuffer) {
            sibling->messageBuffer.insert_or_assign(key, std::move(message));
        }

        // Update sibling pointers
        sibling->rightSibling = this->rightSibling;
        if (this->rightSibling) {
            InternalNodePtr rightSiblingPtr = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(this->cache->get(this->rightSibling));
            rightSiblingPtr->leftSibling = sibling->id;
        }

        // Update parent pointers
        for (auto& child : sibling->children) {
            InternalNodePtr childPtr = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(this->cache->get(child));
            childPtr->parent = sibling->id;
        }

        oldChild = { KeyType(), this->id, ChildChangeType::MergeLeft };
    } else {
        // pull the pivot key from the parent
        this->keys.insert(this->keys.end(), parentPtr->keys[indexInParent]);

        // merge the keys and children
        this->keys.insert(this->keys.end(), sibling->keys.begin(), sibling->keys.end());
        this->children.insert(this->children.end(), sibling->children.begin(), sibling->children.end());

        // combine the message buffers
        for (auto& [key, message] : sibling->messageBuffer) {
            this->messageBuffer.insert_or_assign(key, std::move(message));
        }

        // Update sibling pointers
        this->rightSibling = sibling->rightSibling;
        if (sibling->rightSibling) {
            InternalNodePtr rightSiblingPtr = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(this->cache->get(sibling->rightSibling));
            rightSiblingPtr->leftSibling = this->id;
        }

        // Update parent pointers
        for (auto& child : this->children) {
            InternalNodePtr childPtr = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(this->cache->get(child));
            childPtr->parent = this->id;
        }

        oldChild = { KeyType(), sibling->id, ChildChangeType::MergeRight };
    }

    return ErrorCode::Success;
}

template <typename KeyType, typename ValueType>
size_t BeTreeInternalNode<KeyType, ValueType>::serialize(char*& buf) const {
    std::stringstream memoryStream{};
    this->serialize(memoryStream);

    std::string data = memoryStream.str();
    size_t bufferSize = data.size();
    buf = new char[bufferSize];
    memcpy(buf, data.c_str(), bufferSize);

    return bufferSize;
}

template <typename KeyType, typename ValueType>
void BeTreeInternalNode<KeyType, ValueType>::serialize(std::ostream& os) const {
    static_assert(
        std::is_trivial<KeyType>::value &&
        std::is_standard_layout<KeyType>::value &&
        std::is_trivial<ValueType>::value &&
        std::is_standard_layout<ValueType>::value,
        "Can only deserialize POD types with this function");

    assert(this->keys.size() == this->children.size() - 1 && "The number of keys should be one less than the number of children");
    uint16_t numKeys = this->keys.size();
    uint16_t numMessages = this->messageBuffer.size();
    auto start = os.tellp();

    os.put(0); // type
    os.write(reinterpret_cast<const char*>(&numKeys), sizeof(uint16_t));
    os.write(reinterpret_cast<const char*>(&numMessages), sizeof(uint16_t));
    os.write(reinterpret_cast<const char*>(&this->parent), sizeof(uint64_t));
    os.write(reinterpret_cast<const char*>(&this->leftSibling), sizeof(uint64_t));
    os.write(reinterpret_cast<const char*>(&this->rightSibling), sizeof(uint64_t));
    os.write(reinterpret_cast<const char*>(&this->lowestSearchKey), sizeof(KeyType));

    os.write(reinterpret_cast<const char*>(this->keys.data()), numKeys * sizeof(KeyType));
    size_t remainingBytes = ((this->fanout - 1) - numKeys) * sizeof(KeyType);
    for (size_t i = 0; i < remainingBytes; ++i) {
        os.put(0);
    }

    os.write(reinterpret_cast<const char*>(this->children.data()), (numKeys + 1) * sizeof(uint64_t));
    remainingBytes = ((this->fanout - 1) - numKeys) * sizeof(uint64_t);
    for (size_t i = 0; i < remainingBytes; ++i) {
        os.put(0);
    }

    for (auto& [key, message] : this->messageBuffer) {
        message->serialize(os);
    }
    remainingBytes = (this->maxBufferSize - numMessages) * Message<KeyType, ValueType>::getSerializedSize();
    for (size_t i = 0; i < remainingBytes; ++i) {
        os.put(0);
    }

    auto end = os.tellp();
    assert(this->getSerializedSize() == (end - start) && "Data size mismatch");
}

template <typename KeyType, typename ValueType>
size_t BeTreeInternalNode<KeyType, ValueType>::deserialize(char*& buf, size_t bufferSize) {
    std::stringstream memoryStream{};
    memoryStream.write(buf, bufferSize);
    this->deserialize(memoryStream);
    return memoryStream.tellg();
}

template <typename KeyType, typename ValueType>
void BeTreeInternalNode<KeyType, ValueType>::deserialize(std::istream& is) {
    static_assert(
        std::is_trivial<KeyType>::value &&
        std::is_standard_layout<KeyType>::value &&
        std::is_trivial<ValueType>::value &&
        std::is_standard_layout<ValueType>::value,
        "Can only deserialize POD types with this function");

    uint8_t type = 0;
    uint16_t numKeys = 0;
    uint16_t numMessages = 0;
    is.read(reinterpret_cast<char*>(&type), sizeof(uint8_t));
    is.read(reinterpret_cast<char*>(&numKeys), sizeof(uint16_t));
    is.read(reinterpret_cast<char*>(&numMessages), sizeof(uint16_t));
    is.read(reinterpret_cast<char*>(&this->parent), sizeof(uint64_t));
    is.read(reinterpret_cast<char*>(&this->leftSibling), sizeof(uint64_t));
    is.read(reinterpret_cast<char*>(&this->rightSibling), sizeof(uint64_t));
    is.read(reinterpret_cast<char*>(&this->lowestSearchKey), sizeof(KeyType));

    this->keys.resize(numKeys);
    is.read(reinterpret_cast<char*>(this->keys.data()), numKeys * sizeof(KeyType));
    size_t remainingBytes = ((this->fanout - 1) - numKeys) * sizeof(KeyType);
    is.seekg(remainingBytes, std::ios_base::cur);

    this->children.resize(numKeys + 1);
    is.read(reinterpret_cast<char*>(this->children.data()), (numKeys + 1) * sizeof(uint64_t));
    remainingBytes = ((this->fanout - 1) - numKeys) * sizeof(uint64_t);
    is.seekg(remainingBytes, std::ios_base::cur);

    for (size_t i = 0; i < numMessages; ++i) {
        auto message = std::make_unique<Message<KeyType, ValueType>>(MessageType::Insert, KeyType(), ValueType());
        message->deserialize(is);
        this->messageBuffer.insert_or_assign(message->key, std::move(message));
    }
    remainingBytes = (this->maxBufferSize - numMessages) * Message<KeyType, ValueType>::getSerializedSize();
    is.seekg(remainingBytes, std::ios_base::cur);

    assert(this->keys.size() == this->children.size() - 1 && "The number of keys should be one less than the number of children");
    assert(this->getSerializedSize() == is.tellg() && "Data size mismatch");
}


template <typename KeyType, typename ValueType>
void BeTreeInternalNode<KeyType, ValueType>::printNode(std::ostream& out) const {
    out << "Internal: ";
    for (size_t i = 0; i < this->size(); ++i) {
        out << this->keys[i] << " ";
    }
    out << "Buffer: ";
    for (auto& [key, message] : this->messageBuffer) {
        // if the message is a remove message, print it with a '-' and red
        // if the message is a insert message, print it with a '+' and green
        if (message->type == MessageType::Remove) {
            out << "\033[1;31m-" << key << "\033[0m ";
        } else if (message->type == MessageType::Insert) {
            out << "\033[1;32m+" << key << "\033[0m ";
        } else {
            out << key << " ";
        }
    }
    // also print the lowest search key
    out << "Lowest: " << this->lowestSearchKey;
    out << "\n";
    for (size_t i = 0; i < this->children.size(); ++i) {
        BeTreeNodePtr child = this->cache->get(this->children[i]);
        child->printNode(out);
    }
}
