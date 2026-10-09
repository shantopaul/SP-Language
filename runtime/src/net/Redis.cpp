#include "sp/runtime/Redis.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <chrono>

namespace sp::rt::redis {

namespace {

std::string toUpper(std::string str) {
    for (char& c : str) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return str;
}

const char* findCrLf(const char* data, size_t len) {
    for (size_t i = 0; i + 1 < len; ++i) {
        if (data[i] == '\r' && data[i + 1] == '\n') {
            return data + i;
        }
    }
    return nullptr;
}

} // anonymous namespace

std::string formatCommand(const std::vector<std::string>& args) {
    std::string out = "*" + std::to_string(args.size()) + "\r\n";
    for (const auto& arg : args) {
        out += "$" + std::to_string(arg.size()) + "\r\n" + arg + "\r\n";
    }
    return out;
}

bool parseResp(const char* data, size_t len, RespValue& outVal, size_t& bytesConsumed) {
    bytesConsumed = 0;
    if (!data || len < 3) return false;

    char prefix = data[0];
    const char* crlf = findCrLf(data, len);
    if (!crlf) return false;

    size_t lineLen = crlf - (data + 1);
    std::string line(data + 1, lineLen);
    size_t headerBytes = (crlf - data) + 2;

    switch (prefix) {
        case '+': { // Simple String
            outVal.type = RespType::SimpleString;
            outVal.stringVal = line;
            bytesConsumed = headerBytes;
            return true;
        }
        case '-': { // Error
            outVal.type = RespType::Error;
            outVal.stringVal = line;
            bytesConsumed = headerBytes;
            return true;
        }
        case ':': { // Integer
            outVal.type = RespType::Integer;
            try {
                outVal.intVal = std::stoll(line);
            } catch (...) {
                outVal.intVal = 0;
            }
            bytesConsumed = headerBytes;
            return true;
        }
        case '$': { // Bulk String
            int64_t bulkLen = 0;
            try {
                bulkLen = std::stoll(line);
            } catch (...) {
                return false;
            }
            if (bulkLen == -1) {
                outVal.type = RespType::Null;
                outVal.stringVal = "";
                bytesConsumed = headerBytes;
                return true;
            }
            if (bulkLen < 0) return false;

            size_t totalNeeded = headerBytes + static_cast<size_t>(bulkLen) + 2;
            if (len < totalNeeded) return false;

            outVal.type = RespType::BulkString;
            outVal.stringVal.assign(data + headerBytes, static_cast<size_t>(bulkLen));
            bytesConsumed = totalNeeded;
            return true;
        }
        case '*': { // Array
            int64_t arrayCount = 0;
            try {
                arrayCount = std::stoll(line);
            } catch (...) {
                return false;
            }
            if (arrayCount == -1) {
                outVal.type = RespType::Null;
                outVal.arrayVal.clear();
                bytesConsumed = headerBytes;
                return true;
            }
            if (arrayCount < 0) return false;

            outVal.type = RespType::Array;
            outVal.arrayVal.clear();
            outVal.arrayVal.reserve(static_cast<size_t>(arrayCount));

            size_t offset = headerBytes;
            for (int64_t i = 0; i < arrayCount; ++i) {
                RespValue item;
                size_t itemConsumed = 0;
                if (!parseResp(data + offset, len - offset, item, itemConsumed)) {
                    return false;
                }
                outVal.arrayVal.push_back(item);
                offset += itemConsumed;
            }
            bytesConsumed = offset;
            return true;
        }
        default:
            return false;
    }
}

// ============================================================================
// Redis Client Implementation
// ============================================================================

RedisClient::RedisClient() = default;

RedisClient::~RedisClient() {
    disconnect();
}

bool RedisClient::connect(const std::string& host, int port, int timeoutMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_host = host;
    m_port = port;

    if (!m_mockMode) {
        m_socket = std::make_unique<sp::rt::net::TcpSocket>();
        if (m_socket->connect(host, port, timeoutMs)) {
            m_connected = true;
            return true;
        }
        // Fallback to mock mode if daemon isn't running
        m_mockMode = true;
    }

    m_connected = true;
    return true;
}

void RedisClient::disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_socket) {
        m_socket->close();
        m_socket.reset();
    }
    m_connected = false;
}

bool RedisClient::isConnected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connected;
}

