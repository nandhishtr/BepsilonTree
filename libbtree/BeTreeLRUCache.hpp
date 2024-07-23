#pragma once
#include "BeTreeIStorage.hpp"
#include "BeTreeNode.hpp"
#include <cstdint>
#include <iostream>
#include <list>
#include <memory>
#include <unordered_map>
#include <utility>

template <typename KeyType, typename ValueType> class BeTreeNode;
template <typename KeyType, typename ValueType> class BeTreeInternalNode;
template <typename KeyType, typename ValueType> class BeTreeIStorage;

template <typename KeyType, typename ValueType>
class BeTreeLRUCache {
private:
    using NodePtr = std::shared_ptr<BeTreeNode<KeyType, ValueType>>;

    size_t capacity;
    std::list<std::pair<uint64_t, NodePtr>> cache;
    std::weak_ptr<BeTreeIStorage<KeyType, ValueType>> storage;

public:
    BeTreeLRUCache(size_t capacity, std::shared_ptr<BeTreeIStorage<KeyType, ValueType>> storage = nullptr)
        : capacity(capacity), storage(storage) {}

    void setStorage(std::shared_ptr<BeTreeIStorage<KeyType, ValueType>> storage) {
        this->storage = storage;
    }

    void create(NodePtr node) {
        uint64_t id = storage.lock()->saveNode(0, node);
        put(id, node);
        node->id = id;
    }

    void put(uint64_t id, NodePtr node) {
        auto it = std::find_if(cache.begin(), cache.end(), [id](const std::pair<uint64_t, NodePtr>& item) { return item.first == id; });

        if (it != cache.end()) {
            // Move the existing item to the front
            cache.splice(cache.begin(), cache, it);
            it->second = node;
        } else {
            // Add new item
            if (cache.size() >= capacity) {
                evictLeastUsed();
            }
            cache.push_front({ id, node });
        }
    }

    NodePtr get(uint64_t id) {
        auto it = std::find_if(cache.begin(), cache.end(), [id](const std::pair<uint64_t, NodePtr>& item) { return item.first == id; });
        if (it == cache.end()) {
            NodePtr node = storage.lock()->loadNode(id);
            put(id, node);
            return node;
        } else {
            // Move the existing item to the front
            cache.splice(cache.begin(), cache, it);
            return it->second;
        }
    }

    void remove(uint64_t id) {
        auto it = std::find_if(cache.begin(), cache.end(), [id](const std::pair<uint64_t, NodePtr>& item) { return item.first == id; });
        if (it != cache.end()) {
            cache.erase(it);
        }
    }

    void deleteNode(uint64_t id, NodePtr node) {
        this->remove(id);
        storage.lock()->removeNode(id, node);
    }

    void flush() {
        for (const auto& item : cache) {
            storage.lock()->saveNode(item.first, item.second);
        }
    }

private:
    void evictLeastUsed() {
        if (cache.empty()) {
            return;
        }

        // Iterate through the cache from the back and remove the first nodes with use_count() == 1 until the cache size is less than the capacity
        auto it = cache.rbegin();
        while (it != cache.rend()) {
            if (it->second.use_count() > 1 || it->second->isRoot()) {
                ++it;
                continue;
            }
            // HACK: Do not serialize internal nodes that have more messages than allowed
            if (!it->second->isLeaf()) {
                auto internalNode = std::static_pointer_cast<BeTreeInternalNode<KeyType, ValueType>>(it->second);
                if (internalNode->messageBuffer.size() > internalNode->maxBufferSize) {
                    ++it;
                    continue;
                }
            }

            storage.lock()->saveNode(it->first, it->second);
            cache.erase(std::next(it).base());
            // also print if it's the root node
            if (cache.size() < capacity) {
                break;
            }
        }
    }
};
