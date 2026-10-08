#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <string_view>

namespace sp::rt::json {

enum class JsonType {
    Null = 0,
    Bool,
    Int,
    Double,
    String,
    Array,
    Object
};

class JsonValue;
using JsonValuePtr = std::shared_ptr<JsonValue>;

class JsonValue {
public:
    JsonValue() : m_type(JsonType::Null) {}
    explicit JsonValue(bool val) : m_type(JsonType::Bool), m_boolVal(val) {}
    explicit JsonValue(int64_t val) : m_type(JsonType::Int), m_intVal(val) {}
    explicit JsonValue(double val) : m_type(JsonType::Double), m_doubleVal(val) {}
    explicit JsonValue(std::string val) : m_type(JsonType::String), m_strVal(std::move(val)) {}
    explicit JsonValue(const char* val) : m_type(JsonType::String), m_strVal(val ? val : "") {}
    explicit JsonValue(JsonType type) : m_type(type) {}

    static JsonValuePtr makeNull() { return std::make_shared<JsonValue>(JsonType::Null); }
    static JsonValuePtr makeBool(bool val) { return std::make_shared<JsonValue>(val); }
    static JsonValuePtr makeInt(int64_t val) { return std::make_shared<JsonValue>(val); }
    static JsonValuePtr makeDouble(double val) { return std::make_shared<JsonValue>(val); }
    static JsonValuePtr makeString(std::string val) { return std::make_shared<JsonValue>(std::move(val)); }
    static JsonValuePtr makeArray() { return std::make_shared<JsonValue>(JsonType::Array); }
    static JsonValuePtr makeObject() { return std::make_shared<JsonValue>(JsonType::Object); }

    JsonType getType() const { return m_type; }
    bool isNull() const { return m_type == JsonType::Null; }
    bool isBool() const { return m_type == JsonType::Bool; }
    bool isInt() const { return m_type == JsonType::Int; }
    bool isDouble() const { return m_type == JsonType::Double; }
    bool isNumber() const { return m_type == JsonType::Int || m_type == JsonType::Double; }
    bool isString() const { return m_type == JsonType::String; }
    bool isArray() const { return m_type == JsonType::Array; }
    bool isObject() const { return m_type == JsonType::Object; }

    bool asBool() const { return m_boolVal; }
    int64_t asInt() const { return m_type == JsonType::Double ? static_cast<int64_t>(m_doubleVal) : m_intVal; }
    double asDouble() const { return m_type == JsonType::Int ? static_cast<double>(m_intVal) : m_doubleVal; }
    const std::string& asString() const { return m_strVal; }

    // Array operations
    size_t size() const;
    void add(JsonValuePtr item);
    JsonValuePtr get(size_t index) const;

    // Object operations
    void set(const std::string& key, JsonValuePtr value);
    JsonValuePtr get(const std::string& key) const;
    bool has(const std::string& key) const;
    std::vector<std::string> getKeys() const;
    const std::vector<std::pair<std::string, JsonValuePtr>>& getMembers() const { return m_members; }
    const std::vector<JsonValuePtr>& getArrayItems() const { return m_arrayItems; }

    // Stringify
    std::string stringify(int indent = 0) const;

private:
    JsonType m_type{JsonType::Null};
    bool m_boolVal{false};
    int64_t m_intVal{0};
    double m_doubleVal{0.0};
    std::string m_strVal;
    std::vector<JsonValuePtr> m_arrayItems;
    std::vector<std::pair<std::string, JsonValuePtr>> m_members;
};

// Parser
JsonValuePtr parse(std::string_view jsonStr, std::string* outError = nullptr);

} // namespace sp::rt::json
