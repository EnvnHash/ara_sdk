#include "string_utils.h"

#include <algorithm>
#include <cctype>
#ifdef _WIN32
#include <Windows.h>
#else
#include <codecvt>
#endif
#include <locale>


namespace ara {
std::string ReplaceString(std::string subject, const std::string &search, const std::string &replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
    return subject;
}

void ReplaceStringInPlace(std::string &subject, const std::string &search, const std::string &replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

std::vector<std::string> splitByNewline(const std::string &s) {
    std::vector<std::string> elems;
    std::istringstream       ss;
    ss.str(s);
    for (std::string line; std::getline(ss, line);) {
        elems.push_back(line);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    std::stringstream        ss(s);
    std::string              item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, std::string delim) {
    std::string              strCpy = s;
    std::vector<std::string> elems;
    size_t                   fPos = strCpy.find(delim);

    while (fPos != std::string::npos) {
        elems.push_back(strCpy.substr(0, fPos));
        strCpy = strCpy.substr(fPos + std::strlen(delim.c_str()), std::strlen(strCpy.c_str()) - 1);
        fPos   = strCpy.find(delim);
    }

    elems.push_back(strCpy);

    return elems;
}

bool is_number(const std::string &s) {
    return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

std::string str_toupper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
    return s;
}

#ifdef _WIN32
std::string ConvertWCSToStdString(const wchar_t *wideStr) {
    UINT codePage = CP_ACP;
    size_t length = std::min<size_t>(std::char_traits<wchar_t>::length(wideStr), 2^32);
    int nChars = WideCharToMultiByte(codePage, 0, wideStr, static_cast<int>(length), nullptr, 0, nullptr, nullptr);
    if (nChars == 0) {
        throw std::runtime_error("Failed to calculate required buffer size.");
    }

    std::vector<char> buffer(nChars);
    int result = WideCharToMultiByte(codePage, 0, wideStr, static_cast<int>(length), &buffer[0], nChars, nullptr, nullptr);
    if (result == 0) {
        throw std::runtime_error("Failed to convert wide string.");
    }

    std::string outStr;
    std::copy(buffer.begin(), buffer.end(), std::back_inserter(outStr));

    return outStr;
}
#endif

/*
//std-string stuff to add save current device settings to .cfg file.
//Used for robust string comparison and display.
//DO NOT assign converted string to any device variables - use original BSTR
type instead. std::string ConvertBSTRToMBS(BSTR bstr)
{
    int wslen = ::SysStringLen(bstr);
    return ConvertWCSToMBS((wchar_t*)bstr, wslen);
}
*/

}  // namespace ara
