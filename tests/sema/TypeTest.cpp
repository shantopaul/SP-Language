#include <gtest/gtest.h>
#include "sp/sema/Type.h"

using namespace sp::sema;

TEST(TypeTest, PrimitiveTypesBasicProperties) {
    TypeContext ctx;

    EXPECT_TRUE(ctx.getInt()->isInteger());
    EXPECT_TRUE(ctx.getInt()->isSignedInteger());
    EXPECT_TRUE(ctx.getInt()->isNumeric());
    EXPECT_FALSE(ctx.getInt()->isFloatingPoint());

    EXPECT_TRUE(ctx.getDouble()->isFloatingPoint());
    EXPECT_TRUE(ctx.getDouble()->isNumeric());
    EXPECT_FALSE(ctx.getDouble()->isInteger());

    EXPECT_TRUE(ctx.getBool()->isBoolean());
    EXPECT_TRUE(ctx.getString()->isString());
    EXPECT_TRUE(ctx.getVoid()->isVoid());
    EXPECT_TRUE(ctx.getNull()->isNull());

    EXPECT_EQ(ctx.getInt()->toString(), "int");
    EXPECT_EQ(ctx.getDouble()->toString(), "double");
    EXPECT_EQ(ctx.getString()->toString(), "string");
    EXPECT_EQ(ctx.getBool()->toString(), "bool");
    EXPECT_EQ(ctx.getVoid()->toString(), "void");
}

TEST(TypeTest, ImplicitWideningConversions) {
    TypeContext ctx;

    // Widening from smaller integers to larger integers and floating point
    EXPECT_TRUE(ctx.getInt8()->canImplicitlyConvertTo(*ctx.getInt16()));
    EXPECT_TRUE(ctx.getInt8()->canImplicitlyConvertTo(*ctx.getInt32()));
    EXPECT_TRUE(ctx.getInt8()->canImplicitlyConvertTo(*ctx.getInt64()));
    EXPECT_TRUE(ctx.getInt8()->canImplicitlyConvertTo(*ctx.getDouble()));

    EXPECT_TRUE(ctx.getInt32()->canImplicitlyConvertTo(*ctx.getInt64()));
    EXPECT_TRUE(ctx.getInt32()->canImplicitlyConvertTo(*ctx.getDouble()));
    EXPECT_TRUE(ctx.getFloat()->canImplicitlyConvertTo(*ctx.getDouble()));

    // Narrowing is NOT implicitly permitted
    EXPECT_FALSE(ctx.getInt64()->canImplicitlyConvertTo(*ctx.getInt32()));
    EXPECT_FALSE(ctx.getDouble()->canImplicitlyConvertTo(*ctx.getFloat()));
    EXPECT_FALSE(ctx.getDouble()->canImplicitlyConvertTo(*ctx.getInt32()));
    EXPECT_FALSE(ctx.getString()->canImplicitlyConvertTo(*ctx.getInt32()));
}

TEST(TypeTest, NullableTypesAndAssignability) {
    TypeContext ctx;

    auto nullableInt = ctx.getNullableType(ctx.getInt());
    EXPECT_TRUE(nullableInt->isNullable());
    EXPECT_EQ(nullableInt->toString(), "int?");

    // Int is assignable to Int?
    EXPECT_TRUE(ctx.getInt()->isAssignableTo(*nullableInt));

    // null is assignable to Int?
    EXPECT_TRUE(ctx.getNull()->isAssignableTo(*nullableInt));

    // null is NOT assignable to non-nullable Int
    EXPECT_FALSE(ctx.getNull()->isAssignableTo(*ctx.getInt()));

    // null is NOT assignable to non-nullable String
    EXPECT_FALSE(ctx.getNull()->isAssignableTo(*ctx.getString()));
}

TEST(TypeTest, ArrayAndMapTypes) {
    TypeContext ctx;

    auto intArray = ctx.getArrayType(ctx.getInt());
    EXPECT_TRUE(intArray->isArray());
    EXPECT_EQ(intArray->toString(), "int[]");

    auto strIntMap = ctx.getMapType(ctx.getString(), ctx.getInt());
    EXPECT_TRUE(strIntMap->isMap());
    EXPECT_EQ(strIntMap->toString(), "Map<string, int>");
}

TEST(TypeTest, FunctionTypeEqualityAndRepresentation) {
    TypeContext ctx;

    auto fn1 = ctx.getFunctionType({ctx.getInt(), ctx.getInt()}, ctx.getInt());
    auto fn2 = ctx.getFunctionType({ctx.getInt(), ctx.getInt()}, ctx.getInt());
    auto fn3 = ctx.getFunctionType({ctx.getInt(), ctx.getString()}, ctx.getInt());

    EXPECT_EQ(fn1->toString(), "fn(int, int) -> int");
    EXPECT_TRUE(fn1->equals(*fn2));
    EXPECT_FALSE(fn1->equals(*fn3));
}

