#pragma once

#include "pch.h"
#include "BeTreeMessage.hpp"
#include "ErrorCodes.h"
#include <cassert>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

// Forward declarations
template <typename KeyType, typename ValueType> class BeTreeInternalNode;
template <typename KeyType, typename ValueType> class BeTreeNode;
template <typename KeyType, typename ValueType> class BeTreeLeafNode;
template <typename KeyType, typename ValueType> struct Message;
enum class ErrorCode;
enum class MessageType;

template <typename KeyType, typename ValueType>
class BeTreeInternalNode : public BeTreeNode<KeyType, ValueType> {
public:
    using typename BeTreeNode<KeyType, ValueType>::MessagePtr;
    using typename BeTreeNode<KeyType, ValueType>::BeTreeNodePtr;
    using typename BeTreeNode<KeyType, ValueType>::InternalNodePtr;
    using typename BeTreeNode<KeyType, ValueType>::LeafNodePtr;
    using typename BeTreeNode<KeyType, ValueType>::ChildChange;
    using typename BeTreeNode<KeyType, ValueType>::ChildChangeType;

    std::map<KeyType, MessagePtr> messageBuffer;
    uint16_t maxBufferSize;

    std::vector<BeTreeNodePtr> children;

    BeTreeInternalNode(uint16_t fanout, uint16_t level, uint16_t maxBufferSize = 0, InternalNodePtr parent = nullptr)
        : BeTreeNode<KeyType, ValueType>(fanout, level, parent) {
        if (maxBufferSize == 0) {
            this->maxBufferSize = fanout - 1;
        } else {
            this->maxBufferSize = maxBufferSize;
        }
    }

    bool isFlushable() const {
        return this->messageBuffer.size() >= maxBufferSize;
    }

    // TODO: We could somehow add a lowerstSearchKey field to the node to avoid searching for it
    KeyType getLowestSearchKey() const override {
        // Return the lowest key of the first child or the lowest key of the buffer
        auto lowestKey = this->children[0]->getLowestSearchKey();
        if (this->messageBuffer.size() > 0) {
            return std::min(lowestKey, this->messageBuffer.begin()->first);
        } else {
            return lowestKey;
        }
    }

    ErrorCode applyMessage(MessagePtr message, uint16_t indexInParent, ChildChange& childChange) override;
    ErrorCode insert(MessagePtr message, ChildChange& newChild) override;
    ErrorCode remove(MessagePtr message, uint16_t indexInParent, ChildChange& oldChild) override;
    std::pair<ValueType, ErrorCode> search(MessagePtr message) override;

    ErrorCode flushBuffer(ChildChange& childChange);

    ErrorCode split(ChildChange& newChild);
    ErrorCode handleUnderflow(uint16_t indexInParent, ChildChange& oldChild);
    ErrorCode redistribute(uint16_t indexInParent, bool withLeft, ChildChange& oldChild);
    ErrorCode merge(uint16_t indexInParent, ChildChange& oldChild, bool withLeft);

    void printNode(std::ostream& out) const override;
};

// Implementations

