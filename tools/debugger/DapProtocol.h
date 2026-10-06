#pragma once

#include "spls/Json.h"
#include <string>
#include <vector>

namespace sp::dap {

using JsonValue = sp::lsp::JsonValue;

struct Source {
    std::string name;
    std::string path;

    JsonValue toJson() const {
        JsonValue j = JsonValue::object();
        if (!name.empty()) j["name"] = name;
        if (!path.empty()) j["path"] = path;
        return j;
    }

    static Source fromJson(const JsonValue& j) {
        Source s;
        s.name = j["name"].asString("");
        s.path = j["path"].asString("");
        return s;
    }
};

struct Breakpoint {
    int id = 0;
    bool verified = true;
    int line = 0;
    Source source;

    JsonValue toJson() const {
        JsonValue j = JsonValue::object();
        j["id"] = id;
        j["verified"] = verified;
        j["line"] = line;
        j["source"] = source.toJson();
        return j;
    }
};

struct StackFrame {
    int id = 0;
    std::string name;
    Source source;
    int line = 0;
    int column = 0;

    JsonValue toJson() const {
        JsonValue j = JsonValue::object();
        j["id"] = id;
        j["name"] = name;
        j["source"] = source.toJson();
        j["line"] = line;
        j["column"] = column;
        return j;
    }
};

struct Scope {
    std::string name;
    int variablesReference = 0;
    bool expensive = false;

    JsonValue toJson() const {
        JsonValue j = JsonValue::object();
        j["name"] = name;
        j["variablesReference"] = variablesReference;
        j["expensive"] = expensive;
        return j;
    }
};

struct Variable {
    std::string name;
    std::string value;
    std::string type;
    int variablesReference = 0;

    JsonValue toJson() const {
        JsonValue j = JsonValue::object();
        j["name"] = name;
        j["value"] = value;
        j["type"] = type;
        j["variablesReference"] = variablesReference;
        return j;
    }
};

struct Thread {
    int id = 1;
    std::string name = "main";

    JsonValue toJson() const {
        JsonValue j = JsonValue::object();
        j["id"] = id;
        j["name"] = name;
        return j;
    }
};

} // namespace sp::dap
