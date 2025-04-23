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

#include <Asset/ResNode.h>

namespace ara {

class Texture;

class AssetImageBase : public ResNode {
public:
    enum class Type { none, simple, frame };
    enum class Dist { none, horz, vert };

    AssetImageBase(const std::string& name, GLBase *glbase) : ResNode(name, glbase) {}
    ~AssetImageBase() override = default;

    virtual Texture             *getTexture() { return nullptr; }
    virtual GLuint               getTexID() { return 0; }
    virtual glm::ivec2          &getTexSize() { return m_texSize; }
    virtual int                 *getSrcPixSize() { return m_nullIntPtr; }
    virtual int                 *getSectionSize() { return m_nullIntPtr; }
    virtual int                 *getSectionPos() { return m_nullIntPtr; }
    virtual Type                getType() { return m_type; }
    virtual Dist                getDist() { return m_dist; }
    int                         getVer(int ver) { return (m_ver.find(ver) != m_ver.end()) ? m_ver[ver] : m_verDefault; }

    std::array<int, 2> getVerPos(int ver);  // returns pos
    int  getDistPixOffset() const { return m_distPixOffset; }
    int *getSectionSep() { return m_pixSep; }

protected:
    static inline int   m_nullIntPtr[2] = {0, 0};
    Type                m_type          = Type::none;
    Dist                m_dist          = Dist::none;
    int                 m_pixSep[2]     = {0, 0};  // separation between each component in pixels
    int                 m_distPixOffset = 0;
    int                 m_verDefault    = 0;    // if a given version isn't found then this value will be used, by default is zero
    std::map<int, int>  m_ver;                  // version, this value multiplies in m_Dist direction by m_DistPixOffset
    glm::ivec2          m_texSize{0};
};


}  // namespace ara
