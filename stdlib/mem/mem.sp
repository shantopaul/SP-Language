module std.mem;

// Runtime memory management and arena allocator bindings
extern "C" {
    fn sp_rt_arena_create(int initialCapacity) -> int;
    fn sp_rt_arena_alloc(int arena, int size) -> int;
    fn sp_rt_arena_reset(int arena);
    fn sp_rt_arena_destroy(int arena);
    fn sp_rt_arena_allocated_bytes(int arena) -> int;
    fn sp_rt_mem_allocated_bytes() -> int;
    fn sp_rt_mem_live_objects() -> int;
}

/**
 * High-performance regional memory arena for request-scoped batch allocations.
 * Enables O(1) bulk allocation and fast bulk reset without individual free overhead.
 */
public class Arena {
    private int handle;

    public Arena() {
        this.handle = sp_rt_arena_create(65536);
    }

    public Arena(int initialCapacity) {
        this.handle = sp_rt_arena_create(initialCapacity);
    }

    public fn alloc(int size) -> int {
        return sp_rt_arena_alloc(this.handle, size);
    }

    public fn reset() {
        sp_rt_arena_reset(this.handle);
    }

    public fn destroy() {
        if (this.handle != 0) {
            sp_rt_arena_destroy(this.handle);
            this.handle = 0;
        }
    }

    public fn allocatedBytes() -> int {
        return sp_rt_arena_allocated_bytes(this.handle);
    }
}

/**
 * Returns total currently tracked heap memory allocated across the SP runtime in bytes.
 */
public fn totalAllocatedBytes() -> int {
    return sp_rt_mem_allocated_bytes();
}

/**
 * Returns total count of live reference-counted objects currently managed by the SP runtime.
 */
public fn totalLiveObjects() -> int {
    return sp_rt_mem_live_objects();
}
