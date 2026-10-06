#include <gtest/gtest.h>
#include "fmt/Formatter.h"

using namespace sp::fmt;

TEST(FormatterTest, BasicFormattingAndIdempotency) {
    Formatter formatter;
    std::string messy = "fn main(){int x=1+2;print(x);}";
    std::string formatted = formatter.format(messy);

    std::string expected =
        "fn main() {\n"
        "    int x = 1 + 2;\n"
        "    print(x);\n"
        "}\n";

    EXPECT_EQ(formatted, expected);

    // Idempotency: formatting formatted code produces the exact same output
    EXPECT_EQ(formatter.format(formatted), formatted);
    EXPECT_TRUE(formatter.isFormatted(formatted));
    EXPECT_FALSE(formatter.isFormatted(messy));
}

TEST(FormatterTest, IndentationAndNestedBlocks) {
    Formatter formatter;
    std::string input =
        "class Calculator {\n"
        "fn add(a: int, b: int): int {\n"
        "if (a > 0) {\n"
        "return a + b;\n"
        "} else {\n"
        "return b;\n"
        "}\n"
        "}\n"
        "}\n";

    std::string formatted = formatter.format(input);

    std::string expected =
        "class Calculator {\n"
        "    fn add(a: int, b: int): int {\n"
        "        if (a > 0) {\n"
        "            return a + b;\n"
        "        } else {\n"
        "            return b;\n"
        "        }\n"
        "    }\n"
        "}\n";

    EXPECT_EQ(formatted, expected);
    EXPECT_EQ(formatter.format(formatted), formatted);
}

TEST(FormatterTest, ForLoopPreservesInlineSemicolons) {
    Formatter formatter;
    std::string input = "fn loop() { for (int i = 0; i < 10; i++) { print(i); } }";
    std::string formatted = formatter.format(input);

    std::string expected =
        "fn loop() {\n"
        "    for (int i = 0; i < 10; i++) {\n"
        "        print(i);\n"
        "    }\n"
        "}\n";

    EXPECT_EQ(formatted, expected);
    EXPECT_EQ(formatter.format(formatted), formatted);
}

TEST(FormatterTest, CommentAndDocCommentPreservation) {
    Formatter formatter;
    std::string input =
        "/// Calculates the sum\n"
        "fn sum(a: int, b: int): int {\n"
        "// Return the result\n"
        "return a + b;\n"
        "}\n";

    std::string formatted = formatter.format(input);

    std::string expected =
        "/// Calculates the sum\n"
        "fn sum(a: int, b: int): int {\n"
        "    // Return the result\n"
        "    return a + b;\n"
        "}\n";

    EXPECT_EQ(formatted, expected);
    EXPECT_EQ(formatter.format(formatted), formatted);
}

TEST(FormatterTest, BinaryAndUnaryOperatorSpacing) {
    Formatter formatter;
    std::string input = "let result = -x + y * 4 / (z - 1);";
    std::string formatted = formatter.format(input);

    std::string expected = "let result = -x + y * 4 / (z - 1);\n";
    EXPECT_EQ(formatted, expected);
    EXPECT_EQ(formatter.format(formatted), formatted);
}

TEST(FormatterTest, BlankLineNormalization) {
    Formatter formatter;
    std::string input =
        "fn first() {\n"
        "    return 1;\n"
        "}\n\n\n\n\n"
        "fn second() {\n"
        "    return 2;\n"
        "}\n";

    std::string formatted = formatter.format(input);

    std::string expected =
        "fn first() {\n"
        "    return 1;\n"
        "}\n\n"
        "fn second() {\n"
        "    return 2;\n"
        "}\n";

    EXPECT_EQ(formatted, expected);
    EXPECT_EQ(formatter.format(formatted), formatted);
}
