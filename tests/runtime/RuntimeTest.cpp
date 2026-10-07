#include "sp/runtime/Runtime.h"
#include <gtest/gtest.h>

TEST(RuntimeTest, ArrayAllocationAndElementAccess) {
    void* arr = sp_rt_array_alloc(5, sizeof(int64_t));
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(sp_rt_array_length(arr), 5);

    sp_rt_array_set_i64(arr, 0, 100);
    sp_rt_array_set_i64(arr, 1, 200);
    sp_rt_array_set_i64(arr, 4, 500);

    EXPECT_EQ(sp_rt_array_get_i64(arr, 0), 100);
    EXPECT_EQ(sp_rt_array_get_i64(arr, 1), 200);
    EXPECT_EQ(sp_rt_array_get_i64(arr, 4), 500);

    sp_rt_free(arr);
}

TEST(RuntimeTest, ArrayFloatingPointElements) {
    void* arr = sp_rt_array_alloc(3, sizeof(double));
    ASSERT_NE(arr, nullptr);
    EXPECT_EQ(sp_rt_array_length(arr), 3);

    sp_rt_array_set_f64(arr, 0, 3.14);
    sp_rt_array_set_f64(arr, 1, 2.718);
    sp_rt_array_set_f64(arr, 2, 1.414);

    EXPECT_DOUBLE_EQ(sp_rt_array_get_f64(arr, 0), 3.14);
    EXPECT_DOUBLE_EQ(sp_rt_array_get_f64(arr, 1), 2.718);
    EXPECT_DOUBLE_EQ(sp_rt_array_get_f64(arr, 2), 1.414);

    sp_rt_free(arr);
}

TEST(RuntimeTest, DynamicListOperations) {
    void* list = sp_rt_list_alloc(sizeof(int64_t), 4);
    ASSERT_NE(list, nullptr);
    EXPECT_EQ(sp_rt_list_size(list), 0);

    sp_rt_list_push_i64(list, 10);
    sp_rt_list_push_i64(list, 20);
    sp_rt_list_push_i64(list, 30);
    EXPECT_EQ(sp_rt_list_size(list), 3);

    EXPECT_EQ(sp_rt_list_get_i64(list, 0), 10);
    EXPECT_EQ(sp_rt_list_get_i64(list, 1), 20);
    EXPECT_EQ(sp_rt_list_get_i64(list, 2), 30);

    sp_rt_list_set_i64(list, 1, 99);
    EXPECT_EQ(sp_rt_list_get_i64(list, 1), 99);

    int64_t popped = sp_rt_list_pop_i64(list);
    EXPECT_EQ(popped, 30);
    EXPECT_EQ(sp_rt_list_size(list), 2);
}

TEST(RuntimeTest, MapStringAndIntKeyOperations) {
    void* map = sp_rt_map_alloc();
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(sp_rt_map_size(map), 0);

    // String keys
    sp_rt_map_set_str(map, "Shanto", 21);
    sp_rt_map_set_str(map, "Alice", 25);
    EXPECT_TRUE(sp_rt_map_contains_str(map, "Shanto"));
    EXPECT_TRUE(sp_rt_map_contains_str(map, "Alice"));
    EXPECT_FALSE(sp_rt_map_contains_str(map, "Bob"));

    EXPECT_EQ(sp_rt_map_get_str(map, "Shanto"), 21);
    EXPECT_EQ(sp_rt_map_get_str(map, "Alice"), 25);

    // Int keys
    sp_rt_map_set_i64(map, 101, 500);
    EXPECT_EQ(sp_rt_map_get_i64(map, 101), 500);
    EXPECT_EQ(sp_rt_map_size(map), 3);
}

// ---------------------------------------------------------------------------
// Phase 11: Deterministic ARC and Memory Lifecycle Tests
// ---------------------------------------------------------------------------

static int g_dtor_invocations = 0;
static void test_dtor_callback(void* ptr) {
    (void)ptr;
    g_dtor_invocations++;
}