template <typename KeyType, typename ValueType>
ErrorCode BeTreeInternalNode<KeyType, ValueType>::applyMessage(MessagePtr message, uint16_t indexInParent, ChildChange& childChange) {
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
    return children[idx]->search(std::move(message));
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
    ChildChange newChild = { KeyType(), nullptr , ChildChangeType::None };

    // NOTE: For now we will stop flushing messages if the child node splits or merges or if we arrived at a leaf node
    //       This is to avoid the complexity of handling the index changes in the buffer
    // TODO: Right now this does not really work as intended, because when the child of a child splits or merges
    //       we may call that child again. Right now this isn't a problem, but when we are using files for storage
    //       this will be a latency issue.
    auto it = maxStart;
    ErrorCode err = ErrorCode::Success;
    for (it = maxStart; it != maxEnd; ++it) {
        err = this->children[idx]->applyMessage(std::move(it->second), idx, newChild);
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
        //uint16_t indexInParent = this->parent->getIndex(this->getLowestSearchKey()) - this->parent->keys.begin(); // use upper bound instead
        uint16_t indexInParent = std::upper_bound(this->parent->keys.begin(), this->parent->keys.end(), this->getLowestSearchKey()) - this->parent->keys.begin();
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
    auto newInternal = std::make_shared<BeTreeInternalNode<KeyType, ValueType>>(this->fanout, this->level, this->maxBufferSize, this->parent);
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

    // Set sibling pointers
    newInternal->leftSibling = this->shared_from_this();
    newInternal->rightSibling = this->rightSibling;
    if (this->rightSibling) {
        this->rightSibling->leftSibling = newInternal;
    }
    this->rightSibling = newInternal;

    // Set parent pointers
    for (auto& child : newInternal->children) {
        child->parent = newInternal;
    }

    // Update parent
    newChild = { newPivot, newInternal, ChildChangeType::Split };
    return ErrorCode::Success;
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeInternalNode<KeyType, ValueType>::handleUnderflow(uint16_t indexInParent, ChildChange& oldChild) {
    if (this->isRoot()) {
        return ErrorCode::Success;
    }
    // redistribute or merge with sibling
    if (this->leftSibling && this->leftSibling->isBorrowable() && this->leftSibling->parent == this->parent) {
        return redistribute(indexInParent, true, oldChild);
    } else if (this->leftSibling && this->leftSibling->parent == this->parent) {
        return merge(indexInParent, oldChild, true);
    } else if (this->rightSibling && this->rightSibling->isBorrowable() && this->rightSibling->parent == this->parent) {
        return redistribute(indexInParent, false, oldChild);
    } else if (this->rightSibling && this->rightSibling->parent == this->parent) {
        return merge(indexInParent, oldChild, false);
    } else {
        return ErrorCode::Success;
    }
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeInternalNode<KeyType, ValueType>::redistribute(uint16_t indexInParent, bool withLeft, ChildChange& oldChild) {
    auto sibling = withLeft ?
        std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(this->leftSibling) :
        std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(this->rightSibling);

    auto sizeDiff = (sibling->size() - this->size()) / 2;
    assert(sizeDiff >= 0);

    if (withLeft) {
        this->keys.insert(this->keys.begin(), this->getLowestSearchKey());

        this->keys.insert(this->keys.begin(), sibling->keys.end() - sizeDiff + 1, sibling->keys.end());
        this->children.insert(this->children.begin(), sibling->children.end() - sizeDiff, sibling->children.end());
        sibling->keys.erase(sibling->keys.end() - sizeDiff, sibling->keys.end());
        sibling->children.erase(sibling->children.end() - sizeDiff, sibling->children.end());

        // Move messages from sibling to this node
        // TODO: Use binary search to find the split point in the buffer instead of iterating through it
        for (auto it = sibling->messageBuffer.begin(); it != sibling->messageBuffer.end();) {
            if (it->first >= this->getLowestSearchKey()) {
                this->messageBuffer.insert_or_assign(it->first, std::move(it->second));
                it = sibling->messageBuffer.erase(it);
            } else {
                ++it;
            }
        }

        // Update parent of redistributed children
        for (int i = 0; i < sizeDiff; ++i) {
            this->children[i]->parent = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(this->shared_from_this());
        }

        // Update pivot key in parent
        oldChild = { this->getLowestSearchKey(), nullptr, ChildChangeType::RedistributeLeft };
    } else {
        this->keys.insert(this->keys.end(), sibling->getLowestSearchKey());
        this->keys.insert(this->keys.end(), sibling->keys.begin(), sibling->keys.begin() + sizeDiff - 1);
        this->children.insert(this->children.end(), sibling->children.begin(), sibling->children.begin() + sizeDiff);
        sibling->keys.erase(sibling->keys.begin(), sibling->keys.begin() + sizeDiff);
        sibling->children.erase(sibling->children.begin(), sibling->children.begin() + sizeDiff);

        // Move messages from sibling to this node
        // TODO: Use binary search to find the split point in the buffer instead of iterating through it
        for (auto it = sibling->messageBuffer.begin(); it != sibling->messageBuffer.end();) {
            //if (it->first < sibling->getLowestSearchKey()) {
            // NOTE: we can't directly call getLowestSearchKey because the lowest key of the sibling might be in their buffer
            //       which leads to the case where we don't move any messages from the sibling to this node
            //       so we have to call getLowestSearchKey on the siblings first child
            if (it->first < sibling->children[0]->getLowestSearchKey()) {
                this->messageBuffer.insert_or_assign(it->first, std::move(it->second));
                it = sibling->messageBuffer.erase(it);
            } else {
                ++it;
            }
        }

        // Update parent of redistributed children
        for (int i = 0; i < sizeDiff; ++i) {
            this->children[this->children.size() - sizeDiff + i]->parent = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(this->shared_from_this());
        }

        // Update pivot key in parent
        oldChild = { sibling->getLowestSearchKey(), nullptr, ChildChangeType::RedistributeRight };
    }

    return ErrorCode::Success;
}

template <typename KeyType, typename ValueType>
ErrorCode BeTreeInternalNode<KeyType, ValueType>::merge(uint16_t indexInParent, ChildChange& oldChild, bool withLeft) {
    auto sibling = withLeft ?
        std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(this->leftSibling) :
        std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(this->rightSibling);

    if (withLeft) {
        // pull the pivot key from the parent
        sibling->keys.insert(sibling->keys.end(), this->parent->keys[indexInParent - 1]);

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
            this->rightSibling->leftSibling = sibling;
        }

        // Update parent pointers
        for (auto& child : sibling->children) {
            child->parent = sibling;
        }

        oldChild = { KeyType(), this->shared_from_this(), ChildChangeType::MergeLeft };
    } else {
        // pull the pivot key from the parent
        this->keys.insert(this->keys.end(), this->parent->keys[indexInParent]);

        // merge the keys and children
        this->keys.insert(this->keys.end(), sibling->keys.begin(), sibling->keys.end());
        this->children.insert(this->children.end(), sibling->children.begin(), sibling->children.end());
        this->rightSibling = sibling->rightSibling;

        // combine the message buffers
        for (auto& [key, message] : sibling->messageBuffer) {
            this->messageBuffer.insert_or_assign(key, std::move(message));
        }

        // Update sibling pointers
        if (sibling->rightSibling) {
            sibling->rightSibling->leftSibling = this->shared_from_this();
        }

        // Update parent pointers
        for (auto& child : this->children) {
            child->parent = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(this->shared_from_this());
        }

        oldChild = { KeyType(), sibling, ChildChangeType::MergeRight };
    }

    return ErrorCode::Success;
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
    out << "\n";
    for (size_t i = 0; i < this->children.size(); ++i) {
        this->children[i]->printNode(out);
    }
}
