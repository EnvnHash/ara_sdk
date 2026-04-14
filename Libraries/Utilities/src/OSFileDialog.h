
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

#ifdef _WIN32
#include <shobjidl.h>
#include <windows.h>
#elif defined(__linux__) && !defined(__ANDROID__)
#include <gtk/gtk.h>
#endif

#include <util_common.h>

namespace ara {
#ifdef _WIN32
std::string osOpenFileDialog(const std::vector<std::pair<std::string, std::string>>& allowedSuffix, HWND owner = nullptr);
std::string osSaveFileDialog(const std::vector<std::pair<std::string, std::string>>& fileTypes, HWND owner = nullptr);
std::vector<COMDLG_FILTERSPEC> convertToFilterSpec(const std::vector<std::pair<std::string, std::string>>& fileTypes,
    std::vector<std::pair<std::wstring, std::wstring>>& wFileTypes);
#elif defined(__linux__) && !defined(__ANDROID__)
std::string osOpenFileDialog(const std::vector<std::pair<std::string, std::string>>& allowedSuffix);
std::string osSaveFileDialog(const std::vector<std::pair<std::string, std::string>>& fileTypes);
#elif __APPLE__
std::string osOpenFileDialog(const std::vector<std::pair<std::string, std::string>>& allowedSuffix);
std::string osSaveFileDialog(const std::vector<std::pair<std::string, std::string>>& fileTypes);
#endif

}  // namespace ara
