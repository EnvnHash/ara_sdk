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
#include <GeoPrimitives/Quad.h>
#include <Meshes/Mesh.h>
#include <Utils/CoordGridGen.h>
#include <Utils/Typo/TypoGlyphMap.h>

using namespace glm;
using namespace std;

namespace ara {
CoordGridGen::CoordGridGen(ivec2 texSize) {
#ifdef ARA_USE_GLFW
    unique_ptr<GLFWWindow> m_glCtx = GLBase::createOpenGLCtx();

    Shaders *coordShdr = initCoordGenShader();

    // init typegenerator
    unique_ptr<TypoGlyphMap> typo = make_unique<TypoGlyphMap>(texSize.x, texSize.y);
    typo->loadFont("data/open-sans/OpenSans-Light.ttf", &m_shCol);

    // init a quad to draw
    unique_ptr<Quad> quad = make_unique<Quad>(QuadInitParams{-1.f, -1.f, 2.f, 2.f});

    coordShdr->begin();
    coordShdr->setUniform2f("resolution", static_cast<float>(texSize.x), static_cast<float>(texSize.y));

    quad->draw();
    ara::Shaders::end();

    // destroy opengl context
    m_glCtx->destroy();
#endif
}

Shaders *CoordGridGen::initCoordGenShader() {
    std::string shdr_Header = m_shCol.getShaderHeader();

    std::string vert = STRINGIFY(layout(location = 0) in vec4 position; layout(location = 2) in vec2 texCoord;
                                 out vec2 tex_coord; void main() {
                                     tex_coord   = texCoord;
                                     gl_Position = position;
                                 });
    vert             = shdr_Header + vert;

    std::string frag = STRINGIFY(
        in vec2 texCoord; \n layout(location = 0) out vec4 fragColor; \n uniform uint gridType; \n uniform vec2 resolution; \n void
            main() {
                vec2 lineWidth = vec2(1.0 / resolution.x, 1.0 / resolution.y);
                vec2 stepSize  = vec2(0.05, 0.05);

                // Normalized pixel coordinates (from 0 to 1)
                vec2 uv = fragCoord / iResolution.xy;

                float lineVertW  = mod(uv.y, stepSize.x) < lineWidth.y ? 1.0 : 0.0;
                float lineHoriW  = mod(uv.x, stepSize.y) < lineWidth.x ? 1.0 : 0.0;
                vec4  whiteLines = vec4(lineHoriW + lineVertW);

                float lineVertR = mod(uv.y, stepSize.x * 5.0) < lineWidth.y ? 1.0 : 0.0;
                float lineHoriR = mod(uv.x, stepSize.y * 5.0) < lineWidth.x ? 1.0 : 0.0;
                vec4  redLines  = vec4(1.0, 0.0, 0.0, 1.0) * (lineHoriR + lineVertR);

                // Output to screen
                fragColor = redLines.x > 0.0 ? redLines : whiteLines;
                fragColor = vec4(In.color.rgb * 1.4, i * In.color.a * alpha);
                \n
            });
    frag = shdr_Header + "// SNParticles frag\n" + frag;

    return m_shCol.add("CoordGridGen", vert, frag);
}

}  // namespace ara