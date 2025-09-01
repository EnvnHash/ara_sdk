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

#include "GeoPrimitives/Quad.h"
#include "Utils/FBO.h"
#include "Utils/Texture.h"

namespace ara {
class PropoImage {
public:
    enum propoImagePos { CENTER = 0, UPPER_LEFT = 1, UPPER_RIGHT = 2, LOWER_LEFT = 3, LOWER_RIGHT = 4 };

    explicit PropoImage(GLBase* glbase);
    PropoImage(GLBase* glbase, const std::string& fileName, int screenW, int screenH, float logoWidth = 0.5f,
               propoImagePos pos = CENTER, float border = 0.f);

    void setupQuad();
    void draw() const;
    void setWidth(float _newWidth);

    [[nodiscard]] float getImgHeight() const { return m_imgHeight; }
    [[nodiscard]] float getImgAspectRatio() const { return m_imgAspectRatio; }
    [[nodiscard]] GLint getTexId() const { return static_cast<GLint>(m_imgTex->getId()); }

private:
    std::unique_ptr<Texture> m_imgTex;
    std::unique_ptr<Quad>    m_imgQuad;
    propoImagePos            m_pos{};
    bool                     m_inited = false;

    float m_imgWidth{};
    float m_imgHeight{};
    float m_oldImgWidth{};
    float m_oldImgHeight{};
    float m_screenW{};
    float m_screenH{};
    float m_border{};
    float m_imgAspectRatio{};
    float m_screenAspectRatio{};
};
}  // namespace ara
