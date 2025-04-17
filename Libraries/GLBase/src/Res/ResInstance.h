//
// Created by user on 04.02.2021.
//

#pragma once

#include "Res/ResFile.h"
#include "Res/ResFont.h"
#include "Res/ResGlFont.h"
#include "RwBinFile.h"

#ifdef ARA_USE_CMRC
#include <cmrc/cmrc.hpp>
#endif

namespace ara {

class ImageBase;

class Instance {
public:
    Instance(const std::string &data_root_path, const std::string &compilation_filepath, GLBase *glbasconst);
    bool Load(const std::string &path, bool force_external);  // force_external will make Load to read from an
                                                              // external path even if the resource exists
    bool     Reload();
    ResNode *getRoot() { return rootNode.get(); }
    ResNode *findNode(const std::string &path) { return rootNode->findNode(path); }

    template <typename T>
    T *findNode(std::string path) { /*std::unique_lock<std::mutex> l(m_updtMtx);*/
        ResNode *n = findNode(std::move(path));
        if (n == nullptr) return nullptr;
        return (typeid(n[0]) == typeid(T)) ? (T *)n : nullptr;
    }

    bool getvalue(std::string &dest, const std::string &path);
    bool getvalue(std::string &dest, const std::string &path, int index);

    std::string value(const std::string &path) {
        auto node = findNode<ResFont>(path);
        if (node == nullptr) return std::string();
        return node->m_Value;
    }

    std::string value(const std::string &path, const std::string def) {
        auto node = findNode<ResFont>(path);
        if (node == nullptr) return def;
        return node->m_Value;
    }

    int value1i(const std::string &path, int def) {
        std::string s;
        if (!getvalue(s, path)) return def;
        int v;
        try {
            v = stoi(s);
        } catch (...) {
            v = def;
        }
        return v;
    }

    float value1f(const std::string &path, float def) {
        std::string s;
        if (!getvalue(s, path)) return def;
        float v;
        try {
            v = stof(s);
        } catch (...) {
            v = def;
        }
        return v;
    }

    bool valueiv(std::vector<int> &v, const std::string &path, int fcount = 0, int def = 0);
    bool valuefv(std::vector<float> &v, const std::string &path, int fcount = 0, float def = 0);

    float *color(std::string path);

    Font *font(std::string path, float pixRatio);

    // resources
    // this function will decide if it loads from the compilation or an external file, tries internal first, then external
    size_t loadResource(ResNode *node, std::vector<uint8_t> &dest, const std::string& path, bool force_external = false);
    size_t loadResourceFromCompilation(std::vector<uint8_t> &dest, std::string path) {
        if (!m_ResFile.isOK()) {
            return 0;
        }
        return m_ResFile.ReadEntry(dest, path);
    }

    static size_t loadResourceFromExternal(std::vector<uint8_t> &dest, const std::string &path) {
        size_t ts = ReadBinFile(dest, path);
        if (!ts) {
            LOGE << "[ERROR] loadResourceFromExternal(" << path << ") size=" << ts;
        }
        return ts;
    }

    Font *loadFont(const std::string& path, int size, float pixRatio);

    //
    ImageBase *img(const std::string& path);
    // void						drawImg(std::string
    // path, float* mvp, float x, float y, float w, float h, float* color, int
    // flags, int index, int align_x, int align_h, float scale);

    //
    bool isOK() { return m_LoadState == 100; }
    bool usingComp() { return m_ResFile.isOK(); }  // Using compilation

    //
    bool checkForResSourceChange();
    bool checkForChangesInFolderFiles();
    void CallResSourceChange();
    void CallForChangesInFolderFiles();

    //
    bool Compile(std::filesystem::path p);

    //
    void         setPreContent(std::string &str) { m_PreContent = str; }
    void         setPostContent(std::string &str) { m_PostContent = str; }
    void         setPostContent(std::string str) { m_PostContent = std::move(str); }
    std::string &getPreContent() { return m_PreContent; }
    std::string &getPostContent() { return m_PostContent; }

    // Fonts
    Font *getGLFont(std::string font_type_path, int size, float pixRatio);

    void        clearGLFont() { m_FontList.clear(); }
    std::mutex *getMtx() { return &m_updtMtx; }

private:
    ResNode::Ptr rootNode = nullptr;

    float default_Color[4] = {1, 1, 1, 1};

    std::string                     m_DataRootPath;
    std::string                     m_ResFilePath;
    std::filesystem::file_time_type m_ResFileLastTime;
    std::filesystem::path           m_ResSysFile;

    unsigned m_LoadState = 0;

    std::map<std::filesystem::directory_entry, std::filesystem::file_time_type> m_ResFolderFiles;

    struct e_file_bind {
        std::string node_path;
        std::string file_path;
    };

    std::vector<e_file_bind> m_FileBind;

    void PropagateFileChange(bool deleted, const std::string &fpath);

    template <typename TP>
    static time_t to_time_t(TP tp) {
        return std::chrono::system_clock::to_time_t(std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            tp - TP::clock::now() + std::chrono::system_clock::now()));
    }

    // Fonts
    FontList m_FontList;

    // Resources
    void populateFolderFiles();

    ResFile     m_ResFile;
    std::string m_PreContent;
    std::string m_PostContent;

    struct e_font_lut {
        std::string path;
        int         size;
    };

    std::mutex                                  m_updtMtx;
    std::unordered_map<std::string, e_font_lut> m_FontLUT;
    GLBase                                     *m_glbase = nullptr;
};

}  // namespace ara
