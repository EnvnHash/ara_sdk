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
#include "GLBase.h"
#include "Asset/AssetImageSection.h"
#include "Asset/AssetImageSource.h"
#include "Asset/AssetColor.h"
#include "Asset/ResSrcFile.h"

using namespace std;

namespace ara {

AssetManager::AssetManager(const string &data_root_path, const string &compilation_filepath, GLBase *glbase)
    : m_glbase(glbase) {
    AssetLoader::setAssetPath(data_root_path);
    m_rootNode = std::make_unique<ResNode>("root", m_glbase);
    m_rootNode->setAssetManager(this);
}

bool AssetManager::load(const string &path) {
    SrcFile srcFile(m_glbase);

    m_loadState   = true;
    m_resFilePath = path;

    std::vector<uint8_t> vp;
    loadResource(nullptr, vp, path);
    insertPreAndPostContent(vp);

    m_loadState = false;

    try {
        if (srcFile.process(m_rootNode.get(), vp)) {
            m_rootNode->preprocess();
            m_rootNode->process();

            if (!m_rootNode->errList.empty()) {
                throw runtime_error("instance: Cannot process");
            }

            if (!m_rootNode->load()) {
                throw runtime_error("instance: Cannot load");
            }

            // get fonts
            if (const auto fontsNode = findNode("fonts")) {
                for (const auto &f : fontsNode->m_children) {
                    m_fontLUT.insert({f->getName(), e_font_lut{f->getRawValue(), 20}});
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

std::optional<std::string> AssetManager::getValue(const string &path) {
    const auto node = findNode<AssetFont>(path);
    if (!node) {
        return std::nullopt;
    }
    return std::optional(node->getRawValue());
}

std::optional<std::string> AssetManager::getValue(const string &path, const int index) {
    const auto node = findNode<AssetFont>(path);
    if (!node) {
        return std::nullopt;
    }

    auto tok = node->splitValue();
    if (index < 0 || index >= tok.getParCount()) {
        return std::nullopt;
    }

    return std::optional(tok.getPar(index));
}

float *AssetManager::color(const string& path) {
    const auto c = findNode<AssetColor>(path);
    return c == nullptr ? m_defaultColor : c->getColor4fv();
}

Font *AssetManager::font(void* context, const string& path, const float pixRatio) {
    const auto font = findNode<AssetFont>(path);
    auto f    = getGLFont(context, string("Fonts/verdana.ttf"), 20, pixRatio);

    if (font != nullptr) {
        if (Font *faux; (faux = getGLFont(context, font->m_fontPath, font->m_size, pixRatio)) != nullptr) {
            f = faux;
        }
    }

    return f;
}

Font *AssetManager::getGLFont(void* context, string font_type_path, const int size, const float pixRatio) {
    if (m_fontLUT.contains(font_type_path)) {
        auto [path, sz]   = m_fontLUT[font_type_path];
        font_type_path = path;
    }

    auto f = m_fontList[context].find(font_type_path, size, pixRatio);

    if (!f) {
        if (std::vector<uint8_t> vp; loadResource(nullptr, vp, font_type_path) > 0) {
            if ((f = m_fontList[context].add(vp, font_type_path, size, pixRatio)) != nullptr) {
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
        if (e_file_bind eb{node->getPath(), path}; ranges::find_if(m_fileBind, [&](const e_file_bind &e) {
                 return e.file_path == eb.file_path && e.node_path == eb.node_path;
             }) == m_fileBind.end()) {
            m_fileBind.emplace_back(eb);
        }
    }

    auto [sz, modTime] = AssetLoader::loadAssetToMem(dest, path);
    m_resFolderFiles[path] = modTime;
    return sz;
}

#ifdef ARA_USE_CMRC
std::pair<const uint8_t*, size_t> AssetManager::loadResource(ResNode *node, const string& path) {
    if (node) {
        e_file_bind eb{node->getPath(), path};
        if (ranges::find_if(m_fileBind, [&](const e_file_bind &e) {
                return e.file_path == eb.file_path && e.node_path == eb.node_path;
            }) == m_fileBind.end()) {
            m_fileBind.emplace_back(eb);
        }
    }

    auto ts = AssetLoader::mapAssetToMem(path);
    m_resFolderFiles[path] = ts.modTime;

    return { ts.data, ts.size };
}
#endif

Font *AssetManager::loadFont(void* context, const string& path, const int size, const float pixRatio) {
    std::vector<uint8_t> v;
    if (loadResource(nullptr, v, path) <= 0) {
        return nullptr;
    }
    return m_fontList[context].add(v, path, size, pixRatio);
}

bool AssetManager::checkForChangesInFolderFiles() {
    if (usingComp() || !isOK()) {
        return false;
    }

    filesystem::file_time_type ft;
    const auto dataPath = AssetLoader::getAssetPath(); // absolute path

    // check for file deletion or modification, stop at first change found
    for (auto &[filename, lastChangeTime] : m_resFolderFiles) {
        if (auto p = dataPath / filename; !std::filesystem::exists(p)) {
            LOG << "AssetManager detected a file was deleted in the resource folder";
            return true;
        } else {
            try {
                ft = last_write_time(p);
            } catch (...) {
                LOGE << "AssetManager::checkForChangesInFolderFiles Error: Failed to get last write time";
            }

            if (ft != lastChangeTime) {
                LOG << "AssetManager detected a file was modified in the resource folder";
                return true;
            }
        }
    }
    return false;
}

bool AssetManager::reload() {
    auto nroot = std::make_unique<ResNode>("root", m_glbase);
    nroot->setAssetManager(this);

    SrcFile              srcFile(m_glbase);
    std::vector<uint8_t> vp;

    loadResource(nullptr, vp, m_resFilePath);
    insertPreAndPostContent(vp);

    if (srcFile.process(nroot.get(), vp)) {
        nroot->preprocess();
        nroot->process();

        if (nroot->errList.empty() && nroot->load() && nroot->errList.empty()) {
            m_rootNode = std::move(nroot);
            return true;
        }

        LOGE << "New resource file has errors";
        for (auto &[lineIndex, errorString] : nroot->errList) {
            LOGE << "Line " << std::to_string(lineIndex + 1) << " " << errorString;
        }
    }

    return false;
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

    std::unique_lock lock(m_updtMtx);

    auto root = std::make_unique<ResNode>("root", m_glbase);
    root->setAssetManager(this);

    SrcFile              srcFile(m_glbase);
    std::vector<uint8_t> vp;

    loadResource(nullptr, vp, m_resFilePath);
    insertPreAndPostContent(vp);

    if (srcFile.process(root.get(), vp)) {
        processFile(std::move(root));
    }
}

void AssetManager::processFile(std::unique_ptr<ResNode>&& root) {
    root->preprocess();
    root->process();

    bool err = true;
    if (root->errList.empty() && root->load() && root->errList.empty()) {
        m_rootNode  = std::move(root);
        err         = false;
    }

    if (err) {
        LOGE << "New resource file has errors";
        for (auto &[lineIndex, errorString] : root->errList) {
            LOGE << "Line " << std::to_string(lineIndex + 1) << " " << errorString;
        }
    }
}

void AssetManager::callForChangesInFolderFiles() {
    if (usingComp() || !isOK()) return;

    // Check for new files...
    for (const filesystem::directory_entry &file : filesystem::recursive_directory_iterator(m_glbase->m_resRootPath)) {
        if (const auto& filePath = file.path(); !m_resFolderFiles.contains(filePath.filename().string())) {
            m_resFolderFiles[filePath.string()] = filesystem::last_write_time(file);
        }
    }

    // check for file deletion or modification
    bool keep;
    do {
        keep = false;
        checkFolderFiles(keep);
    } while (keep);
}

void AssetManager::checkFolderFiles(bool& keep) {
    for (auto &[file, modTime] : m_resFolderFiles) {
        const auto& p = filesystem::path(m_glbase->m_resRootPath) / file;
        if (!filesystem::exists(p)) {
            m_resFolderFiles.erase(p.string());
            keep = true;
            break;
        }

        filesystem::file_time_type ft;

        try {
            ft = filesystem::last_write_time(p);
        } catch (...) {
        }

        if (ft != modTime) {
            /*
            auto str = e.first.path().string();
            std::replace(str.begin(), str.end(), '\\', '/');
            str.erase(0, m_dataRootPath.size());
            PropagateFileChange(false, str);
            */
            m_resFolderFiles[file] = ft;
        }
    }
}

void AssetManager::propagateFileChange(const bool deleted, const string &fpath) {
    if (usingComp()) {
        return;
    }

    ResNode *node;
    LOG << "PROPAGATE " << fpath;

    for (auto &[node_path, file_path] : m_fileBind) {
        if (file_path == fpath) {
            if ((node = findNode(node_path)) != nullptr) {
                LOG << "propagate to " << fpath;
                node->onResourceChange(deleted, fpath);
            }
        }
    }
}

AssetImageBase *AssetManager::img(const string& path) const {
    auto node = findNode(path);
    return !node || !(typeid(node[0]) == typeid(AssetImageSource)
        || typeid(node[0]) == typeid(AssetImageSection)) ? nullptr : dynamic_cast<AssetImageBase *>(node);
}

std::string AssetManager::value(const std::string &path) {
    const auto node = findNode<AssetFont>(path);
    return node ? node->getRawValue() : std::string{};
}

std::string AssetManager::value(const std::string &path, const std::string& def) {
    const auto node = findNode<AssetFont>(path);
    return node ? node->getRawValue() : std::string{};
}

void AssetManager::addFontListForContext(void* context) {
    m_fontList[context].setGlbase(m_glbase);
}
}  // namespace ara
