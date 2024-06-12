#pragma once

#include "BeTreeInternalNode.hpp"
#include "BeTreeLeafNode.hpp"
#include "BeTreeMessage.hpp"
#include "ErrorCodes.h"
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <utility>
#include <vector>

// Forward declarations
template <typename KeyType, typename ValueType> class BeTreeNode;
template <typename KeyType, typename ValueType> class BeTreeInternalNode;
template <typename KeyType, typename ValueType> class BeTreeLeafNode;
template <typename KeyType, typename ValueType> struct Message;
enum class ErrorCode;
enum class MessageType;

template <typename KeyType, typename ValueType>
class BeTreeNode : public std::enable_shared_from_this<BeTreeNode<KeyType, ValueType>> {
public:
    using MessagePtr = std::unique_ptr<Message<KeyType, ValueType>>;
    using BeTreeNodePtr = std::shared_ptr<BeTreeNode<KeyType, ValueType>>;
    using InternalNodePtr = std::shared_ptr<BeTreeInternalNode<KeyType, ValueType>>;
    using LeafNodePtr = std::shared_ptr<BeTreeLeafNode<KeyType, ValueType>>;

    uint16_t fanout;
    uint16_t level;
    std::vector<KeyType> keys;

    InternalNodePtr parent;
    BeTreeNodePtr leftSibling;
    BeTreeNodePtr rightSibling;

    struct ChildChange {
        KeyType key;
        BeTreeNodePtr node;
        bool isInsert;
    };

    virtual ~BeTreeNode() = default;
    BeTreeNode(uint16_t fanout, uint16_t level, InternalNodePtr parent = nullptr)
        : fanout(fanout), level(level), parent(parent) {}

    virtual ErrorCode insert(MessagePtr message, ChildChange& newChild) = 0;
    virtual ErrorCode remove(MessagePtr message, uint16_t indexInParent, ChildChange& oldChild) = 0;
    virtual std::pair<ValueType, ErrorCode> search(MessagePtr message) = 0;

    // Helper functions
    bool isLeaf() const { return level == 0; }
    bool isRoot() const { return this->parent == nullptr; }
    uint16_t size() const { return this->keys.size(); }
    bool isUnderflowing() const { return size() < this->fanout / 2; }
    bool isMergeable() const { return size() == this->fanout / 2; }
    bool isBorrowable() const { return size() > this->fanout / 2; }
    bool isSplittable() const { return size() >= this->fanout - 1; }
    bool isOverflowing() const { return size() >= this->fanout; }

    auto getIndex(KeyType key) {
        return std::lower_bound(keys.begin(), keys.end(), key);
    }

    virtual KeyType getLowestSearchKey() const = 0;

    virtual void printNode(std::ostream& out) const = 0;
};
