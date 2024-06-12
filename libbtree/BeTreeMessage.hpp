#pragma once

// Base Message struct template
enum class MessageType {
    Insert,
    Remove,
    Search,
    // Upsert, // Not used in the current implementation
};
template <typename KeyType, typename ValueType>
struct Message {
public:
    KeyType key;
    ValueType value;
    MessageType type;
    //uint64_t timestamp; // timestamp for message ordering

    //Message(MessageType type, KeyType key) : type(type), key(key), timestamp(++global_timestamp) {}
    //Message(MessageType type, KeyType key, ValueType value) : type(type), key(key), value(value), timestamp(++global_timestamp) {}
    Message(MessageType type, KeyType key) : type(type), key(key) {}
    Message(MessageType type, KeyType key, ValueType value) : type(type), key(key), value(value) {}
    virtual ~Message() = default;

private:
    //static uint64_t global_timestamp; // Global timestamp counter
};

// Definitions for static members
//template <typename KeyType, typename ValueType>
//uint64_t Message<KeyType, ValueType>::global_timestamp = 0;
