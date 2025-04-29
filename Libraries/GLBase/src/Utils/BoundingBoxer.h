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
#include "Shaders/ShaderCollector.h"

namespace ara {

class GLBase;
class FBO;

class BoundingBoxer {
public:
    explicit BoundingBoxer(GLBase *glbase);

    void initShader();
    void begin() const;
    void end();

    void        sendModelMat(GLfloat *matPtr) const { boxCalcShader->setUniformMatrix4fv("m_model", matPtr); }
    static void draw() {}
    glm::vec3  &getBoundMin() { return boundMin; }
    glm::vec3  &getBoundMax() { return boundMax; }
    glm::vec3  &getCenter() { return center; }

    [[nodiscard]] Shaders *getShader() const { return boxCalcShader ? boxCalcShader : nullptr; }

private:
    std::unique_ptr<FBO> fbo;
    Shaders             *boxCalcShader = nullptr;
    ShaderCollector     &m_shCol;

    glm::vec3 boundMin{0.f};
    glm::vec3 boundMax{0.f};
    glm::vec3 center{0.f};

#ifdef ARA_USE_GLES31
    std::array<GLfloat, 24> result;
#else
    std::array<GLfloat, 6> result{};
#endif
};

}  // namespace ara