#pragma once

#include <list>
#include <unordered_map>
#include <memory>
#include <functional>
#include "BeTreeNode.hpp"
#include "BeTreeFileStorage.hpp"

template <typename KeyType, typename ValueType>
class BeTreeLRUCache {
private:
    using NodePtr = std::shared_ptr<BeTreeNode<KeyType, ValueType>>;

    size_t capacity;
    std::list<std::pair<uint64_t, NodePtr>> cache;
    std::unordered_map<uint64_t, decltype(cache.begin())> cacheMap;
    std::shared_ptr<BeTreeFileStorage<KeyType, ValueType>> storage;

public:
    BeTreeLRUCache(size_t capacity, std::shared_ptr<BeTreeFileStorage<KeyType, ValueType>> storage)
        : capacity(capacity), storage(storage) {}

    void put(uint64_t id, NodePtr node) {
        if (cacheMap.find(id) != cacheMap.end()) {
            // Move the existing item to the front
            cache.splice(cache.begin(), cache, cacheMap[id]);
            cacheMap[id]->second = node;
        } else {
            // Add new item
            if (cache.size() >= capacity) {
                evictLeastUsed();
            }
            cache.push_front({ id, node });
            cacheMap[id] = cache.begin();
        }
    }

    NodePtr get(uint64_t id) {
        auto it = cacheMap.find(id);
        if (it == cacheMap.end()) {
            NodePtr node = storage->readNode(id);
            put(id, node);
            return node;
        } else {
            // Move the existing item to the front
            cache.splice(cache.begin(), cache, it->second);
            return it->second->second;
        }
    }

    void remove(uint64_t id) {
        auto it = cacheMap.find(id);
        if (it != cacheMap.end()) {
            cache.erase(it->second);
            cacheMap.erase(it);
        }
    }

    void deleteNode(uint64_t id, NodePtr node) {
        this->remove(id);
        storage->removeNode(id, node);
    }

    void flush() {
        for (const auto& item : cache) {
            storage->saveNode(item.first, item.second);
        }
    }

private:
    void evictLeastUsed() {
        if (!cache.empty()) {
            auto lastItem = cache.back();
            storage->saveNode(lru.first, lru.second);
            cacheMap.erase(lru.first);
            cache.pop_back();
        }
    }
};