TEST(TypeTest, NominalClassInheritanceAndSubtyping) {
    TypeContext ctx;

    auto animal = ctx.createClassType("Animal");
    auto dog = ctx.createClassType("Dog");
    dog->setSuperclass(animal);

    EXPECT_TRUE(dog->isSubtypeOf(*animal));
    EXPECT_TRUE(dog->isAssignableTo(*animal));
    EXPECT_FALSE(animal->isSubtypeOf(*dog));

    auto iface = ctx.createInterfaceType("Printable");
    dog->addInterface(iface);
    EXPECT_TRUE(dog->isSubtypeOf(*iface));
    EXPECT_TRUE(dog->isAssignableTo(*iface));
}

TEST(TypeTest, CommonTypeResolution) {
    TypeContext ctx;

    // Same types
    EXPECT_EQ(ctx.getCommonType(ctx.getInt(), ctx.getInt()), ctx.getInt());

    // Integer + Double -> Double
    EXPECT_EQ(ctx.getCommonType(ctx.getInt(), ctx.getDouble()), ctx.getDouble());
    EXPECT_EQ(ctx.getCommonType(ctx.getFloat(), ctx.getDouble()), ctx.getDouble());

    // Null + T -> T?
    auto commonNullStr = ctx.getCommonType(ctx.getNull(), ctx.getString());
    EXPECT_TRUE(commonNullStr->isNullable());
    EXPECT_EQ(commonNullStr->toString(), "string?");
}

TEST(TypeTest, TypeStringResolution) {
    TypeContext ctx;

    EXPECT_EQ(ctx.resolveType("int"), ctx.getInt());
    EXPECT_EQ(ctx.resolveType("double"), ctx.getDouble());
    EXPECT_EQ(ctx.resolveType("string"), ctx.getString());
    EXPECT_EQ(ctx.resolveType("bool"), ctx.getBool());

    auto resolvedNullable = ctx.resolveType("string?");
    ASSERT_NE(resolvedNullable, nullptr);
    EXPECT_TRUE(resolvedNullable->isNullable());
    EXPECT_EQ(resolvedNullable->toString(), "string?");

    auto resolvedArray = ctx.resolveType("int[]");
    ASSERT_NE(resolvedArray, nullptr);
    EXPECT_TRUE(resolvedArray->isArray());
    EXPECT_EQ(resolvedArray->toString(), "int[]");

    auto resolvedMap = ctx.resolveType("Map<string, int>");
    ASSERT_NE(resolvedMap, nullptr);
    EXPECT_TRUE(resolvedMap->isMap());
    EXPECT_EQ(resolvedMap->toString(), "Map<string, int>");
}

TEST(TypeTest, ResultAndOptionTypeResolutionAndVariance) {
    TypeContext ctx;

    auto resIntStr = ctx.resolveType("Result<int, string>");
    ASSERT_NE(resIntStr, nullptr);
    EXPECT_TRUE(resIntStr->isEnum());
    EXPECT_EQ(resIntStr->toString(), "Result<int, string>");

    auto optInt = ctx.resolveType("Option<int>");
    ASSERT_NE(optInt, nullptr);
    EXPECT_TRUE(optInt->isEnum());
    EXPECT_EQ(optInt->toString(), "Option<int>");

    auto enumRes = std::static_pointer_cast<EnumType>(resIntStr);
    EXPECT_TRUE(enumRes->isTaggedUnion());
    EXPECT_TRUE(enumRes->hasVariant("Ok"));
    EXPECT_TRUE(enumRes->hasVariant("Err"));
    EXPECT_TRUE(enumRes->hasPayload("Ok"));
    EXPECT_TRUE(enumRes->hasPayload("Err"));

    auto enumOpt = std::static_pointer_cast<EnumType>(optInt);
    EXPECT_TRUE(enumOpt->isTaggedUnion());
    EXPECT_TRUE(enumOpt->hasVariant("Some"));
    EXPECT_TRUE(enumOpt->hasVariant("None"));
    EXPECT_TRUE(enumOpt->hasPayload("Some"));
    EXPECT_FALSE(enumOpt->hasPayload("None"));

    // Variance: Result<int, any> is assignable to Result<int, string>
    auto resIntAny = ctx.resolveType("Result<int, any>");
    EXPECT_TRUE(resIntAny->isAssignableTo(*resIntStr));

    // Variance: Option<any> is assignable to Option<int>
    auto optAny = ctx.resolveType("Option<any>");
    EXPECT_TRUE(optAny->isAssignableTo(*optInt));
}
