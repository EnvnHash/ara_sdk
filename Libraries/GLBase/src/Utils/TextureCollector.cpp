//
// Created by user on 17.03.2021.
//

#include <Res/ResInstance.h>

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

Texture *TextureCollector::addFromRes(Instance *res, const std::string &fn, int mipMapLevel) {
    return checkForExistence(fn, nullptr, [&]  {
        if (res) {
            std::vector<uint8_t> vp;
            res->loadResource(nullptr, vp, fn);
            return add(vp, fn, nullptr, mipMapLevel);
        } else {
            return (Texture*)nullptr;
        }
    });
}

Texture *TextureCollector::add(std::vector<uint8_t>& vp, const std::string &fn, const std::filesystem::path *dataPath, int32_t mipMapLevel) {
    if (vp.empty()) {
        return nullptr;
    }

    m_texMap[fn] = { Texture(m_glBase) };
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

void TextureCollector::remove(const std::filesystem::path &fileName) {
    auto fn = fileName.string();
    if (!m_texMap.empty() && !fileName.empty()) {
        auto it = m_texMap.find(fn);
        if (it != m_texMap.end()) {
            for (auto &cb : it->second.removeCbs) {
                cb();
            }
            m_texMap.erase(it);
        }
    }
}

void TextureCollector::addRemoveCb(const std::string &fileName, const std::function<void()>& f) {
    auto it = m_texMap.find(fileName);
    if (it != m_texMap.end()) {
        it->second.removeCbs.emplace_back(f);
    }
}

Texture* TextureCollector::checkForExistence(const std::string &fn, const std::filesystem::path *dataPath,
                                             const std::function<Texture*()>& f) {
    if (!m_texMap.empty()) {
        auto it = m_texMap.find(fn);
        if (it != m_texMap.end()) {
            std::filesystem::path p;
            if (dataPath) {
                p = *dataPath / std::filesystem::path(fn);
            } else {
                p = std::filesystem::path(fn);
            }

#ifndef ARA_USE_CMRC
            // check for file updates
            auto ft = std::filesystem::last_write_time(p);
            if (ft != m_texMap[fn].time) {
                it->second.texture.releaseTexture();
                return f();
            } else {
#endif
                return &it->second.texture;
#ifndef ARA_USE_CMRC
            }
#endif
        }
    }

    return f();
}

}  // namespace ara