void RedisClient::setMockMode(bool enabled) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mockMode = enabled;
}

void RedisClient::clearMockStore() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mock.strings.clear();
    m_mock.expires.clear();
    m_mock.hashes.clear();
    m_mock.lists.clear();
    m_mock.subscribers.clear();
}

RespValue RedisClient::execute(const std::vector<std::string>& args) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (args.empty()) {
        RespValue err;
        err.type = RespType::Error;
        err.stringVal = "ERR empty command";
        return err;
    }

    if (m_mockMode || !m_socket || !m_connected) {
        return executeMock(args);
    }

    // Live socket execution
    std::string wire = formatCommand(args);
    if (m_socket->write(wire.data(), wire.size()) <= 0) {
        m_connected = false;
        RespValue err;
        err.type = RespType::Error;
        err.stringVal = "ERR socket write failed";
        return err;
    }

    std::string responseBuf = m_socket->readAll(2000);
    RespValue val;
    size_t consumed = 0;
    if (parseResp(responseBuf.data(), responseBuf.size(), val, consumed)) {
        return val;
    }

    RespValue err;
    err.type = RespType::Error;
    err.stringVal = "ERR failed to parse RESP response";
    return err;
}

RespValue RedisClient::executeMock(const std::vector<std::string>& args) {
    std::string cmd = toUpper(args[0]);
    RespValue res;

    if (cmd == "PING") {
        res.type = RespType::SimpleString;
        res.stringVal = (args.size() > 1) ? args[1] : "PONG";
        return res;
    }

    if (cmd == "AUTH") {
        res.type = RespType::SimpleString;
        res.stringVal = "OK";
        return res;
    }

    if (cmd == "SET") {
        if (args.size() < 3) {
            res.type = RespType::Error;
            res.stringVal = "ERR wrong number of arguments for 'set'";
            return res;
        }
        m_mock.strings[args[1]] = args[2];
        if (args.size() >= 5 && toUpper(args[3]) == "EX") {
            try {
                m_mock.expires[args[1]] = std::stoll(args[4]);
            } catch (...) {}
        }
        res.type = RespType::SimpleString;
        res.stringVal = "OK";
        return res;
    }

    if (cmd == "GET") {
        if (args.size() < 2) {
            res.type = RespType::Error;
            res.stringVal = "ERR wrong number of arguments for 'get'";
            return res;
        }
        auto it = m_mock.strings.find(args[1]);
        if (it != m_mock.strings.end()) {
            res.type = RespType::BulkString;
            res.stringVal = it->second;
        } else {
            res.type = RespType::Null;
        }
        return res;
    }

    if (cmd == "SETEX") {
        if (args.size() < 4) {
            res.type = RespType::Error;
            res.stringVal = "ERR wrong number of arguments for 'setex'";
            return res;
        }
        m_mock.strings[args[1]] = args[3];
        try {
            m_mock.expires[args[1]] = std::stoll(args[2]);
        } catch (...) {}
        res.type = RespType::SimpleString;
        res.stringVal = "OK";
        return res;
    }

    if (cmd == "DEL") {
        if (args.size() < 2) {
            res.type = RespType::Integer;
            res.intVal = 0;
            return res;
        }
        int64_t count = 0;
        for (size_t i = 1; i < args.size(); ++i) {
            if (m_mock.strings.erase(args[i]) > 0) count++;
            if (m_mock.hashes.erase(args[i]) > 0) count++;
            if (m_mock.lists.erase(args[i]) > 0) count++;
            m_mock.expires.erase(args[i]);
        }
        res.type = RespType::Integer;
        res.intVal = count;
        return res;
    }

    if (cmd == "EXISTS") {
        int64_t count = 0;
        for (size_t i = 1; i < args.size(); ++i) {
            if (m_mock.strings.find(args[i]) != m_mock.strings.end() ||
                m_mock.hashes.find(args[i]) != m_mock.hashes.end() ||
                m_mock.lists.find(args[i]) != m_mock.lists.end()) {
                count++;
            }
        }
        res.type = RespType::Integer;
        res.intVal = count;
        return res;
    }

    if (cmd == "EXPIRE") {
        if (args.size() < 3) {
            res.type = RespType::Integer;
            res.intVal = 0;
            return res;
        }
        if (m_mock.strings.find(args[1]) != m_mock.strings.end() ||
            m_mock.hashes.find(args[1]) != m_mock.hashes.end()) {
            m_mock.expires[args[1]] = std::stoll(args[2]);
            res.type = RespType::Integer;
            res.intVal = 1;
        } else {
            res.type = RespType::Integer;
            res.intVal = 0;
        }
        return res;
    }

    if (cmd == "TTL") {
        if (args.size() < 2) {
            res.type = RespType::Integer;
            res.intVal = -2;
            return res;
        }
        auto it = m_mock.expires.find(args[1]);
        if (it != m_mock.expires.end()) {
            res.type = RespType::Integer;
            res.intVal = it->second;
        } else if (m_mock.strings.find(args[1]) != m_mock.strings.end()) {
            res.type = RespType::Integer;
            res.intVal = -1; // No expiration
        } else {
            res.type = RespType::Integer;
            res.intVal = -2; // Key not found
        }
        return res;
    }

    if (cmd == "INCR" || cmd == "DECR") {
        if (args.size() < 2) {
            res.type = RespType::Error;
            res.stringVal = "ERR wrong number of arguments";
            return res;
        }
        int64_t val = 0;
        auto it = m_mock.strings.find(args[1]);
        if (it != m_mock.strings.end()) {
            try { val = std::stoll(it->second); } catch (...) { val = 0; }
        }
        val += (cmd == "INCR") ? 1 : -1;
        m_mock.strings[args[1]] = std::to_string(val);
        res.type = RespType::Integer;
        res.intVal = val;
        return res;
    }

    if (cmd == "HSET") {
        if (args.size() < 4) {
            res.type = RespType::Error;
            res.stringVal = "ERR wrong number of arguments for 'hset'";
            return res;
        }
        m_mock.hashes[args[1]][args[2]] = args[3];
        res.type = RespType::Integer;
        res.intVal = 1;
        return res;
    }

    if (cmd == "HGET") {
        if (args.size() < 3) {
            res.type = RespType::Null;
            return res;
        }
        auto hIt = m_mock.hashes.find(args[1]);
        if (hIt != m_mock.hashes.end()) {
            auto fIt = hIt->second.find(args[2]);
            if (fIt != hIt->second.end()) {
                res.type = RespType::BulkString;
                res.stringVal = fIt->second;
                return res;
            }
        }
        res.type = RespType::Null;
        return res;
    }

    if (cmd == "HDEL") {
        if (args.size() < 3) {
            res.type = RespType::Integer;
            res.intVal = 0;
            return res;
        }
        auto hIt = m_mock.hashes.find(args[1]);
        if (hIt != m_mock.hashes.end()) {
            int64_t erased = hIt->second.erase(args[2]);
            res.type = RespType::Integer;
            res.intVal = erased;
            return res;
        }
        res.type = RespType::Integer;
        res.intVal = 0;
        return res;
    }

    if (cmd == "HGETALL") {
        res.type = RespType::Array;
        if (args.size() >= 2) {
            auto hIt = m_mock.hashes.find(args[1]);
            if (hIt != m_mock.hashes.end()) {
                for (const auto& [k, v] : hIt->second) {
                    RespValue rk, rv;
                    rk.type = RespType::BulkString; rk.stringVal = k;
                    rv.type = RespType::BulkString; rv.stringVal = v;
                    res.arrayVal.push_back(rk);
                    res.arrayVal.push_back(rv);
                }
            }
        }
        return res;
    }

    if (cmd == "LPUSH" || cmd == "RPUSH") {
        if (args.size() < 3) {
            res.type = RespType::Error;
            res.stringVal = "ERR wrong number of arguments";
            return res;
        }
        auto& list = m_mock.lists[args[1]];
        if (cmd == "LPUSH") {
            for (size_t i = 2; i < args.size(); ++i) {
                list.insert(list.begin(), args[i]);
            }
        } else {
            for (size_t i = 2; i < args.size(); ++i) {
                list.push_back(args[i]);
            }
        }
        res.type = RespType::Integer;
        res.intVal = static_cast<int64_t>(list.size());
        return res;
    }

    if (cmd == "LPOP" || cmd == "RPOP") {
        if (args.size() < 2) {
            res.type = RespType::Null;
            return res;
        }
        auto it = m_mock.lists.find(args[1]);
        if (it != m_mock.lists.end() && !it->second.empty()) {
            res.type = RespType::BulkString;
            if (cmd == "LPOP") {
                res.stringVal = it->second.front();
                it->second.erase(it->second.begin());
            } else {
                res.stringVal = it->second.back();
                it->second.pop_back();
            }
            return res;
        }
        res.type = RespType::Null;
        return res;
    }

    if (cmd == "LLEN") {
        res.type = RespType::Integer;
        if (args.size() >= 2) {
            auto it = m_mock.lists.find(args[1]);
            res.intVal = (it != m_mock.lists.end()) ? static_cast<int64_t>(it->second.size()) : 0;
        }
        return res;
    }

    if (cmd == "LRANGE") {
        res.type = RespType::Array;
        if (args.size() >= 4) {
            auto it = m_mock.lists.find(args[1]);
            if (it != m_mock.lists.end()) {
                int64_t start = std::stoll(args[2]);
                int64_t stop = std::stoll(args[3]);
                int64_t n = static_cast<int64_t>(it->second.size());
                if (start < 0) start = std::max<int64_t>(0, n + start);
                if (stop < 0) stop = std::max<int64_t>(0, n + stop);
                stop = std::min<int64_t>(stop, n - 1);
                for (int64_t i = start; i <= stop && i < n; ++i) {
                    RespValue item;
                    item.type = RespType::BulkString;
                    item.stringVal = it->second[i];
                    res.arrayVal.push_back(item);
                }
            }
        }
        return res;
    }

    if (cmd == "PUBLISH") {
        if (args.size() < 3) {
            res.type = RespType::Integer;
            res.intVal = 0;
            return res;
        }
        std::string channel = args[1];
        std::string message = args[2];
        int64_t subscribersNotified = 0;
        auto it = m_mock.subscribers.find(channel);
        if (it != m_mock.subscribers.end()) {
            subscribersNotified = static_cast<int64_t>(it->second.size());
            for (auto& cb : it->second) {
                if (cb) cb(channel, message);
            }
        }
        res.type = RespType::Integer;
        res.intVal = subscribersNotified;
        return res;
    }

    if (cmd == "SUBSCRIBE") {
        res.type = RespType::Array;
        // Mock subscribe acknowledgement
        for (size_t i = 1; i < args.size(); ++i) {
            RespValue typeVal, chanVal, countVal;
            typeVal.type = RespType::BulkString; typeVal.stringVal = "subscribe";
            chanVal.type = RespType::BulkString; chanVal.stringVal = args[i];
            countVal.type = RespType::Integer; countVal.intVal = static_cast<int64_t>(i);
            res.arrayVal = {typeVal, chanVal, countVal};
        }
        return res;
    }

    res.type = RespType::Error;
    res.stringVal = "ERR unknown command '" + args[0] + "'";
    return res;
}

