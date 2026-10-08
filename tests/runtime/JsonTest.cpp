#include <gtest/gtest.h>
#include "sp/runtime/Json.h"

using namespace sp::rt::json;

TEST(JsonTest, ParsePrimitives) {
    auto nullVal = parse("null");
    ASSERT_NE(nullVal, nullptr);
    EXPECT_TRUE(nullVal->isNull());

    auto trueVal = parse("true");
    ASSERT_NE(trueVal, nullptr);
    EXPECT_TRUE(trueVal->isBool());
    EXPECT_TRUE(trueVal->asBool());

    auto falseVal = parse("false");
    ASSERT_NE(falseVal, nullptr);
    EXPECT_TRUE(falseVal->isBool());
    EXPECT_FALSE(falseVal->asBool());

    auto intVal = parse("42");
    ASSERT_NE(intVal, nullptr);
    EXPECT_TRUE(intVal->isInt());
    EXPECT_EQ(intVal->asInt(), 42);

    auto floatVal = parse("3.14159");
    ASSERT_NE(floatVal, nullptr);
    EXPECT_TRUE(floatVal->isDouble());
    EXPECT_NEAR(floatVal->asDouble(), 3.14159, 0.00001);

    auto strVal = parse("\"Hello SP!\"");
    ASSERT_NE(strVal, nullptr);
    EXPECT_TRUE(strVal->isString());
    EXPECT_EQ(strVal->asString(), "Hello SP!");
}

TEST(JsonTest, ParseComplexObjectAndArray) {
    std::string jsonStr = R"({
        "service": "sp-gateway",
        "port": 8080,
        "enabled": true,
        "tags": ["cloud", "high-performance", "native"],
        "config": {
            "maxConnections": 10000,
            "timeout": 2.5
        }
    })";

    auto root = parse(jsonStr);
    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(root->isObject());

    EXPECT_EQ(root->get("service")->asString(), "sp-gateway");
    EXPECT_EQ(root->get("port")->asInt(), 8080);
    EXPECT_TRUE(root->get("enabled")->asBool());

    auto tags = root->get("tags");
    ASSERT_NE(tags, nullptr);
    EXPECT_TRUE(tags->isArray());
    EXPECT_EQ(tags->size(), 3u);
    EXPECT_EQ(tags->get(0)->asString(), "cloud");
    EXPECT_EQ(tags->get(1)->asString(), "high-performance");
    EXPECT_EQ(tags->get(2)->asString(), "native");

    auto config = root->get("config");
    ASSERT_NE(config, nullptr);
    EXPECT_TRUE(config->isObject());
    EXPECT_EQ(config->get("maxConnections")->asInt(), 10000);
    EXPECT_NEAR(config->get("timeout")->asDouble(), 2.5, 0.01);
}

TEST(JsonTest, StringifyMinifiedAndIndented) {
    auto obj = JsonValue::makeObject();
    obj->set("name", JsonValue::makeString("SP"));
    obj->set("version", JsonValue::makeInt(16));

    std::string minified = obj->stringify(0);
    EXPECT_EQ(minified, "{\"name\":\"SP\",\"version\":16}");

    std::string pretty = obj->stringify(2);
    EXPECT_TRUE(pretty.find("\n") != std::string::npos);
    EXPECT_TRUE(pretty.find("  \"name\": \"SP\"") != std::string::npos);

    // Round-trip parse
    auto roundTrip = parse(minified);
    ASSERT_NE(roundTrip, nullptr);
    EXPECT_EQ(roundTrip->get("name")->asString(), "SP");
    EXPECT_EQ(roundTrip->get("version")->asInt(), 16);
}

TEST(JsonTest, ErrorHandlingOnInvalidJson) {
    std::string err;
    auto invalid1 = parse("{ unquoted_key: 123 }", &err);
    EXPECT_EQ(invalid1, nullptr);
    EXPECT_FALSE(err.empty());

    auto invalid2 = parse("[1, 2, 3,", &err);
    EXPECT_EQ(invalid2, nullptr);

    auto invalid3 = parse("\"unterminated", &err);
    EXPECT_EQ(invalid3, nullptr);
}
