module std.collections;

// Standard library collections module providing List, Map, and Set utilities.

public fn isListEmpty(List<int> list) -> bool {
    return list.isEmpty();
}

public fn isMapEmpty(Map<string, int> map) -> bool {
    return map.isEmpty();
}

public fn isSetEmpty(Set<int> set) -> bool {
    return set.isEmpty();
}
