#pragma once

// Forward declarations
template <typename KeyType, typename ValueType> class BeTreeInternalNode;
template <typename KeyType, typename ValueType> class BeTreeNode;
template <typename KeyType, typename ValueType> class BeTreeLeafNode;
template <typename KeyType, typename ValueType> struct Message;
enum class ErrorCode;
enum class MessageType;

#include "pch.h"
#include "BeTreeMessage.hpp"
#include "BeTreeNode.hpp"
#include "ErrorCodes.h"
#include <cassert>
#include <cstdint>
#include <iosfwd>
#include <map>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

template <typename KeyType, typename ValueType>
class BeTreeInternalNode : public BeTreeNode<KeyType, ValueType> {
public:
    using typename BeTreeNode<KeyType, ValueType>::MessagePtr;
    using typename BeTreeNode<KeyType, ValueType>::BeTreeNodePtr;
    using typename BeTreeNode<KeyType, ValueType>::InternalNodePtr;
    using typename BeTreeNode<KeyType, ValueType>::LeafNodePtr;
    using typename BeTreeNode<KeyType, ValueType>::ChildChange;

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

    ErrorCode insert(MessagePtr message, ChildChange& newChild) override {
        this->messageBuffer.insert_or_assign(message->key, std::move(message));

        if (isFlushable()) {
            return flushBuffer(newChild);
        } else {
            return ErrorCode::Success;
        }
    }

    ErrorCode remove(MessagePtr message, uint16_t indexInParent, ChildChange& oldChild) override {
        this->messageBuffer.insert_or_assign(message->key, std::move(message));

        if (isFlushable()) {
            return flushBuffer(oldChild);
        } else {
            return ErrorCode::Success;
        }
    }

    std::pair<ValueType, ErrorCode> search(MessagePtr message) override {
        auto idx = std::upper_bound(this->keys.begin(), this->keys.end(), message->key) - this->keys.begin();
        // Search for the message in the buffer
        auto it = this->messageBuffer.find(message->key);
        if (it != this->messageBuffer.end()) {
            return { it->second->value, ErrorCode::Success };
        }

        // If not found in the buffer, search in the child node
        return children[idx]->search(std::move(message));
    }

    ErrorCode flushBuffer(ChildChange& childChange) {
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
        uint16_t idx = std::upper_bound(this->keys.begin(), this->keys.end(), maxStart->first) - this->keys.begin();
        ChildChange newChild = { KeyType(), nullptr , true };
        ChildChange oldChild = { KeyType(), nullptr , true };

        // NOTE: For now we will stop flushing messages if the child node splits or merges
        //       This is to avoid the complexity of handling the index changes in the buffer
        auto it = maxStart;
        for (it = maxStart; it != maxEnd; ++it) {
            if (it->second->type == MessageType::Insert) {
                ErrorCode err = this->children[idx]->insert(std::move(it->second), newChild);
                if (newChild.node) {
                    // Insert the new child
                    this->keys.insert(this->keys.begin() + idx, newChild.key);
                    this->children.insert(this->children.begin() + idx + 1, newChild.node);
                    break;
                } else if (err != ErrorCode::Success) {
                    return err;
                }
            } else if (it->second->type == MessageType::Remove) {
                ErrorCode err = this->children[idx]->remove(std::move(it->second), idx, oldChild);
                if (oldChild.node) {
                    // Remove the child
                    if (oldChild.node == this->children[idx]) {
                        this->keys.erase(this->keys.begin() + idx - 1);
                        this->children.erase(this->children.begin() + idx);
                    } else {
                        this->keys.erase(this->keys.begin() + idx);
                        this->children.erase(this->children.begin() + idx + 1);
                    }
                    break;
                } else if (err != ErrorCode::Success) {
                    return err;
                }
            }
        }
        // Remove the flushed messages from the buffer
        if (it != maxEnd) {
            it++;
        }
        messageBuffer.erase(maxStart, it);

        // Handle overflow and underflow of our node
        if (this->isOverflowing()) {
            return this->split(childChange);
        } else if (this->isUnderflowing() && !this->isRoot()) {
            uint16_t indexInParent = this->parent->getIndex(this->keys[0]) - this->parent->keys.begin();
            return handleUnderflow(indexInParent, childChange);
        }

        return ErrorCode::Success;
    }

    ErrorCode split(ChildChange& newChild) {
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
        newChild = { newPivot, newInternal, true };
        return ErrorCode::Success;
    }

    ErrorCode handleUnderflow(uint16_t indexInParent, ChildChange& oldChild) {
        if (this->isRoot()) {
            return ErrorCode::Success;
        }
        // redistribute or merge with sibling
        if (this->leftSibling && this->leftSibling->isBorrowable() && this->leftSibling->parent == this->parent) {
            return redistribute(indexInParent, true);
        } else if (this->leftSibling && this->leftSibling->parent == this->parent) {
            return merge(indexInParent, oldChild, true);
        } else if (this->rightSibling && this->rightSibling->isBorrowable() && this->rightSibling->parent == this->parent) {
            return redistribute(indexInParent, false);
        } else if (this->rightSibling && this->rightSibling->parent == this->parent) {
            return merge(indexInParent, oldChild, false);
        } else {
            return ErrorCode::Success;
        }
    }

    ErrorCode redistribute(uint16_t indexInParent, bool withLeft) {
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
                if (it->first >= this->keys[0]) {
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
            this->parent->keys[indexInParent - 1] = this->getLowestSearchKey();
        } else {
            this->keys.insert(this->keys.end(), sibling->getLowestSearchKey());
            this->keys.insert(this->keys.end(), sibling->keys.begin(), sibling->keys.begin() + sizeDiff - 1);
            this->children.insert(this->children.end(), sibling->children.begin(), sibling->children.begin() + sizeDiff);
            sibling->keys.erase(sibling->keys.begin(), sibling->keys.begin() + sizeDiff);
            sibling->children.erase(sibling->children.begin(), sibling->children.begin() + sizeDiff);

            // Move messages from sibling to this node
            // TODO: Use binary search to find the split point in the buffer instead of iterating through it
            for (auto it = sibling->messageBuffer.begin(); it != sibling->messageBuffer.end();) {
                if (it->first < this->keys[0]) {
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
            this->parent->keys[indexInParent] = sibling->getLowestSearchKey();
        }

        return ErrorCode::Success;
    }

    KeyType getLowestSearchKey() const override {
        return this->children[0]->getLowestSearchKey();
    }

    ErrorCode merge(uint16_t indexInParent, ChildChange& oldChild, bool withLeft) {
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

            oldChild.node = this->shared_from_this();
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

            oldChild.node = sibling;
        }

        return ErrorCode::Success;
    }

    void printNode(std::ostream& out) const override {
        out << "Internal: ";
        for (size_t i = 0; i < this->size(); ++i) {
            out << this->keys[i] << " ";
        }
        out << "Buffer: ";
        for (auto& [key, message] : this->messageBuffer) {
            out << key << " ";
        }
        out << "\n";
        for (size_t i = 0; i < this->children.size(); ++i) {
            this->children[i]->printNode(out);
        }
    }
};