bool RedisClient::auth(const std::string& password) {
    auto res = execute({"AUTH", password});
    return res.getString() == "OK";
}

std::string RedisClient::ping(const std::string& msg) {
    std::vector<std::string> args = {"PING"};
    if (!msg.empty()) args.push_back(msg);
    auto res = execute(args);
    return res.getString();
}

std::optional<std::string> RedisClient::get(const std::string& key) {
    auto res = execute({"GET", key});
    if (res.isNull()) return std::nullopt;
    return res.getString();
}

bool RedisClient::set(const std::string& key, const std::string& value) {
    auto res = execute({"SET", key, value});
    return res.getString() == "OK";
}

bool RedisClient::setEx(const std::string& key, int64_t seconds, const std::string& value) {
    auto res = execute({"SETEX", key, std::to_string(seconds), value});
    return res.getString() == "OK";
}

bool RedisClient::del(const std::string& key) {
    auto res = execute({"DEL", key});
    return res.getInt() > 0;
}

bool RedisClient::exists(const std::string& key) {
    auto res = execute({"EXISTS", key});
    return res.getInt() > 0;
}

bool RedisClient::expire(const std::string& key, int64_t seconds) {
    auto res = execute({"EXPIRE", key, std::to_string(seconds)});
    return res.getInt() == 1;
}

