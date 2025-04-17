//
// Created by user on 17.03.2021.
//

#pragma once

#include <glb_common/glb_common.h>
#include "Texture.h"

namespace ara {

class GLBase;
class Texture;
class Instance;

class TextureCollectorElement {
public:
    Texture                             texture;
    std::list<std::function<void()>>    removeCbs;
    std::filesystem::file_time_type     time{};
};

class TextureCollector {
public:
    void init(GLBase *glBase) { m_glBase = glBase; }
    Texture *add(const std::filesystem::path &fileName);
    Texture *add(std::vector<uint8_t>& vp, const std::string &fileName, const std::filesystem::path *dataPath, int32_t mipMapLevel);
    Texture *addFromMem(const std::filesystem::path &fileName, const std::filesystem::path &dataPath, int32_t mipMapLevel);
    Texture *addFromRes(Instance *res, const std::string &fn, int mipMapLevel);
    void addRemoveCb(const std::string &fileName, const std::function<void()>& f);
    void remove(const std::filesystem::path &fileName);
    Texture *checkForExistence(const std::string &fileName, const std::filesystem::path *dataPath,
                               const std::function<Texture*()>& f);

private:
    std::unordered_map<std::string, TextureCollectorElement>    m_texMap;
    GLBase*                                                     m_glBase = nullptr;
};

}  // namespace ara
