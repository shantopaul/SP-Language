module models.item;

import std.string;

// ============================================================================
// Data Model & JSON Sanitization Helpers
// ============================================================================

public fn escapeJson(string input) -> string {
    let out = "";
    let len = input.length();
    for (let i = 0; i < len; i = i + 1) {
        let ch = input.charAt(i);
        if (ch == '\"') {
            out = out + "\\\"";
        } else if (ch == '\\') {
            out = out + "\\\\";
        } else if (ch == '\n') {
            out = out + "\\n";
        } else if (ch == '\r') {
            out = out + "\\r";
        } else if (ch == '\t') {
            out = out + "\\t";
        } else {
            out = out + ch;
        }
    }
    return out;
}

public class ItemModel {
    public static fn serialize(string id, string title, string desc, string status, string priority, string createdAt) -> string {
        return "{\"id\":" + id + ",\"title\":\"" + escapeJson(title) + "\",\"description\":\"" + escapeJson(desc) + "\",\"status\":\"" + escapeJson(status) + "\",\"priority\":\"" + escapeJson(priority) + "\",\"created_at\":\"" + escapeJson(createdAt) + "\"}";
    }
}
