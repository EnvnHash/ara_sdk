//
// Created by user on 11.08.2020.
//

#include "OSFileDialog.h"

#include <codecvt>
#include <string_utils.h>

namespace ara {
#ifdef _WIN32
std::string OpenFileDialog(std::vector<COMDLG_FILTERSPEC> &allowedSuffix, HWND owner) {
    std::string outFileName;
    HRESULT     hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        IFileOpenDialog *pFileOpen;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog,
                              reinterpret_cast<void **>(&pFileOpen));

        if (SUCCEEDED(hr)) {
            hr = pFileOpen->SetFileTypes((UINT)allowedSuffix.size(), &allowedSuffix[0]);
            // Show the Open dialog box.
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

std::string SaveFileDialog(std::vector<std::pair<std::string, std::string>> fileTypes, HWND owner) {
    std::string outFileName;
    HRESULT     hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    std::vector<COMDLG_FILTERSPEC> aFileTypes;
    // aFileTypes.push_back( { L"All files", L"*.*" } );

    // convert string to wstring
    std::vector<std::pair<std::wstring, std::wstring>> wFileTypes;
    wFileTypes.reserve(fileTypes.size());
    for (auto &ftyp : fileTypes) {
        wFileTypes.emplace_back(std::wstring(ftyp.first.begin(), ftyp.first.end()),
                                std::wstring(ftyp.second.begin(), ftyp.second.end()));
    }

    aFileTypes.reserve(wFileTypes.size());
    for (auto &ftyp : wFileTypes) {
        aFileTypes.emplace_back(COMDLG_FILTERSPEC{ftyp.first.c_str(), ftyp.second.c_str()});
    }

    if (SUCCEEDED(hr)) {
        IFileSaveDialog *pFileSave;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL, IID_IFileSaveDialog,
                              reinterpret_cast<void **>(&pFileSave));

        if (SUCCEEDED(hr)) {
            pFileSave->SetFileTypes((UINT)aFileTypes.size(), &aFileTypes[0]);
            pFileSave->SetDefaultExtension(L".xml");

            // Show the Open dialog box.
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

std::string OpenFileDialog(std::vector<const char *> &allowedSuffix) {
    std::string    outFileName;
    GtkFileFilter *filter;
    gtk_init(nullptr, nullptr);
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open file", NULL, GTK_FILE_CHOOSER_ACTION_OPEN, "Cancel",
                                                    GTK_RESPONSE_CANCEL, "Open", GTK_RESPONSE_OK, NULL);
    for (auto &ftyp : allowedSuffix) {
        filter = gtk_file_filter_new();  // filter 1
        gtk_file_filter_set_name(filter, ftyp);
        gtk_file_filter_add_pattern(filter, ftyp);
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    }

    gint res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_OK) {
        char           *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename                = gtk_file_chooser_get_filename(chooser);
        outFileName             = std::string(filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
    while (gtk_events_pending()) gtk_main_iteration();

    return outFileName;
}

std::string SaveFileDialog(std::vector<std::pair<std::string, std::string>> fileTypes) {
    std::string    outFileName;
    GtkFileFilter *filter;
    gtk_init(nullptr, nullptr);
    GtkWidget *dialog = gtk_file_chooser_dialog_new("Save file", NULL, GTK_FILE_CHOOSER_ACTION_SAVE, "Cancel",
                                                    GTK_RESPONSE_CANCEL, "Save", GTK_RESPONSE_OK, NULL);

    // Filters
    for (auto &ftyp : fileTypes) {
        filter = gtk_file_filter_new();  // filter 1
        gtk_file_filter_set_name(filter, ftyp.first.c_str());
        gtk_file_filter_add_pattern(filter, ftyp.second.c_str());
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    }

    gint res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_OK) {
        char           *filename;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        filename                = gtk_file_chooser_get_filename(chooser);
        outFileName             = std::string(filename);
        g_free(filename);
    }

    gtk_widget_destroy(dialog);
    while (gtk_events_pending()) gtk_main_iteration();

    return outFileName;
}

#elif __APPLE__

std::string OpenFileDialog(std::vector<const char*>& allowedSuffix) {
    std::string outFileName;
    return outFileName;
}

std::string SaveFileDialog(const std::vector<std::pair<std::string, std::string>>& fileTypes) {
    std::string outFileName;
    return outFileName;
}
#endif

}  // namespace ara