TEST(RuntimeTest, ARCAllocationRetainReleaseLifecycle) {
    g_dtor_invocations = 0;
    uint64_t initial_arc_objs = sp_rt_mem_active_arc_objects();

    // Allocate an ARC object with 64 bytes payload
    void* obj = sp_rt_arc_alloc(64, test_dtor_callback);
    ASSERT_NE(obj, nullptr);
    EXPECT_EQ(sp_rt_strong_count(obj), 1u);
    EXPECT_EQ(sp_rt_weak_count(obj), 1u);
    EXPECT_EQ(sp_rt_mem_active_arc_objects(), initial_arc_objs + 1);

    // Retain object
    void* retained = sp_rt_retain(obj);
    EXPECT_EQ(retained, obj);
    EXPECT_EQ(sp_rt_strong_count(obj), 2u);

    // Release once: destructor should NOT run yet
    sp_rt_release(obj);
    EXPECT_EQ(sp_rt_strong_count(obj), 1u);
    EXPECT_EQ(g_dtor_invocations, 0);

    // Release second time: destructor MUST run immediately
    sp_rt_release(obj);
    EXPECT_EQ(g_dtor_invocations, 1);
    EXPECT_EQ(sp_rt_mem_active_arc_objects(), initial_arc_objs);
}

TEST(RuntimeTest, WeakReferenceCycleBreakingLifecycle) {
    g_dtor_invocations = 0;

    void* strong = sp_rt_arc_alloc(32, test_dtor_callback);
    ASSERT_NE(strong, nullptr);

    // Create weak pointer
    void* weak = sp_rt_weak_create(strong);
    ASSERT_NE(weak, nullptr);
    EXPECT_EQ(sp_rt_strong_count(strong), 1u);
    EXPECT_EQ(sp_rt_weak_count(strong), 2u); // 1 internal for strong + 1 external weak

    // Upgrade weak reference while strong is still alive: succeeds
    void* upgraded = sp_rt_weak_upgrade(weak);
    ASSERT_NE(upgraded, nullptr);
    EXPECT_EQ(upgraded, strong);
    EXPECT_EQ(sp_rt_strong_count(strong), 2u);

    // Release the upgraded strong reference
    sp_rt_release(upgraded);
    EXPECT_EQ(sp_rt_strong_count(strong), 1u);
    EXPECT_EQ(g_dtor_invocations, 0);

    // Drop the primary strong reference
    sp_rt_release(strong);
    EXPECT_EQ(g_dtor_invocations, 1); // Destructor ran!

    // Try to upgrade after strong destruction: MUST return nullptr
    void* dead_upgrade = sp_rt_weak_upgrade(weak);
    EXPECT_EQ(dead_upgrade, nullptr);

    // Finally drop the weak reference
    sp_rt_weak_release(weak);
}

TEST(RuntimeTest, MemoryArenaAllocationAndReset) {
    void* arena = sp_rt_arena_create(1024); // 1 KB arena
    ASSERT_NE(arena, nullptr);
    EXPECT_EQ(sp_rt_arena_allocated(arena), 0u);

    // Allocate 100 bytes
    void* p1 = sp_rt_arena_alloc(arena, 100, 8);
    ASSERT_NE(p1, nullptr);
    EXPECT_EQ(sp_rt_arena_allocated(arena), 100u);

    // Allocate 200 bytes
    void* p2 = sp_rt_arena_alloc(arena, 200, 8);
    ASSERT_NE(p2, nullptr);
    EXPECT_GE(sp_rt_arena_allocated(arena), 300u);

    // Reset arena: allocations reset to 0 with zero free overhead
    sp_rt_arena_reset(arena);
    EXPECT_EQ(sp_rt_arena_allocated(arena), 0u);

    // Reallocate after reset
    void* p3 = sp_rt_arena_alloc(arena, 50, 8);
    ASSERT_NE(p3, nullptr);
    EXPECT_EQ(sp_rt_arena_allocated(arena), 50u);

    sp_rt_arena_destroy(arena);
}
