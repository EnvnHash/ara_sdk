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

#include <RwBinFile.h>
#include "Asset/AssetManager.h"
#include "Asset/AssetImageSection.h"
#include "Asset/AssetImageSource.h"
#include "Asset/AssetColor.h"
#include "Asset/ResSrcFile.h"

using namespace std;

namespace ara {

AssetManager::AssetManager(const string &data_root_path, const string &compilation_filepath, GLBase *glbase)
    : m_glbase(glbase) {
    m_assetLoader.setAssetPath(data_root_path);
    m_fontList.setGlbase(glbase);

    m_rootNode = std::make_unique<ResNode>("root", m_glbase);
    m_rootNode->setAssetManager(this);
}

bool AssetManager::Load(const string &path) {
    SrcFile sfile(m_glbase);

    m_loadState   = true;
    m_resFilePath = path;

    std::vector<uint8_t> vp;
    loadResource(nullptr, vp, path);
    insertPreAndPostContent(vp);

    m_loadState = false;

    try {
        if (sfile.process(m_rootNode.get(), vp)) {
            m_rootNode->preprocess();
            m_rootNode->process();

            if (!m_rootNode->errList.empty()) {
                throw runtime_error("instance: Cannot process");
            }

            if (!m_rootNode->load()) {
                throw runtime_error("instance: Cannot load");
            }

            // get fonts
            if (auto fontsNode = findNode("fonts")) {
                for (const auto &f : fontsNode->m_node) {
                    m_fontLUT.insert({f->m_name, e_font_lut{f->m_value, 20}});
                }
            }

            m_loadState = true;
            return true;
        }
        return true;
    } catch (std::runtime_error &err) {
        LOGE << err.what() << endl;
        LOGE << "instance: Cannot process source file";
        return false;
    }
}

bool AssetManager::getvalue(string &dest, const string &path) {
    auto node = findNode<AssetFont>(path);
    if (!node) {
        return false;
    }

    dest = node->m_value;
    return true;
}

bool AssetManager::getvalue(string &dest, const string &path, int index) {
    auto node = findNode<AssetFont>(path);
    if (!node) {
        return false;
    }

    ParVec tok = node->splitValue();

    if (index < 0 || index >= tok.getParCount()) {
        return false;
    }

    dest = tok.getPar(index);
    return true;
}

float *AssetManager::color(const string& path) {
    auto c = findNode<AssetColor>(path);
    return c == nullptr ? default_Color : c->getColor4fv();
}

Font *AssetManager::font(const string& path, float pixRatio) {
    auto font = findNode<AssetFont>(path);
    auto f    = getGLFont(string("Fonts/verdana.ttf"), 20, pixRatio);

    if (font != nullptr) {
        Font *faux;
        if ((faux = getGLFont(font->m_FontPath, font->m_Size, pixRatio)) != nullptr) {
            f = faux;
        }
    }

    return f;
}

Font *AssetManager::getGLFont(string font_type_path, int size, float pixRatio) {
    if (m_fontLUT.contains(font_type_path)) {
        e_font_lut f   = m_fontLUT[font_type_path];
        font_type_path = f.path;
    }

    auto f = m_fontList.find(font_type_path, size, pixRatio);

    if (!f) {
        std::vector<uint8_t> vp;
        if (loadResource(nullptr, vp, font_type_path) > 0) {
            if ((f = m_fontList.add(vp, font_type_path, size, pixRatio)) != nullptr) {
                return f;
            }
        }
    }

    if (!f) {
        LOGE << "Cannot open font (" << font_type_path << ") size=" << size;
    }

    return f;
}

size_t AssetManager::loadResource(ResNode *node, std::vector<uint8_t> &dest, const string& path) {
    if (node) {
        e_file_bind eb{node->getPath(), path};
        if (ranges::find_if(m_fileBind, [&](const e_file_bind &e) {
                 return e.file_path == eb.file_path && e.node_path == eb.node_path;
             }) == m_fileBind.end()) {
            m_fileBind.emplace_back(eb);
        }
    }

    auto ts = m_assetLoader.loadAssetToMem(dest, path);
    m_resFolderFiles[path] = ts.second;

    return ts.first;
}

Font *AssetManager::loadFont(const string& path, int size, float pixRatio) {
    std::vector<uint8_t> v;
    if (loadResource(nullptr, v, path) <= 0) {
        return nullptr;
    }
    return m_fontList.add(v, path, size, pixRatio);
}

bool AssetManager::checkForResSourceChange() {
    if (usingComp() || !isOK()) {
        return false;
    }

    try {
        return filesystem::last_write_time(m_resSysFile) != m_resFileLastTime;
    } catch (...) {
        return false;
    }
}

bool AssetManager::checkForChangesInFolderFiles() {
    if (usingComp() || !isOK()) {
        return false;
    }

    filesystem::file_time_type ft;
    bool change = false;

    // check for file deletion or modification
    for (auto &e: m_resFolderFiles) {
        if (!filesystem::exists(e.first)) {
            change = true;
            break;
        } else {
            try {
                ft = filesystem::last_write_time(e.first);
            } catch (...) {
            }

            if (ft != e.second) {
                change = true;
            }
        }
    }

    return change;
}

bool AssetManager::Reload() {
    ResNode::Ptr nroot;

    nroot = std::make_unique<ResNode>("root", m_glbase);
    nroot->setAssetManager(this);

    SrcFile              sfile(m_glbase);
    std::vector<uint8_t> vp;
    bool                 err = true;

    loadResource(nullptr, vp, m_resFilePath);
    insertPreAndPostContent(vp);

    if (sfile.process(nroot.get(), vp)) {
        nroot->preprocess();
        nroot->process();

        if (nroot->errList.empty()) {
            if (nroot->load()) {
                if (nroot->errList.empty()) {
                    m_rootNode = std::move(nroot);
                    err      = false;
                    return true;
                }
            }
        }

        if (err) {
            LOGE << "New resource file has errors";
            for (ResNode::e_error &e : nroot->errList) {
                LOGE << "Line " << std::to_string(e.lineIndex + 1) << " " << e.errorString;
            }
        }
    }

    return err;
}

void AssetManager::insertPreAndPostContent(std::vector<uint8_t>& vp) {
    if (!getPreContent().empty()) {
        vp.insert(vp.begin(), getPreContent().begin(), getPreContent().end());
    }

    if (!getPostContent().empty()) {
        vp.insert(vp.end(), getPostContent().begin(), getPostContent().end());
    }
}

void AssetManager::callResSourceChange() {
    if (usingComp() || !isOK()) {
        return;
    }

    filesystem::file_time_type ft;

    try {
        ft = filesystem::last_write_time(m_resSysFile);
    } catch (...) {
        return;
    }

    // std::unique_lock<mutex> lock(m_updtMtx);

    if (ft != m_resFileLastTime) {
        ResNode::Ptr nroot;

        LOG << "\x1B[93mRes-File Changed (" << m_resSysFile << ") \x1B[37m";

        nroot = std::make_unique<ResNode>("root", m_glbase);
        nroot->setAssetManager(this);

        SrcFile              sfile(m_glbase);
        std::vector<uint8_t> vp;

        loadResource(nullptr, vp, m_resFilePath);
        insertPreAndPostContent(vp);

        if (sfile.process(nroot.get(), vp)) {
            nroot->preprocess();
            nroot->process();

            bool err = true;

            if (nroot->errList.empty() && nroot->load() && nroot->errList.empty()) {
                m_rootNode = std::move(nroot);
                err      = false;
            }

            if (err) {
                LOGE << "New resource file has errors";
                for (ResNode::e_error &e : nroot->errList) {
                    LOGE << "Line " << std::to_string(e.lineIndex + 1) << " " << e.errorString;
                }
            }
        }

        m_resFileLastTime = ft;
    }
}

void AssetManager::callForChangesInFolderFiles() {
    if (usingComp() || !isOK()) return;

    filesystem::file_time_type ft;

    // Check for new files...
    for (const filesystem::directory_entry &file : filesystem::recursive_directory_iterator("resdata")) {
        auto fileStr = file.path().string();
        if (!m_resFolderFiles.contains(fileStr)) {
            m_resFolderFiles[fileStr] = filesystem::last_write_time(file);
        }
    }

    // check for file deletion or modification
    bool keep;

    do {
        keep = false;

        for (auto &e : m_resFolderFiles) {
            if (!filesystem::exists(e.first)) {
                m_resFolderFiles.erase(e.first);
                keep = true;
                break;
            } else {
                try {
                    ft = filesystem::last_write_time(e.first);
                } catch (...) {
                }

                if (ft != e.second) {
                    /*
                    auto str = e.first.path().string();
                    std::replace(str.begin(), str.end(), '\\', '/');
                    str.erase(0, m_dataRootPath.size());
                    PropagateFileChange(false, str);
                    m_resFolderFiles[e.first] = ft;
                    */
                }
            }
        }
    } while (keep);
}

void AssetManager::propagateFileChange(bool deleted, const string &fpath) {
    if (usingComp()) {
        return;
    }

    ResNode *node;
    LOG << "PROPAGATE " << fpath;

    for (e_file_bind &fb : m_fileBind) {
        if (fb.file_path == fpath) {
            if ((node = findNode(fb.node_path)) != nullptr) {
                LOG << "propagate to " << fpath;
                node->onResourceChange(deleted, fpath);
            }
        }
    }
}

AssetImageBase *AssetManager::img(const string& path) {
    auto node = findNode(path);
    return !node || !(typeid(node[0]) == typeid(AssetImageSource)
        || typeid(node[0]) == typeid(AssetImageSection)) ? nullptr : dynamic_cast<AssetImageBase *>(node);
}

std::string AssetManager::value(const std::string &path) {
    auto node = findNode<AssetFont>(path);
    return node ? node->m_value : std::string{};
}

std::string AssetManager::value(const std::string &path, const std::string& def) {
    auto node = findNode<AssetFont>(path);
    return node ? node->m_value : std::string{};
}

}  // namespace ara
