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

#include <GlbCommon/GlbCommon.h>
#include "Texture.h"

namespace ara {

class GLBase;
class Texture;
class AssetManager;

class TextureCollectorElement {
public:
    Texture                                                 texture;
    std::unordered_map<std::string, std::function<void()>>  removeCbs;
    std::filesystem::file_time_type                         time{};
};

class TextureCollector {
public:
    void init(GLBase *glBase) { m_glBase = glBase; }
    Texture *add(const std::filesystem::path &fileName);
    void addCommon(const std::string &fn, int32_t mipMapLevel, const std::filesystem::path *dataPath);
    Texture *add(const char* filePtr, size_t size, const std::string &fn, const std::filesystem::path *dataPath, int32_t mipMapLevel);
    Texture *add(std::vector<uint8_t>& vp, const std::string &fileName, const std::filesystem::path *dataPath, int32_t mipMapLevel);
    Texture *addFromMem(const std::filesystem::path &fileName, const std::filesystem::path &dataPath, int32_t mipMapLevel);
    Texture *addFromAssetManager(const std::string &fn, int mipMapLevel);
    void addRemoveCb(const std::string &fileName, const std::function<void()>& f);
    void removeRemoveCb(const std::string &fileName);
    auto remove(const std::filesystem::path &fileName);
    Texture *checkForExistence(const std::string &fileName, const std::filesystem::path *dataPath,
                               const std::function<Texture*()>& f);
    void clear();

private:
    std::unordered_map<std::string, TextureCollectorElement>    m_texMap;
    GLBase*                                                     m_glBase = nullptr;
};

}  // namespace ara
