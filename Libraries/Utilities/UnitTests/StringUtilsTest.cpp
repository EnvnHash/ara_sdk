//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <UtilityUnitTestCommon.h>
#include "string_utils.h"

namespace ara {

TEST(StringUtilsTest, ReplaceString) {
    std::string subject = "hello world";
    EXPECT_EQ(ReplaceString(subject, "world", "everyone"), "hello everyone");

    subject = "repeat repeat";
    EXPECT_EQ(ReplaceString(subject, "repeat", "replace"), "replace replace");
}

TEST(StringUtilsTest, ReplaceStringInPlace) {
    std::string subject = "hello world";
    ReplaceStringInPlace(subject, "world", "everyone");
    EXPECT_EQ(subject, "hello everyone");

    subject = "repeat repeat";
    ReplaceStringInPlace(subject, "repeat", "replace");
    EXPECT_EQ(subject, "replace replace");
}

TEST(StringUtilsTest, SplitByNewline) {
    std::string text = "line1\nline2\nline3";
    std::vector<std::string> result = splitByNewline(text);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "line1");
    EXPECT_EQ(result[1], "line2");
    EXPECT_EQ(result[2], "line3");
}

TEST(StringUtilsTest, SplitByDelimiter) {
    std::string text = "apple,banana,cherry";
    char delimiter = ',';
    std::vector<std::string> result = split(text, delimiter);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "apple");
    EXPECT_EQ(result[1], "banana");
    EXPECT_EQ(result[2], "cherry");
}

TEST(StringUtilsTest, SplitByStringDelimiter) {
    std::string text = "one||two||three";
    std::string delimiter = "||";
    std::vector<std::string> result = split(text, delimiter);
    EXPECT_EQ(result.size(), 3);
    EXPECT_EQ(result[0], "one");
    EXPECT_EQ(result[1], "two");
    EXPECT_EQ(result[2], "three");
}

TEST(StringUtilsTest, IsNumber) {
    EXPECT_TRUE(is_number("12345"));
    EXPECT_FALSE(is_number("abcde"));
}

TEST(StringUtilsTest, StrToUpper) {
    std::string str = "hello world";
    EXPECT_EQ(str_toupper(str), "HELLO WORLD");

    str = "Mixed CASE";
    EXPECT_EQ(str_toupper(str), "MIXED CASE");
}

#ifdef _WIN32
TEST(StringUtilsTest, ConvertWCSToStdString) {
    const wchar_t *wideStr = L"Hello World";
    std::string narrowStr = ConvertWCSToStdString(wideStr);
    EXPECT_EQ(narrowStr, "Hello World");
}
#endif

}  // namespace ara
