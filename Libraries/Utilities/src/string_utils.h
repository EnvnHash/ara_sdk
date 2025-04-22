#pragma once

#include <util_common.h>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ara {
std::string ReplaceString(std::string subject, const std::string &search, const std::string &replace);
void ReplaceStringInPlace(std::string &subject, const std::string &search, const std::string &replace);
std::vector<std::string> splitByNewline(const std::string &s);
std::vector<std::string> split(const std::string &s, const std::string& delim);
std::vector<std::string> split(const std::string &s, char delim);
bool is_number(const std::string &s);
std::string str_toupper(std::string s);

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args) {
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if (size_s <= 0) {
        throw std::runtime_error( "Error during formatting." );
    }
    auto size = static_cast<size_t>( size_s );
    auto buf = std::make_unique<char[]>( size );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return { buf.get(), buf.get() + size - 1 };
}

#ifdef _WIN32
std::string ConvertWCSToStdString(const wchar_t *wcharStr);
LPCWSTR StringToLPCWSTR(const std::string& utf8String);
#endif

}  // namespace ara