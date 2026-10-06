module std.string;

extern "C" {
    fn sp_rt_str_len(string s) -> int;
    fn sp_rt_str_upper(string s) -> string;
    fn sp_rt_str_lower(string s) -> string;
    fn sp_rt_str_trim(string s) -> string;
    fn sp_rt_str_contains(string s, string sub) -> bool;
    fn sp_rt_str_starts_with(string s, string prefix) -> bool;
    fn sp_rt_str_ends_with(string s, string suffix) -> bool;
    fn sp_rt_str_index_of(string s, string sub) -> int;
    fn sp_rt_str_substring(string s, int start, int end) -> string;
    fn sp_rt_str_replace(string s, string from, string to) -> string;
    fn sp_rt_str_split(string s, string delim) -> string[];
    fn sp_rt_str_reverse(string s) -> string;
    fn sp_rt_str_repeat(string s, int count) -> string;
    fn sp_rt_str_is_empty(string s) -> bool;
}

public fn length(string s) -> int {
    return sp_rt_str_len(s);
}

public fn toUpper(string s) -> string {
    return sp_rt_str_upper(s);
}

public fn toLower(string s) -> string {
    return sp_rt_str_lower(s);
}

public fn trim(string s) -> string {
    return sp_rt_str_trim(s);
}

public fn contains(string s, string sub) -> bool {
    return sp_rt_str_contains(s, sub);
}

public fn startsWith(string s, string prefix) -> bool {
    return sp_rt_str_starts_with(s, prefix);
}

public fn endsWith(string s, string suffix) -> bool {
    return sp_rt_str_ends_with(s, suffix);
}

public fn indexOf(string s, string sub) -> int {
    return sp_rt_str_index_of(s, sub);
}

public fn substring(string s, int start, int end) -> string {
    return sp_rt_str_substring(s, start, end);
}

public fn replace(string s, string from, string to) -> string {
    return sp_rt_str_replace(s, from, to);
}

public fn split(string s, string delim) -> string[] {
    return sp_rt_str_split(s, delim);
}

public fn reverse(string s) -> string {
    return sp_rt_str_reverse(s);
}

public fn repeat(string s, int count) -> string {
    return sp_rt_str_repeat(s, count);
}

public fn isEmpty(string s) -> bool {
    return sp_rt_str_is_empty(s);
}

public class String {
    public static fn length(string s) -> int {
        return sp_rt_str_len(s);
    }

    public static fn upper(string s) -> string {
        return sp_rt_str_upper(s);
    }

    public static fn lower(string s) -> string {
        return sp_rt_str_lower(s);
    }

    public static fn trim(string s) -> string {
        return sp_rt_str_trim(s);
    }

    public static fn contains(string s, string sub) -> bool {
        return sp_rt_str_contains(s, sub);
    }

    public static fn indexOf(string s, string sub) -> int {
        return sp_rt_str_index_of(s, sub);
    }

    public static fn replace(string s, string from, string to) -> string {
        return sp_rt_str_replace(s, from, to);
    }

    public static fn substring(string s, int start, int end) -> string {
        return sp_rt_str_substring(s, start, end);
    }

    public static fn split(string s, string delim) -> string[] {
        return sp_rt_str_split(s, delim);
    }

    public static fn reverse(string s) -> string {
        return sp_rt_str_reverse(s);
    }
}
