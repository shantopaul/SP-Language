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
