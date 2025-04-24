
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

    int result = std::snprintf(buf.get(), size, format.c_str(), args ... );
    if (result < 0) {  // Check for errors during snprintf
        throw std::runtime_error("Error during formatting.");
    }

    buf[size - 1] = '\0';

    return { buf.get(), buf.get() + size - 1 };
}

#ifdef _WIN32
std::string ConvertWCSToStdString(const wchar_t *wcharStr);
LPCWSTR StringToLPCWSTR(const std::string& utf8String);
#endif

}  // namespace ara