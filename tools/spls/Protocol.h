#pragma once

#include "Json.h"
#include <string>
#include <vector>

namespace sp::lsp {

struct Position {
    int line = 0;
    int character = 0;

    JsonValue toJson() const {
        JsonValue j = JsonValue::object();
        j["line"] = line;
        j["character"] = character;
        return j;
    }

    static Position fromJson(const JsonValue& j) {
        Position pos;
        pos.line = static_cast<int>(j["line"].asInt(0));
        pos.character = static_cast<int>(j["character"].asInt(0));
        return pos;
    }
};

struct Range {
    Position start;
    Position end;

    JsonValue toJson() const {
        JsonValue j = JsonValue::object();
        j["start"] = start.toJson();
        j["end"] = end.toJson();
        return j;
    }

    static Range fromJson(const JsonValue& j) {
        Range r;
        r.start = Position::fromJson(j["start"]);
        r.end = Position::fromJson(j["end"]);
        return r;
    }
};

struct Location {
    std::string uri;
    Range range;

    JsonValue toJson() const {
        JsonValue j = JsonValue::object();
        j["uri"] = uri;
        j["range"] = range.toJson();
        return j;
    }
};

struct TextEdit {
    Range range;
    std::string newText;

    JsonValue toJson() const {
        JsonValue j = JsonValue::object();
        j["range"] = range.toJson();
        j["newText"] = newText;
        return j;
    }
};

struct Diagnostic {
    Range range;
    int severity = 1; // 1 = Error, 2 = Warning, 3 = Info, 4 = Hint
    std::string code;
    std::string source = "sp";
    std::string message;

    JsonValue toJson() const {
        JsonValue j = JsonValue::object();
        j["range"] = range.toJson();
        j["severity"] = severity;
        if (!code.empty()) j["code"] = code;
        j["source"] = source;
        j["message"] = message;
        return j;
    }
};

struct CompletionItem {
    std::string label;
    int kind = 1; // 1 = Text, 2 = Method, 3 = Function, 6 = Variable, 7 = Class, 8 = Interface, 14 = Keyword, 25 = TypeParameter
    std::string detail;
    std::string documentation;
    std::string insertText;

    JsonValue toJson() const {
        JsonValue j = JsonValue::object();
        j["label"] = label;
        j["kind"] = kind;
        if (!detail.empty()) j["detail"] = detail;
        if (!documentation.empty()) {
            JsonValue doc = JsonValue::object();
            doc["kind"] = "markdown";
            doc["value"] = documentation;
            j["documentation"] = doc;
        }
        if (!insertText.empty()) j["insertText"] = insertText;
        return j;
    }
};

struct Hover {
    std::string contents; // markdown
    Range range;
    bool hasRange = false;

    JsonValue toJson() const {
        JsonValue j = JsonValue::object();
        JsonValue cont = JsonValue::object();
        cont["kind"] = "markdown";
        cont["value"] = contents;
        j["contents"] = cont;
        if (hasRange) j["range"] = range.toJson();
        return j;
    }
};

} // namespace sp::lsp
