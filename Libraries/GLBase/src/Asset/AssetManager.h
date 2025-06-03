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

#include "Asset/AssetFont.h"
#include "Asset/ResGlFont.h"
#include <AssetLoader.h>

namespace ara {

class AssetImageBase;

class AssetManager {
public:
    struct e_file_bind {
        std::string node_path;
        std::string file_path;
    };

    struct e_font_lut {
        std::string path;
        int         size;
    };

    AssetManager(const std::string &data_root_path, const std::string &compilation_filepath, GLBase *glbasconst);
    ~AssetManager() { clearGLFonts(); }

    bool     Load(const std::string &path);
    bool     Reload();
    void     insertPreAndPostContent(std::vector<uint8_t>& vp);

    [[nodiscard]] ResNode *getRoot() const { return m_rootNode.get(); }
    [[nodiscard]] ResNode *findNode(const std::string &path) const { return m_rootNode->findNode(path); }

    template <typename T>
    T *findNode(const std::string& path) {
        auto n = findNode(path);
        return (n && typeid(n[0]) == typeid(T)) ? static_cast<T*>(n) : nullptr;
    }

    bool getvalue(std::string &dest, const std::string &path);
    bool getvalue(std::string &dest, const std::string &path, int index);

    std::string value(const std::string &path);
    std::string value(const std::string &path, const std::string& def);

    template<CoordinateType32Signed T>
    T value(const std::string &path, T def) {
        std::string s;
        if (!getvalue(s, path)) {
            return def;
        }

        try {
            return typeid(T) == typeid(int32_t) ? stoi(s) : stof(s);
        } catch (...) {
            return def;
        }
    }

    template<CoordinateType32Signed T>
    bool value_v(std::vector<T> &v, const std::string &path, int fcount = 0, T def = 0) {
        v.clear();

        auto *node = findNode<AssetFont>(path);
        if (node == nullptr) {
            return false;
        }

        ParVec tok = node->splitValue();
        fcount     = fcount > 0 ? fcount : tok.getParCount();

        for (int i = 0; i < fcount; i++) {
            v.emplace_back(tok.getFloatPar(i, static_cast<T>(def)));
        }

        return true;
    }


    float   *color(const std::string& path);
    Font    *font(const std::string& path, float pixRatio);

    // resources
    size_t          loadResource(ResNode *node, std::vector<uint8_t> &dest, const std::string& path);
    Font           *loadFont(const std::string& path, int size, float pixRatio);
    AssetImageBase *img(const std::string& path);

    [[nodiscard]] bool isOK() const { return m_loadState; }
    static bool usingComp() { return AssetLoader::usingCmrc(); }

    bool checkForChangesInFolderFiles();
    void callResSourceChange();
    void callForChangesInFolderFiles();

    void         setPreContent(const std::string &str) { m_preContent = str; }
    void         setPostContent(const std::string &str) { m_postContent = str; }
    void         setPostContent(std::string str) { m_postContent = std::move(str); }
    std::string &getPreContent() { return m_preContent; }
    std::string &getPostContent() { return m_postContent; }

    // Fonts
    Font       *getGLFont(std::string font_type_path, int size, float pixRatio);
    void        clearGLFonts() { m_fontList.clear(); }
    std::mutex *getMtx() { return &m_updtMtx; }
    FontList&   getGLFont() { return m_fontList; }

    AssetLoader &getAssetLoader() { return m_assetLoader; }

private:
    void propagateFileChange(bool deleted, const std::string &fpath);

    AssetLoader                     m_assetLoader;
    FontList                        m_fontList;
    ResNode::Ptr                    m_rootNode = nullptr;
    //ResFile                         m_resFile;
    std::string                     m_preContent;
    std::string                     m_postContent;
    //std::string                     m_dataRootPath;
    std::string                     m_resFilePath;
    std::filesystem::file_time_type m_resFileLastTime;
    bool                            m_loadState = false;

    std::unordered_map<std::string, std::filesystem::file_time_type> m_resFolderFiles;
    //std::map<std::filesystem::directory_entry, std::filesystem::file_time_type> m_resFolderFiles;
    std::vector<e_file_bind>                                         m_fileBind;

    float                                       default_Color[4] = {1, 1, 1, 1};
    std::mutex                                  m_updtMtx;
    std::unordered_map<std::string, e_font_lut> m_fontLUT;
    GLBase                                     *m_glbase = nullptr;
};

}  // namespace ara