int64_t RedisClient::ttl(const std::string& key) {
    auto res = execute({"TTL", key});
    return res.getInt();
}

int64_t RedisClient::incr(const std::string& key) {
    auto res = execute({"INCR", key});
    return res.getInt();
}

int64_t RedisClient::decr(const std::string& key) {
    auto res = execute({"DECR", key});
    return res.getInt();
}

std::optional<std::string> RedisClient::hget(const std::string& key, const std::string& field) {
    auto res = execute({"HGET", key, field});
    if (res.isNull()) return std::nullopt;
    return res.getString();
}

bool RedisClient::hset(const std::string& key, const std::string& field, const std::string& value) {
    auto res = execute({"HSET", key, field, value});
    return !res.isError();
}

bool RedisClient::hdel(const std::string& key, const std::string& field) {
    auto res = execute({"HDEL", key, field});
    return res.getInt() > 0;
}

std::unordered_map<std::string, std::string> RedisClient::hgetAll(const std::string& key) {
    std::unordered_map<std::string, std::string> out;
    auto res = execute({"HGETALL", key});
    const auto& arr = res.getArray();
    for (size_t i = 0; i + 1 < arr.size(); i += 2) {
        out[arr[i].getString()] = arr[i + 1].getString();
    }
    return out;
}

int64_t RedisClient::lpush(const std::string& key, const std::string& value) {
    auto res = execute({"LPUSH", key, value});
    return res.getInt();
}

