module util.json;

import std.json;

public class JsonUtil {
    /// Escape a string so it is safe to embed inside a JSON string literal.
    public static fn escape(string raw) -> string {
        let out = "";
        let n = raw.length();
        let i = 0;
        while (i < n) {
            let ch = raw.charAt(i);
            if (ch == '\\') {
                out = out + "\\\\";
            } else if (ch == '"') {
                out = out + "\\\"";
            } else if (ch == '\n') {
                out = out + "\\n";
            } else if (ch == '\r') {
                out = out + "\\r";
            } else if (ch == '\t') {
                out = out + "\\t";
            } else {
                out = out + raw.substring(i, i + 1);
            }
            i = i + 1;
        }
        return out;
    }

    /// Wrap and escape a string as a JSON string literal.
    public static fn quote(string raw) -> string {
        return "\"" + JsonUtil.escape(raw) + "\"";
    }

    /// Return "null" when empty, otherwise a quoted JSON string.
    public static fn nullableString(string raw) -> string {
        if (raw.length() == 0) {
            return "null";
        }
        return JsonUtil.quote(raw);
    }

    public static fn boolLiteral(bool val) -> string {
        if (val) {
            return "true";
        }
        return "false";
    }

    public static fn intLiteral(int64 val) -> string {
        return "${val}";
    }

    public static fn hasField(JsonValue obj, string key) -> bool {
        if (!obj.isValid()) {
            return false;
        }
        return obj.has(key);
    }

    public static fn isNullField(JsonValue obj, string key) -> bool {
        if (!obj.isValid() || !obj.has(key)) {
            return false;
        }
        let child = obj.get(key);
        let s = child.stringify(0).trim();
        return s == "null";
    }

    public static fn getString(JsonValue obj, string key, string fallback = "") -> string {
        if (!obj.isValid() || !obj.has(key)) {
            return fallback;
        }
        let child = obj.get(key);
        if (!child.isValid()) {
            return fallback;
        }
        let raw = child.stringify(0).trim();
        if (raw == "null") {
            return fallback;
        }
        return child.asString();
    }

    public static fn getInt(JsonValue obj, string key, int64 fallback = 0) -> int64 {
        if (!obj.isValid() || !obj.has(key)) {
            return fallback;
        }
        let child = obj.get(key);
        if (!child.isValid()) {
            return fallback;
        }
        let raw = child.stringify(0).trim();
        if (raw == "null" || raw.length() == 0) {
            return fallback;
        }
        return child.asInt();
    }

    public static fn getBool(JsonValue obj, string key, bool fallback = false) -> bool {
        if (!obj.isValid() || !obj.has(key)) {
            return fallback;
        }
        let child = obj.get(key);
        if (!child.isValid()) {
            return fallback;
        }
        let raw = child.stringify(0).trim();
        if (raw == "true") {
            return true;
        }
        if (raw == "false") {
            return false;
        }
        return fallback;
    }

    /// Read a JSON array of non-empty strings from a field.
    public static fn getStringArray(JsonValue obj, string key) -> List<string> {
        let items = new List<string>();
        if (!obj.isValid() || !obj.has(key)) {
            return items;
        }
        let arr = obj.get(key);
        if (!arr.isValid()) {
            return items;
        }
        let count = arr.size();
        let i = 0;
        while (i < count) {
            let elem = arr.at(i);
            if (elem.isValid()) {
                let s = elem.asString().trim();
                if (s.length() > 0) {
                    items.push(s);
                }
            }
            i = i + 1;
        }
        return items;
    }
}
