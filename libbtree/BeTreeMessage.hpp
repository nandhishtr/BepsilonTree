#pragma once
#include <cstring>
#include <iosfwd>
#include <iostream>

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
    Message(MessageType type, KeyType key) : type(type), key(key), value(0) {}
    Message(MessageType type, KeyType key, ValueType value) : type(type), key(key), value(value) {}
    virtual ~Message() = default;

    static size_t getSerializedSize() {
        return sizeof(MessageType) + sizeof(KeyType) + sizeof(ValueType);
    }

    size_t serialize(char*& buf) const {// returns the number of bytes written
        size_t bytesWritten = 0;
        memcpy(buf, &type, sizeof(MessageType));
        bytesWritten += sizeof(MessageType);

        memcpy(buf + bytesWritten, &key, sizeof(KeyType));
        bytesWritten += sizeof(KeyType);

        memcpy(buf + bytesWritten, &value, sizeof(ValueType));
        bytesWritten += sizeof(ValueType);

        return bytesWritten;
    }

    void serialize(std::ostream& os) const {
        os.write(reinterpret_cast<const char*>(&type), sizeof(MessageType));
        os.write(reinterpret_cast<const char*>(&key), sizeof(KeyType));
        os.write(reinterpret_cast<const char*>(&value), sizeof(ValueType));
    }

    size_t deserialize(char*& buf, size_t bufferSize) { // returns the number of bytes read
        size_t bytesRead = 0;
        memcpy(&type, buf, sizeof(MessageType));
        bytesRead += sizeof(MessageType);

        memcpy(&key, buf + bytesRead, sizeof(KeyType));
        bytesRead += sizeof(KeyType);

        memcpy(&value, buf + bytesRead, sizeof(ValueType));
        bytesRead += sizeof(ValueType);

        return bytesRead;
    }

    void deserialize(std::istream& is) {
        is.read(reinterpret_cast<char*>(&type), sizeof(MessageType));
        is.read(reinterpret_cast<char*>(&key), sizeof(KeyType));
        is.read(reinterpret_cast<char*>(&value), sizeof(ValueType));
    }
};
