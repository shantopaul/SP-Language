#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <functional>

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
    explicit JsonValue(std::string_view valView) : m_type(JsonType::String), m_viewVal(valView) {}
    explicit JsonValue(const char* val) : m_type(JsonType::String), m_strVal(val ? val : "") {}
    explicit JsonValue(JsonType type) : m_type(type) {}

    static JsonValuePtr makeNull();
    static JsonValuePtr makeBool(bool val);
    static JsonValuePtr makeInt(int64_t val);
    static JsonValuePtr makeDouble(double val) { return std::make_shared<JsonValue>(val); }
    static JsonValuePtr makeString(std::string val) { return std::make_shared<JsonValue>(std::move(val)); }
    static JsonValuePtr makeStringView(std::string_view val) { return std::make_shared<JsonValue>(val); }
    static JsonValuePtr makeArray() { return std::make_shared<JsonValue>(JsonType::Array); }
    static JsonValuePtr makeObject() { return std::make_shared<JsonValue>(JsonType::Object); }

    void reserveArray(size_t n) { m_arrayItems.reserve(n); }
    void reserveMembers(size_t n) { m_members.reserve(n); }
    void appendMember(std::string key, JsonValuePtr value);

    [[nodiscard]] JsonType getType() const noexcept { return m_type; }
    [[nodiscard]] bool isNull() const noexcept { return m_type == JsonType::Null; }
    [[nodiscard]] bool isBool() const noexcept { return m_type == JsonType::Bool; }
    [[nodiscard]] bool isInt() const noexcept { return m_type == JsonType::Int; }
    [[nodiscard]] bool isDouble() const noexcept { return m_type == JsonType::Double; }
    [[nodiscard]] bool isNumber() const noexcept { return m_type == JsonType::Int || m_type == JsonType::Double; }
    [[nodiscard]] bool isString() const noexcept { return m_type == JsonType::String; }
    [[nodiscard]] bool isArray() const noexcept { return m_type == JsonType::Array; }
    [[nodiscard]] bool isObject() const noexcept { return m_type == JsonType::Object; }

    [[nodiscard]] bool asBool() const noexcept { return m_boolVal; }
    [[nodiscard]] int64_t asInt() const noexcept { return m_type == JsonType::Double ? static_cast<int64_t>(m_doubleVal) : m_intVal; }
    [[nodiscard]] double asDouble() const noexcept { return m_type == JsonType::Int ? static_cast<double>(m_intVal) : m_doubleVal; }
    
    // Zero-copy string view access (borrowed from payload buffer)
    [[nodiscard]] std::string_view asStringView() const noexcept {
        if (!m_strVal.empty()) return m_strVal;
        return m_viewVal;
    }

    // Materialized owned string access
    const std::string& asString() const {
        if (m_strVal.empty() && !m_viewVal.empty()) {
            const_cast<JsonValue*>(this)->m_strVal = std::string(m_viewVal);
        }
        return m_strVal;
    }

    // Array operations
    [[nodiscard]] size_t size() const;
    void add(JsonValuePtr item);
    [[nodiscard]] JsonValuePtr get(size_t index) const;

    // Object operations
    void set(std::string_view key, JsonValuePtr value);
    [[nodiscard]] JsonValuePtr get(std::string_view key) const;
    [[nodiscard]] bool has(std::string_view key) const;
    [[nodiscard]] std::vector<std::string> getKeys() const;
    [[nodiscard]] const std::vector<std::pair<std::string, JsonValuePtr>>& getMembers() const noexcept { return m_members; }
    [[nodiscard]] const std::vector<JsonValuePtr>& getArrayItems() const noexcept { return m_arrayItems; }

    // Serialization / Stringify
    [[nodiscard]] std::string stringify(int indent = 0) const;

private:
    JsonType m_type{JsonType::Null};
    bool m_boolVal{false};
    int64_t m_intVal{0};
    double m_doubleVal{0.0};
    std::string m_strVal;
    std::string_view m_viewVal;
    std::vector<JsonValuePtr> m_arrayItems;
    std::vector<std::pair<std::string, JsonValuePtr>> m_members;
};

// SIMD-accelerated / Zero-copy parser & fast validator
JsonValuePtr parse(std::string_view jsonStr, std::string* outError = nullptr);
bool validate(std::string_view jsonStr, std::string* outError = nullptr);

// ----------------------------------------------------------------------------
// Typed Class / Struct Serialization Metadata & Reflection (SP 2.4)
// ----------------------------------------------------------------------------

enum class FieldTypeKind {
    Int,
    Int64,
    Double,
    Float,
    Bool,
    String,
    Object,
    Array
};

struct FieldMetadata {
    std::string fieldName;   // SP field name, e.g. "userId"
    std::string jsonKey;     // Serialized JSON key, e.g. "user_id" (custom or identical to fieldName)
    FieldTypeKind typeKind{FieldTypeKind::String};
    std::string typeName;    // e.g. "int", "string", "User"
    uint32_t byteOffset{0};  // Byte offset in object layout
    bool isIgnored{false};   // true if marked with @json_ignore
    bool isNullable{false};  // true if nullable (e.g. string?)
};

struct ClassMetadata {
    std::string className;
    uint32_t objectSize{0};
    bool hasVTable{true};
    std::vector<FieldMetadata> fields;

    [[nodiscard]] const FieldMetadata* findFieldByJsonKey(std::string_view key) const;
    [[nodiscard]] const FieldMetadata* findFieldByName(std::string_view name) const;
};

class ClassMetadataRegistry {
public:
    static ClassMetadataRegistry& instance();

    void registerClass(ClassMetadata meta);
    [[nodiscard]] const ClassMetadata* findClass(std::string_view className) const;
    [[nodiscard]] bool hasClass(std::string_view className) const;
    void clear();

private:
    std::unordered_map<std::string, ClassMetadata> m_classes;
};

// Generic / Typed Decoding and Encoding Functions
void* decodeTypedObject(const std::string& className, std::string_view jsonStr, std::string* outError = nullptr);
std::string encodeTypedObject(const std::string& className, const void* objPtr, int indent = 0);

// ----------------------------------------------------------------------------
// RFC 7807 Problem Details for Request Validation (SP 2.4)
// ----------------------------------------------------------------------------

struct ValidationErrorItem {
    std::string field;
    std::string message;
};

struct ProblemDetails {
    std::string type{"https://api.splang.org/errors/validation"};
    std::string title{"Unprocessable Entity"};
    int status{422};
    std::string detail{"One or more validation errors occurred."};
    std::string instance;
    std::vector<ValidationErrorItem> errors;

    [[nodiscard]] std::string toJsonString(int indent = 0) const;
};

} // namespace sp::rt::json
