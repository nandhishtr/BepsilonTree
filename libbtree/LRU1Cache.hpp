#pragma once

#include <list>
#include <unordered_map>
#include <memory>
#include <functional>
#include "BeTreeNode.hpp"
#include "File1Storage.hpp"

template <typename KeyType, typename ValueType>
class LRU1Cache {
public:
    using NodePtr = std::shared_ptr<BeTreeNode<KeyType, ValueType>>;
    using NodeId = uint64_t;

    LRU1Cache(size_t capacity)
        : capacity_(capacity)
    {
        fileStorage_ = std::make_shared<File1Storage<KeyType, ValueType>>("D:/filestore.hdb");
    }

    void put(NodeId id, NodePtr node) {
        if (cache_.find(id) != cache_.end()) {
            // Move the existing item to the front
            lruList_.splice(lruList_.begin(), lruList_, cache_[id]);
            cache_[id]->second = node;
        }
        else {
            // Add new item
            if (cache_.size() >= capacity_) {
                evictLeastUsed();
            }
            lruList_.push_front({ id, node });
            cache_[id] = lruList_.begin();
        }
    }

    NodePtr get(NodeId id) {
        auto it = cache_.find(id);
        if (it == cache_.end()) {
            // Node not in cache, try to load from file storage
           /* NodePtr node = fileStorage_->loadNode(id);
            if (node) {
                put(id, node);
                return node;
            }*/
            return nullptr;
        }
        // Move accessed item to the front
        lruList_.splice(lruList_.begin(), lruList_, it->second);
        return it->second->second;
    }

    void flushAllToStorage() {
        for (const auto& item : lruList_) {
            fileStorage_->saveNode(item.first, item.second);
        }
    }

private:
    size_t capacity_;
    std::list<std::pair<NodeId, NodePtr>> lruList_;
    std::unordered_map<NodeId, typename std::list<std::pair<NodeId, NodePtr>>::iterator> cache_;
    std::shared_ptr<File1Storage<KeyType, ValueType>> fileStorage_;

    void evictLeastUsed() {
        if (!lruList_.empty()) {
            auto lastItem = lruList_.back();
            fileStorage_->saveNode(lastItem.first, lastItem.second);
            cache_.erase(lastItem.first);
            lruList_.pop_back();
        }
    }
};