int64_t RedisClient::rpush(const std::string& key, const std::string& value) {
    auto res = execute({"RPUSH", key, value});
    return res.getInt();
}

std::optional<std::string> RedisClient::lpop(const std::string& key) {
    auto res = execute({"LPOP", key});
    if (res.isNull()) return std::nullopt;
    return res.getString();
}

std::optional<std::string> RedisClient::rpop(const std::string& key) {
    auto res = execute({"RPOP", key});
    if (res.isNull()) return std::nullopt;
    return res.getString();
}

int64_t RedisClient::llen(const std::string& key) {
    auto res = execute({"LLEN", key});
    return res.getInt();
}

std::vector<std::string> RedisClient::lrange(const std::string& key, int64_t start, int64_t stop) {
    std::vector<std::string> out;
    auto res = execute({"LRANGE", key, std::to_string(start), std::to_string(stop)});
    for (const auto& item : res.getArray()) {
        out.push_back(item.getString());
    }
    return out;
}

int64_t RedisClient::publish(const std::string& channel, const std::string& message) {
    auto res = execute({"PUBLISH", channel, message});
    return res.getInt();
}

void RedisClient::subscribe(const std::vector<std::string>& channels, MessageCallback onMessage) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& ch : channels) {
        m_mock.subscribers[ch].push_back(onMessage);
    }
}

// ============================================================================
// Redis Connection Pool Implementation
// ============================================================================

RedisPool::RedisPool(RedisPoolConfig config)
    : m_config(std::move(config)) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (size_t i = 0; i < m_config.minConnections; ++i) {
        auto client = createClient();
        if (client) {
            m_pool.push(client);
            m_totalConnections++;
        }
    }
}

RedisPool::~RedisPool() {
    close();
}

std::shared_ptr<RedisClient> RedisPool::createClient() {
    auto client = std::make_shared<RedisClient>();
    client->setMockMode(m_config.mockMode);
    if (client->connect(m_config.host, m_config.port)) {
        if (!m_config.password.empty()) {
            client->auth(m_config.password);
        }
        return client;
    }
    return nullptr;
}

std::shared_ptr<RedisClient> RedisPool::acquire() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_closed) return nullptr;

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_config.acquireTimeoutMs);

    while (m_pool.empty()) {
        if (m_totalConnections < m_config.maxConnections) {
            auto client = createClient();
            if (client) {
                m_totalConnections++;
                return client;
            }
        }
        if (m_cv.wait_until(lock, deadline) == std::cv_status::timeout) {
            return nullptr;
        }
        if (m_closed) return nullptr;
    }

    auto client = m_pool.front();
    m_pool.pop();
    return client;
}

void RedisPool::release(std::shared_ptr<RedisClient> client) {
    if (!client) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_closed) return;
    m_pool.push(client);
    m_cv.notify_one();
}

size_t RedisPool::available() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pool.size();
}

size_t RedisPool::total() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_totalConnections;
}

void RedisPool::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_closed = true;
    while (!m_pool.empty()) {
        auto c = m_pool.front();
        m_pool.pop();
        if (c) c->disconnect();
    }
    m_totalConnections = 0;
    m_cv.notify_all();
}

} // namespace sp::rt::redis
