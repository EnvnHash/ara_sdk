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

#include "BoundingBoxer.h"
#include "GLBase.h"
#include "Utils/FBO.h"

using namespace glm;
using namespace std;

namespace ara {

BoundingBoxer::BoundingBoxer(GLBase *glbase) : m_shCol(glbase->shaderCollector()) {
    fbo = make_unique<FBO>(FboInitParams{glbase, 6, 1, 1, GL_R32F, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false});
    fbo->setMinFilter(GL_NEAREST);
    fbo->setMagFilter(GL_NEAREST);
    initShader();
}

void BoundingBoxer::initShader() {
    std::string vert = m_shCol.getShaderHeader() + "// Bounding Boxer Vertex Shader \n";
    vert += STRINGIFY(layout(location = 0) in vec4 position;    \n
        uniform mat4 m_model;                                   \n
        void main() {                                           \n
            gl_Position = m_model * position;                   \n
    });

    // Iterate six times over each vertex, output -x, x, -y, y, -z, z as color
    std::string geom = m_shCol.getShaderHeader() + "// Bounding Boxer Geometry Shader \n";

    geom +=
        STRINGIFY(layout(triangles, invocations = 1) in;        \n
            layout(points, max_vertices = 18) out;              \n
            out vec4 o_col;                                     \n
            void main() {                                       \n
                float outVal;                                   \n
                for (int i = 0; i < gl_in.length(); i++) {      \n
                    for (int j = 0; j < 6; j++) {               \n
                        outVal = j == 0 ? -gl_in[i].gl_Position.x
                                        : j == 1 ? gl_in[i].gl_Position.x
                                        : j == 2 ? -gl_in[i].gl_Position.y
                                        : j == 3 ? gl_in[i].gl_Position.y
                                        : j == 4 ? -gl_in[i].gl_Position.z
                                        : j == 5 ? gl_in[i].gl_Position.z
                                        : 0.0;
                              o_col = vec4(outVal, 0.0, 0.0, 1.0);
                              gl_Position = vec4(float(j) / 3.0 - 1.0 + 0.08333, 0.0, 0.0, 1.0);
                              EmitVertex();                     \n
                              EndPrimitive();                   \n
                          }                                     \n
                      }                                         \n
                  });

    std::string frag = m_shCol.getShaderHeader() + "// Bounding Boxer Fragment Shader \n";
    frag += STRINGIFY(layout(location = 0) out vec4 fragColor;  \n
        in vec4 o_col;                                          \n
        void main() {
            fragColor = o_col;
        });
    boxCalcShader = m_shCol.add("BoundingBoxer", vert, geom, frag);
}

void BoundingBoxer::begin() const {
    if (!fbo) {
        LOGE << "BoundingBoxer no s_fbo";
        return;
    }

    fbo->bind();
    glClearColor(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(),
                 -std::numeric_limits<float>::max(), 0.f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (boxCalcShader) {
        boxCalcShader->begin();
        boxCalcShader->setIdentMatrix4fv("m_model");
    }

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendEquation(GL_MAX);
    glBlendFunc(GL_ONE, GL_ONE);
}

void BoundingBoxer::end() {
    Shaders::end();
    if (!fbo) {
        LOGE << "BoundingBoxer no s_fbo";
        return;
    }
    fbo->unbind();

#ifdef ARA_USE_GLES31
    // NOTE: GLES does not allow GL_RED as m_format for glReadPixels
    glesGetTexImage(fbo->getColorImg(), GL_TEXTURE_2D, GL_RGBA, GL_FLOAT, fbo->getWidth(), 1, (GLubyte *)&result[0]);
#else
    glBindTexture(GL_TEXTURE_2D, fbo->getColorImg());
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, (void *)&result[0]);
    glBindTexture(GL_TEXTURE_2D, 0);
#endif

    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (int i = 0; i < 3; i++) {
        if (-result[i * 2] != std::numeric_limits<float>::max()) {
            boundMin[i] = -result[i * 2];
        }

        if (-result[i * 2 + 1] != std::numeric_limits<float>::max()) {
            boundMax[i] = result[i * 2 + 1];
        }
    }

    for (int i = 0; i < 3; i++) {
        center[i] = (-boundMin[i] + boundMax[i]) * 0.5f + boundMin[i];
    }
}

}  // namespace ara