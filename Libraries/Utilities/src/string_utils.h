#pragma once

#include <util_common.h>

namespace ara {
std::string ReplaceString(std::string subject, const std::string &search, const std::string &replace);
void ReplaceStringInPlace(std::string &subject, const std::string &search, const std::string &replace);
std::vector<std::string> splitByNewline(const std::string &s);
std::vector<std::string> split(const std::string &s, std::string delim);
std::vector<std::string> split(const std::string &s, char delim);
bool is_number(const std::string &s);
std::string str_toupper(std::string s);

#ifdef _WIN32
std::string ConvertWCSToStdString(const wchar_t *wcharStr);
#endif
// std::string ConvertBSTRToMBS(BSTR bstr);
}  // namespace ara