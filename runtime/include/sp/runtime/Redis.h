#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "sp/runtime/Net.h"

namespace sp::rt::redis {

// ============================================================================
// RESP (REdis Serialization Protocol) Types & Utilities
// ============================================================================

enum class RespType {
    SimpleString,
    Error,
    Integer,
    BulkString,
    Array,
    Null
};

struct RespValue {
    RespType type{RespType::Null};
    std::string stringVal;
    int64_t intVal{0};
    std::vector<RespValue> arrayVal;

    bool isNull() const { return type == RespType::Null; }
    bool isError() const { return type == RespType::Error; }
    const std::string& getString() const { return stringVal; }
    int64_t getInt() const { return intVal; }
    const std::vector<RespValue>& getArray() const { return arrayVal; }
};

std::string formatCommand(const std::vector<std::string>& args);
bool parseResp(const char* data, size_t len, RespValue& outVal, size_t& bytesConsumed);

// ============================================================================
// Redis Client
// ============================================================================

class RedisClient {
public:
    using MessageCallback = std::function<void(const std::string& channel, const std::string& message)>;

    RedisClient();
    ~RedisClient();

    bool connect(const std::string& host = "127.0.0.1", int port = 6379, int timeoutMs = 2000);
    void disconnect();
    bool isConnected() const;

    void setMockMode(bool enabled);
    bool isMockMode() const { return m_mockMode; }

    RespValue execute(const std::vector<std::string>& args);

    // Core Redis API
    bool auth(const std::string& password);
    std::string ping(const std::string& msg = "");

    // Strings & Keys
    std::optional<std::string> get(const std::string& key);
    bool set(const std::string& key, const std::string& value);
    bool setEx(const std::string& key, int64_t seconds, const std::string& value);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    bool expire(const std::string& key, int64_t seconds);
    int64_t ttl(const std::string& key);
    int64_t incr(const std::string& key);
    int64_t decr(const std::string& key);

    // Hashes
    std::optional<std::string> hget(const std::string& key, const std::string& field);
    bool hset(const std::string& key, const std::string& field, const std::string& value);
    bool hdel(const std::string& key, const std::string& field);
    std::unordered_map<std::string, std::string> hgetAll(const std::string& key);

    // Lists
    int64_t lpush(const std::string& key, const std::string& value);
    int64_t rpush(const std::string& key, const std::string& value);
    std::optional<std::string> lpop(const std::string& key);
    std::optional<std::string> rpop(const std::string& key);
    int64_t llen(const std::string& key);
    std::vector<std::string> lrange(const std::string& key, int64_t start, int64_t stop);

    // Pub / Sub
    int64_t publish(const std::string& channel, const std::string& message);
    void subscribe(const std::vector<std::string>& channels, MessageCallback onMessage);

    // Mock storage access (for testing verification)
    void clearMockStore();

private:
    std::unique_ptr<sp::rt::net::TcpSocket> m_socket;
    std::string m_host{"127.0.0.1"};
    int m_port{6379};
    bool m_connected{false};
    bool m_mockMode{true}; // Defaults to mock mode for reliable standalone testability
    mutable std::mutex m_mutex;

    // In-memory mock engine
    struct MockData {
        std::unordered_map<std::string, std::string> strings;
        std::unordered_map<std::string, int64_t> expires;
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hashes;
        std::unordered_map<std::string, std::vector<std::string>> lists;
        std::unordered_map<std::string, std::vector<MessageCallback>> subscribers;
    };
    MockData m_mock;
    RespValue executeMock(const std::vector<std::string>& args);
};

// ============================================================================
// Redis Connection Pool
// ============================================================================

struct RedisPoolConfig {
    std::string host{"127.0.0.1"};
    int port{6379};
    std::string password;
    size_t minConnections{2};
    size_t maxConnections{10};
    int acquireTimeoutMs{3000};
    bool mockMode{true};
};

class RedisPool {
public:
    explicit RedisPool(RedisPoolConfig config = {});
    ~RedisPool();

    RedisPool(const RedisPool&) = delete;
    RedisPool& operator=(const RedisPool&) = delete;

    std::shared_ptr<RedisClient> acquire();
    void release(std::shared_ptr<RedisClient> client);

    size_t available() const;
    size_t total() const;
    void close();

private:
    RedisPoolConfig m_config;
    std::queue<std::shared_ptr<RedisClient>> m_pool;
    size_t m_totalConnections{0};
    bool m_closed{false};
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;

    std::shared_ptr<RedisClient> createClient();
};

} // namespace sp::rt::redis
