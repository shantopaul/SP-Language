module util.validation;

public class ValidationUtil {
    public static fn isDigit(char ch) -> bool {
        return ch == '0' || ch == '1' || ch == '2' || ch == '3' || ch == '4'
            || ch == '5' || ch == '6' || ch == '7' || ch == '8' || ch == '9';
    }

    public static fn isHexChar(char ch) -> bool {
        if (ValidationUtil.isDigit(ch)) {
            return true;
        }
        return ch == 'a' || ch == 'b' || ch == 'c' || ch == 'd' || ch == 'e' || ch == 'f'
            || ch == 'A' || ch == 'B' || ch == 'C' || ch == 'D' || ch == 'E' || ch == 'F';
    }

    public static fn isWhitespaceChar(char ch) -> bool {
        return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
    }

    public static fn isValidUuid(string raw) -> bool {
        let s = raw.trim();
        if (s.length() != 36) {
            return false;
        }
        let i = 0;
        while (i < 36) {
            let ch = s.charAt(i);
            if (i == 8 || i == 13 || i == 18 || i == 23) {
                if (ch != '-') {
                    return false;
                }
            } else {
                if (!ValidationUtil.isHexChar(ch)) {
                    return false;
                }
            }
            i = i + 1;
        }
        return true;
    }

    public static fn isValidEmail(string raw) -> bool {
        let email = raw.trim();
        let n = email.length();
        if (n < 5 || n > 254) {
            return false;
        }
        let atIdx = -1;
        let atCount = 0;
        let i = 0;
        while (i < n) {
            let ch = email.charAt(i);
            if (ValidationUtil.isWhitespaceChar(ch)) {
                return false;
            }
            if (ch == '@') {
                atIdx = i;
                atCount = atCount + 1;
            }
            i = i + 1;
        }
        if (atCount != 1 || atIdx <= 0 || atIdx >= n - 3) {
            return false;
        }
        let domain = email.substring(atIdx + 1, n);
        if (domain.startsWith(".") || domain.endsWith(".")) {
            return false;
        }
        return domain.contains(".");
    }

    public static fn isValidStatus(string status) -> bool {
        return status == "todo"
            || status == "in_progress"
            || status == "done"
            || status == "archived";
    }

    public static fn isValidPriority(string priority) -> bool {
        return priority == "low"
            || priority == "medium"
            || priority == "high"
            || priority == "urgent";
    }

    public static fn isValidHexColor(string raw) -> bool {
        let color = raw.trim();
        if (color.length() != 7) {
            return false;
        }
        if (color.charAt(0) != '#') {
            return false;
        }
        let i = 1;
        while (i < 7) {
            if (!ValidationUtil.isHexChar(color.charAt(i))) {
                return false;
            }
            i = i + 1;
        }
        return true;
    }

    /// Validate ISO-8601 date prefix YYYY-MM-DD (optionally followed by Thh:mm...).
    public static fn isValidIsoDate(string raw) -> bool {
        let s = raw.trim();
        let n = s.length();
        if (n < 10 || n > 40) {
            return false;
        }
        // Check YYYY-MM-DD
        if (!ValidationUtil.isDigit(s.charAt(0))
            || !ValidationUtil.isDigit(s.charAt(1))
            || !ValidationUtil.isDigit(s.charAt(2))
            || !ValidationUtil.isDigit(s.charAt(3))
            || s.charAt(4) != '-'
            || !ValidationUtil.isDigit(s.charAt(5))
            || !ValidationUtil.isDigit(s.charAt(6))
            || s.charAt(7) != '-'
            || !ValidationUtil.isDigit(s.charAt(8))
            || !ValidationUtil.isDigit(s.charAt(9))) {
            return false;
        }
        // Check month 01..12 and day 01..31 roughly using exact digit characters
        let m1 = s.charAt(5);
        let m2 = s.charAt(6);
        if (m1 != '0' && m1 != '1') {
            return false;
        }
        if (m1 == '0' && m2 == '0') {
            return false;
        }
        if (m1 == '1' && m2 != '0' && m2 != '1' && m2 != '2') {
            return false;
        }
        let d1 = s.charAt(8);
        let d2 = s.charAt(9);
        if (d1 != '0' && d1 != '1' && d1 != '2' && d1 != '3') {
            return false;
        }
        if (d1 == '0' && d2 == '0') {
            return false;
        }
        if (d1 == '3' && d2 != '0' && d2 != '1') {
            return false;
        }
        if (n == 10) {
            return true;
        }
        let sep = s.charAt(10);
        if (sep != 'T' && sep != ' ') {
            return false;
        }
        // Validate remaining characters are date/time safe characters only
        let i = 11;
        while (i < n) {
            let ch = s.charAt(i);
            let ok = ValidationUtil.isDigit(ch)
                || ch == ':' || ch == '.' || ch == 'Z' || ch == '+' || ch == '-';
            if (!ok) {
                return false;
            }
            i = i + 1;
        }
        return n >= 16;
    }

    public static fn isValidBulkAction(string action) -> bool {
        return action == "complete"
            || action == "reopen"
            || action == "archive"
            || action == "delete"
            || action == "set_priority";
    }
}
