
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
std::string OpenFileDialog(const std::vector<COMDLG_FILTERSPEC>& allowedSuffix, HWND owner);
std::string SaveFileDialog(std::vector<std::pair<std::string, std::string>> fileTypes, HWND owner);
#elif defined(__linux__) && !defined(__ANDROID__)

std::string OpenFileDialog(std::vector<const char *> &allowedSuffix);
std::string SaveFileDialog(std::vector<std::pair<std::string, std::string>> fileTypes);

#elif __APPLE__
std::string OpenFileDialog(std::vector<const char*>& allowedSuffix);
std::string SaveFileDialog(const std::vector<std::pair<std::string, std::string>>& fileTypes);
#endif

}  // namespace ara
