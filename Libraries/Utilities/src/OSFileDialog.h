// Created by user on 11.08.2020.
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
