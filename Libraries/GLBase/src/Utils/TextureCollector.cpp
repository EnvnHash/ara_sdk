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

#include <GLBase.h>
#include <RwBinFile.h>
#include <Asset/AssetManager.h>
#include <Utils/TextureCollector.h>

namespace fs = std::filesystem;
using namespace std;

namespace ara {

Texture *TextureCollector::add(const std::filesystem::path &fileName) {
    if (fileName.empty()) {
        LOGE << "TextureCollector::add Error: emtpy filename";
        return  nullptr;
    }

    auto fn = fileName.string();
    return checkForExistence(fn, nullptr, [&] {
        m_texMap[fn] = { Texture(m_glBase) };
        m_texMap[fn].texture.loadTexture2D(fn, 8);
        m_texMap[fn].time = std::filesystem::last_write_time(fn);
        return &m_texMap[fn].texture;
    });
}

Texture *TextureCollector::addFromMem(const std::filesystem::path &fileName, const std::filesystem::path &dataPath,
                                      int32_t mipMapLevel) {
    if (!m_glBase) {
        return nullptr;
    }

    if (fileName.empty()) {
        LOGE << "TextureCollector::addFromMem Error: emtpy filename";
        return  nullptr;
    }

    auto fn = fileName.string();
    return checkForExistence(fn, &dataPath,[&]  {
        std::vector<uint8_t> vp;
        ReadBinFile(vp, dataPath / fn);
        return add(vp, fn, &dataPath, mipMapLevel);
    });
}

Texture *TextureCollector::addFromAssetManager(const std::string &fn, int mipMapLevel) {
    return checkForExistence(fn, nullptr, [&]  {
        std::vector<uint8_t> vp;
        auto& al = m_glBase->getAssetManager()->getAssetLoader();
        al.loadAssetToMem(vp, fn);
        return add(vp, fn, &al.getAssetPath(), mipMapLevel);
    });
}

Texture *TextureCollector::add(std::vector<uint8_t>& vp, const std::string &fn, const std::filesystem::path *dataPath, int32_t mipMapLevel) {
    if (vp.empty()) {
        return nullptr;
    }

    m_texMap[fn].texture.setGlbase(m_glBase);
    m_texMap[fn].texture.loadFromMemPtr(&vp[0], vp.size(), GL_TEXTURE_2D, mipMapLevel);
    m_texMap[fn].texture.setFiltering(GL_LINEAR, mipMapLevel == 0 ? GL_LINEAR : GL_LINEAR_MIPMAP_LINEAR);
#ifndef ARA_USE_GLES31
    m_texMap[fn].texture.setWraping(GL_CLAMP_TO_BORDER);
#else
    m_texMap[fn].texture.setWraping(GL_CLAMP_TO_EDGE);
#endif

    std::filesystem::path p;
    if (dataPath) {
        p = *dataPath / std::filesystem::path(fn);
    } else {
        p = std::filesystem::path(fn);
    }

#ifndef ARA_USE_CMRC
    try {
        m_texMap[fn].time = std::filesystem::last_write_time(p);
    } catch(std::runtime_error& e) {
        LOGE << "TextureCollector::addFromMem Error: std::runtime_error: " << e.what();
    }
#endif

    return &m_texMap[fn].texture;
}

auto TextureCollector::remove(const std::filesystem::path &fileName) {
    auto fn = fileName.string();
    if (!m_texMap.empty() && !fileName.empty()) {
        auto it = m_texMap.find(fn);
        if (it != m_texMap.end()) {
            for (auto &cb : it->second.removeCbs) {
                cb();
            }
            return m_texMap.erase(it);
        }
    }
    return m_texMap.end();
}

void TextureCollector::addRemoveCb(const std::string &fileName, const std::function<void()>& f) {
    auto it = m_texMap.find(fileName);
    if (it != m_texMap.end()) {
        it->second.removeCbs.emplace_back(f);
    }
}

Texture* TextureCollector::checkForExistence(const std::string &fn, const std::filesystem::path *dataPath,
                                             const std::function<Texture*()>& f) {
    if (!m_texMap.empty() && m_glBase) {
        auto it = m_texMap.find(fn);
        if (it != m_texMap.end()) {
            auto al = m_glBase->getAssetManager()->getAssetLoader();
            auto ft = al.getLastFileUpdate(fn);
            if (ft != m_texMap[fn].time) {
                it->second.texture.releaseTexture();
                return f();
            } else {
                return &it->second.texture;
            }
        }
    }

    return f();
}

void TextureCollector::clear() {
    for (auto texIt = m_texMap.begin(); texIt != m_texMap.end();) {
        texIt = remove(texIt->first);
    }
    m_texMap.clear();
}

}  // namespace ara
