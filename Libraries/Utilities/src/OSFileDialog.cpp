
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

#include "OSFileDialog.h"
#include <string_utils.h>

using namespace std;

namespace ara {
#ifdef _WIN32
vector<COMDLG_FILTERSPEC> convertToFilterSpec(const vector<pair<string, string>>& fileTypes, vector<pair<wstring, wstring>>& wFileTypes) {
    wFileTypes.clear();
    for (auto &[fst, snd] : fileTypes) {
        wFileTypes.emplace_back(wstring(   fst.begin(), fst.end()),
                                wstring(snd.begin(), snd.end()));
    }

    vector<COMDLG_FILTERSPEC> aFileTypes;
    for (auto &[fst, snd] : wFileTypes) {
        aFileTypes.emplace_back(COMDLG_FILTERSPEC{fst.c_str(), snd.c_str()});
    }
    return aFileTypes;
}

std::string osOpenFileDialog(const vector<pair<string, string>> &fileTypes, HWND owner) {
    std::string outFileName;
    vector<pair<wstring, wstring>> wFileTypes;
    const auto aFileTypes = convertToFilterSpec(fileTypes, wFileTypes);
    if (auto hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE); SUCCEEDED(hr)) {
        IFileOpenDialog *pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog,
                              reinterpret_cast<void **>(&pFileOpen));

        if (SUCCEEDED(hr)) {
            pFileOpen->SetFileTypes(static_cast<UINT>(aFileTypes.size()), &aFileTypes[0]);
            // Show the Open dialog box.
            if (!owner) {
                owner = GetActiveWindow(); // or GetForegroundWindow()
            }
            hr = pFileOpen->Show(owner);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr)) {
                IShellItem *pItem;
                hr = pFileOpen->GetResult(&pItem);
                if (SUCCEEDED(hr)) {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                    outFileName = ConvertWCSToStdString(pszFilePath);

                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
        CoUninitialize();
    }
    return outFileName;
}

std::string osSaveFileDialog(const vector<pair<string, string>>& fileTypes, HWND owner) {
    std::string outFileName;
    auto hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    vector<pair<wstring, wstring>> wFileTypes;
    const auto aFileTypes = convertToFilterSpec(fileTypes, wFileTypes);
    if (SUCCEEDED(hr)) {
        IFileSaveDialog *pFileSave;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL, IID_IFileSaveDialog,
                              reinterpret_cast<void **>(&pFileSave));

        if (SUCCEEDED(hr)) {
            pFileSave->SetFileTypes(static_cast<UINT>(aFileTypes.size()), &aFileTypes[0]);
            pFileSave->SetDefaultExtension(L".xml");

            // Show the Open dialog box.
            if (!owner) {
                owner = GetActiveWindow(); // or GetForegroundWindow()
            }
            hr = pFileSave->Show(owner);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr)) {
                IShellItem *pItem;
                hr = pFileSave->GetResult(&pItem);
                if (SUCCEEDED(hr)) {
                    PWSTR pszFilePath;
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    // Note: this throws a warning which can be ignored.
                    // std::wstring_convert<std::codecvt_utf8_utf16<char16_t>,char16_t>

                    // doesn't deal correctly with umlauts
                    auto wstr   = std::wstring(pszFilePath);
                    outFileName = std::string(wstr.begin(), wstr.end());

                    pItem->Release();
                }
            }
            pFileSave->Release();
        }
        CoUninitialize();
    }
    return outFileName;
}

#elif defined(__linux__) && !defined(__ANDROID__)

std::string osOpenFileDialog(const vector<pair<string, string>> &allowedSuffix) {
    std::string    outFileName;
    gtk_init(nullptr, nullptr);
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open file", nullptr, GTK_FILE_CHOOSER_ACTION_OPEN, "Cancel",
                                                    GTK_RESPONSE_CANCEL, "Open", GTK_RESPONSE_OK, NULL);
    for (const auto &ftyp: allowedSuffix | views::values) {
        GtkFileFilter *filter = gtk_file_filter_new();  // filter 1
        gtk_file_filter_set_name(filter, ftyp.c_str());
        gtk_file_filter_add_pattern(filter, ftyp.c_str());
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    }

    if (const gint res = gtk_dialog_run(GTK_DIALOG(dialog)); res == GTK_RESPONSE_OK) {
        const auto chooser  = GTK_FILE_CHOOSER(dialog);
        char *filename      = gtk_file_chooser_get_filename(chooser);
        outFileName         = std::string(filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    return outFileName;
}

std::string osSaveFileDialog(const std::vector<std::pair<std::string, std::string>> &fileTypes) {
    std::string    outFileName;
    gtk_init(nullptr, nullptr);
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save file", nullptr, GTK_FILE_CHOOSER_ACTION_SAVE, "Cancel",
                                                    GTK_RESPONSE_CANCEL, "Save", GTK_RESPONSE_OK, NULL);

    // Filters
    for (auto &ftyp : fileTypes) {
        GtkFileFilter *filter = gtk_file_filter_new();  // filter 1
        gtk_file_filter_set_name(filter, ftyp.first.c_str());
        gtk_file_filter_add_pattern(filter, ftyp.second.c_str());
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    }

    if (const gint res = gtk_dialog_run(GTK_DIALOG(dialog)); res == GTK_RESPONSE_OK) {
        auto *chooser   = GTK_FILE_CHOOSER(dialog);
        char *filename  = gtk_file_chooser_get_filename(chooser);
        outFileName     = std::string(filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
    return outFileName;
}

#elif __APPLE__
std::string osOpenFileDialog(const std::vector<std::pair<std::string, std::string>>& allowedSuffix) {
    std::string outFileName;
    return outFileName;
}

std::string osSaveFileDialog(const std::vector<std::pair<std::string, std::string>>& fileTypes) {
    std::string outFileName;
    return outFileName;
}
#endif

}  // namespace ara
