#include "PaintImageIdMap.h"
#include <UISharedRes.h>
#include <Shaders/ShaderCollector.h>
#include <Utils/Texture.h>
#include <GLBase.h>

using namespace glm;
using namespace std;

namespace ara {

PaintImageIdMap::PaintImageIdMap() {
    setTypeName<PaintImageIdMap>();
    setName(getTypeName<PaintImageIdMap>());
}

void PaintImageIdMap::init() {
    Image::init();

    if (m_sharedRes && m_sharedRes->shCol) {
        const auto vert = ShaderCollector::getShaderHeader() + STRINGIFY(
            layout(location = 0) in vec3 position;
            uniform float size;
            uniform vec2 pos;
            out vec2 texCoord;
            void main() {
                const vec2[4] quadVertices = vec2[4](vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.));
                texCoord = quadVertices[gl_VertexID];
                gl_Position = vec4(quadVertices[gl_VertexID], 0.0, 1.0);
            }
        );

        const auto frag = ShaderCollector::getShaderHeader() + STRINGIFY(
            layout(location = 0) out vec4 fragColor;

            in vec2 texCoord;
            struct Brush {
                float size;
                float hardness;
                vec4 color;
                float opacity;
            };
            uniform vec2 pos;
            uniform vec4 limits;
            uniform ivec2 texSize;
            uniform int maskBit;
            uniform sampler2D tex;
            layout(std140) uniform BrushBlock {
                Brush brush;
            };
            void main() {
                if (texCoord.x < limits.x || texCoord.x > limits.z || texCoord.y > limits.y || texCoord.y < limits.w) {
                    discard;
                }

                float dist = distance(texCoord, pos);
                float falloff = 1.0 - smoothstep(brush.size, brush.size, dist);
                if (falloff == 0.0) {
                    discard;
                }

                vec2 ntexCoord = texCoord * vec2(0.5) + vec2(0.5);
                vec4 col = texture(tex, ntexCoord);

                uint packedColor = packUnorm4x8(col);
                uint mask = uint(1) << maskBit;
                packedColor |= mask;

                fragColor = unpackUnorm4x8(packedColor);
            }
        );

        m_paintShader = m_sharedRes->shCol->add("paintImageIdMapShader", vert, frag);

        if (m_paintShader) {
            m_scaledBrush = m_brush.size;
            m_brushBlock.init(m_paintShader->getProgram(), "BrushBlock");
            m_brushBlock.addVarName("brush.size", &m_scaledBrush, GL_FLOAT);
            m_brushBlock.addVarName("brush.hardness", &m_brush.hardness, GL_FLOAT);
            m_brushBlock.addVarName("brush.color", &m_brush.color, GL_FLOAT_VEC4);
            m_brushBlock.addVarName("brush.opacity", &m_brush.opacity, GL_FLOAT);
        }

        const auto visVert = ShaderCollector::getShaderHeader() + STRINGIFY(
            layout(location = 0) in vec4 position;
            layout(location = 2) in vec2 texCoord;
            uniform mat4 m_pvm;
            uniform vec2 size;
            out vec2 tex_coord;
            const vec2[4] quadVertices = vec2[4](vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.));
            void main() {
                tex_coord = quadVertices[gl_VertexID];
                tex_coord.y = 1.0 - tex_coord.y;
                gl_Position = m_pvm * vec4(quadVertices[gl_VertexID] * size, 0.0, 1.0);
            }
        );

        const auto visFrag = ShaderCollector::getShaderHeader() + STRINGIFY(
            layout(location = 0) out vec4 fragColor;
            in vec2 tex_coord;
            uniform sampler2D tex;
            uniform int visualizationID;
            uniform vec4 nodeViewPort;\n

            void main() {
                if (gl_FragCoord.x < nodeViewPort.x
                    || gl_FragCoord.x > nodeViewPort.x + nodeViewPort.z
                    || gl_FragCoord.y < nodeViewPort.y
                    || gl_FragCoord.y > nodeViewPort.y + nodeViewPort.w) {
                   discard;
                }\n
                vec4 col = texture(tex, tex_coord);
                uint packedColor = packUnorm4x8(col);
                uint mask = uint(1) << visualizationID; // 0x00000020
                bool isSet = (packedColor & mask) != 0u;
                if (!isSet) {
                    discard;
                }
                fragColor = vec4(1.0, 1.0, 1.0, 1.0);
            }
        );

        m_visShader = m_sharedRes->shCol->add("paintImageIdMapVisShader", visVert, visFrag);
    }

    if (m_tex) {
        m_fbo = std::make_unique<FBO>();
        m_fbo->setGlbase(getSharedRes()->glbase);
        m_fbo->fromTexMan(m_tex);
    }
}

bool PaintImageIdMap::draw(uint32_t& objId) {
    return drawFunc(objId);
}

bool PaintImageIdMap::drawIndirect(uint32_t& objId) {
    if (m_drawMan) {
        m_drawMan->pushFunc([this, objId] {
            drawFunc(objId);
        });
    }
    return false;
}

bool PaintImageIdMap::drawFunc(const uint32_t&) {
    if (!m_visShader || !m_fbo) {
        return false;
    }

    m_visShader->begin();
    m_visShader->setUniformMatrix4fv("m_pvm", getMVPMatPtr());
    m_visShader->setUniform2fv("size", &getSize()[0]);
    m_visShader->setUniform1i("visualizationID", m_visualizationID);
    m_visShader->setUniform1i("tex", 0);
    m_visShader->setUniform4fv("nodeViewPort", &m_sc[0]);

    glActiveTexture(GL_TEXTURE0);
    m_tex->bind(0);

    glBindVertexArray(*m_sharedRes->nullVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    return false;
}

void PaintImageIdMap::paint(const vec2& mousePos) {
    if (!(m_tex || m_texId) || !m_paintShader || !m_sharedRes || m_locked) {
        return;
    }

    m_fbo->bind();
    m_paintShader->begin();

    glBlendFunc(GL_ONE, GL_ZERO);

    const vec2 fboSize{ static_cast<float>(m_fbo->getWidth()), static_cast<float>(m_fbo->getHeight()) };

    // Set uniform block
    m_scaledBrush = m_brush.size * 2.f / fboSize.x;
    m_brushBlock.update();
    m_brushBlock.bind();

    std::array transPos = { mousePos, vec2(0.0), vec2(m_secSize) };
    if (m_secSize.x != 0 && m_secSize.y != 0) {
        for (auto &it : transPos) {
            it = vec2(m_secPos) + it / m_size * vec2(m_secSize); // fbo pos in pixels
            it = vec2{ it.x / fboSize.x * 2.f - 1.f,
                        it.y / fboSize.y * -2.f + 1.f }; // normalized fbo pos
        }
    }

    m_paintShader->setUniform2f("pos", transPos[0].x, transPos[0].y);
    m_paintShader->setUniform4f("limits", transPos[1].x, transPos[1].y, transPos[2].x, transPos[2].y);
    m_paintShader->setUniform1i("maskBit", m_drawID);
    m_paintShader->setUniform2i("texSize", static_cast<int>(m_tex->getWidth()), static_cast<int>(m_tex->getHeight()));
    m_paintShader->setUniform1i("tex", 0);

    if (m_tex) {
        m_tex->bind(0);
    } else if (m_texId) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texId);
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    m_fbo->unbind();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_sharedRes->setDrawFlag(true); // Request redrawing of the UI
}

} // namespace ara
