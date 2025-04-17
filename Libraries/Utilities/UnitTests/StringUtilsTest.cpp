//
// Created by sven on 02-04-25.
//

#include <utility_unit_test_common.h>
#include "string_utils.h"

namespace ara {

class StringUtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(StringUtilsTest, ReplaceString) {
    std::string subject = "hello world";
    EXPECT_EQ(ReplaceString(subject, "world", "everyone"), "hello everyone");

    subject = "repeat repeat";
    EXPECT_EQ(ReplaceString(subject, "repeat", "replace"), "replace replace");
}

TEST_F(StringUtilsTest, ReplaceStringInPlace) {
    std::string subject = "hello world";
    ReplaceStringInPlace(subject, "world", "everyone");
    EXPECT_EQ(subject, "hello everyone");

    subject = "repeat repeat";
    ReplaceStringInPlace(subject, "repeat", "replace");
    EXPECT_EQ(subject, "replace replace");
}

TEST_F(StringUtilsTest, SplitByNewline) {
    std::string text = "line1\nline2\nline3";
    std::vector<std::string> result = splitByNewline(text);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "line1");
    EXPECT_EQ(result[1], "line2");
    EXPECT_EQ(result[2], "line3");
}

TEST_F(StringUtilsTest, SplitByDelimiter) {
    std::string text = "apple,banana,cherry";
    char delimiter = ',';
    std::vector<std::string> result = split(text, delimiter);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "apple");
    EXPECT_EQ(result[1], "banana");
    EXPECT_EQ(result[2], "cherry");
}

TEST_F(StringUtilsTest, SplitByStringDelimiter) {
    std::string text = "one||two||three";
    std::string delimiter = "||";
    std::vector<std::string> result = split(text, delimiter);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "one");
    EXPECT_EQ(result[1], "two");
    EXPECT_EQ(result[2], "three");
}

TEST_F(StringUtilsTest, IsNumber) {
    EXPECT_TRUE(is_number("12345"));
    EXPECT_FALSE(is_number("abcde"));
}

TEST_F(StringUtilsTest, StrToUpper) {
    std::string str = "hello world";
    EXPECT_EQ(str_toupper(str), "HELLO WORLD");

    str = "Mixed CASE";
    EXPECT_EQ(str_toupper(str), "MIXED CASE");
}

#ifdef _WIN32
TEST_F(StringUtilsTest, ConvertWCSToStdString) {
    const wchar_t *wideStr = L"Hello World";
    std::string narrowStr = ConvertWCSToStdString(wideStr);
    EXPECT_EQ(narrowStr, "Hello World");
}
#endif

}  // namespace ara
