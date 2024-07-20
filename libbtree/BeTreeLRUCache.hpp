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
template <typename KeyType, typename ValueType> class BeTreeIStorage;

template <typename KeyType, typename ValueType>
class BeTreeLRUCache {
private:
    using NodePtr = std::shared_ptr<BeTreeNode<KeyType, ValueType>>;

    size_t capacity;
    std::list<std::pair<uint64_t, NodePtr>> cache;
    std::unordered_map<uint64_t, decltype(cache.begin())> cacheMap;
    std::shared_ptr<BeTreeIStorage<KeyType, ValueType>> storage;

public:
    BeTreeLRUCache(size_t capacity, std::shared_ptr<BeTreeIStorage<KeyType, ValueType>> storage = nullptr)
        : capacity(capacity), storage(storage) {}

    void setStorage(std::shared_ptr<BeTreeIStorage<KeyType, ValueType>> storage) {
        this->storage = storage;
    }

    void create(NodePtr node) {
        uint64_t id = storage->saveNode(0, node);
        put(id, node);
        node->id = id;
    }

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
            NodePtr node = storage->loadNode(id);
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
            // Iterate through the cache from the back and remove the first node with use_count() <= 2
            for (auto it = cache.rbegin(); it != cache.rend(); ++it) {
                if (it->second.use_count() <= 2) {
                    storage->saveNode(it->first, it->second);
                    cacheMap.erase(it->first);
                    cache.erase(std::next(it).base());
                    break;
                }
            }
        }
    }
};
