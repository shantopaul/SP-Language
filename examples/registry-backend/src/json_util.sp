module json_util;

// ============================================================================
// Small JSON and input-validation helpers for the registry service.
// No regex exists in the SP stdlib, so validation is done with explicit
// character-equality tests. NOTE: do NOT use `"...".indexOf(ch)` with a
// char here — the compiler lowers a char argument to a string method as a
// raw pointer value, which segfaults (see docs/v2/PHASE-31-PRO-4.0.0.md,
// parent fix: checkStringArgs). Char range comparisons (`>=`) are also
// rejected by sema (E1001: operands must be numeric), so membership is
// spelled out with `==` chains — the only char operation sema accepts
// besides `==`/`!=`.
fn isDigit(char c) -> bool {
    return c == '0' || c == '1' || c == '2' || c == '3' || c == '4' ||
           c == '5' || c == '6' || c == '7' || c == '8' || c == '9';
}

fn isLower(char c) -> bool {
    return c == 'a' || c == 'b' || c == 'c' || c == 'd' || c == 'e' ||
           c == 'f' || c == 'g' || c == 'h' || c == 'i' || c == 'j' ||
           c == 'k' || c == 'l' || c == 'm' || c == 'n' || c == 'o' ||
           c == 'p' || c == 'q' || c == 'r' || c == 's' || c == 't' ||
           c == 'u' || c == 'v' || c == 'w' || c == 'x' || c == 'y' ||
           c == 'z';
}

fn isUpper(char c) -> bool {
    return c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'E' ||
           c == 'F' || c == 'G' || c == 'H' || c == 'I' || c == 'J' ||
           c == 'K' || c == 'L' || c == 'M' || c == 'N' || c == 'O' ||
           c == 'P' || c == 'Q' || c == 'R' || c == 'S' || c == 'T' ||
           c == 'U' || c == 'V' || c == 'W' || c == 'X' || c == 'Y' ||
           c == 'Z';
}

fn isAlpha(char c) -> bool {
    return isLower(c) || isUpper(c);
}

fn isAlphaNum(char c) -> bool {
    return isDigit(c) || isAlpha(c);
}

fn isHex(char c) -> bool {
    return isDigit(c) ||
           c == 'a' || c == 'b' || c == 'c' || c == 'd' || c == 'e' || c == 'f' ||
           c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'E' || c == 'F';
}

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

// Package names: 2-64 chars, [a-z0-9_-], must start with [a-z0-9].
// Same rule as spm's isValidPackageName and the Next.js registry route.
public fn isValidPackageName(string name) -> bool {
    let len = name.length();
    if (len < 2 || len > 64) {
        return false;
    }
    let first = name.charAt(0);
    if (!isLower(first) && !isDigit(first)) {
        return false;
    }
    for (let i = 0; i < len; i = i + 1) {
        let ch = name.charAt(i);
        if (!isLower(ch) && !isDigit(ch) && ch != '-' && ch != '_') {
            return false;
        }
    }
    return true;
}

// Simplified semver check: 1-32 chars from [0-9A-Za-z.-], must contain at
// least one digit and one dot (so "1.0.0" passes, "latest" does not).
// This is intentionally stricter than "any string" but simpler than a full
// semver grammar; the exact rule is documented in openapi.yaml.
public fn isValidVersion(string version) -> bool {
    let len = version.length();
    if (len < 1 || len > 32) {
        return false;
    }
    let hasDigit = false;
    let hasDot = false;
    for (let i = 0; i < len; i = i + 1) {
        let ch = version.charAt(i);
        if (!isAlphaNum(ch) && ch != '.' && ch != '-') {
            return false;
        }
        if (isDigit(ch)) {
            hasDigit = true;
        }
        if (ch == '.') {
            hasDot = true;
        }
    }
    return hasDigit && hasDot;
}

// A SHA-256 digest is exactly 64 hexadecimal characters.
public fn isValidSha256(string checksum) -> bool {
    if (checksum.length() != 64) {
        return false;
    }
    for (let i = 0; i < 64; i = i + 1) {
        let ch = checksum.charAt(i);
        if (!isHex(ch)) {
            return false;
        }
    }
    return true;
}

// Base64 alphabet check (standard, with padding). Used before decoding an
// uploaded tarball so garbage input fails fast with a clear 400.
public fn isValidBase64(string input) -> bool {
    let len = input.length();
    if (len == 0 || len % 4 != 0) {
        return false;
    }
    for (let i = 0; i < len; i = i + 1) {
        let ch = input.charAt(i);
        if (!isAlphaNum(ch) && ch != '+' && ch != '/' && ch != '=') {
            return false;
        }
    }
    return true;
}
