#include "PaintImage.h"
#include <UISharedRes.h>
#include <Shaders/ShaderCollector.h>
#include <Utils/Texture.h>
#include <GLBase.h>

using namespace glm;
using namespace std;

namespace ara {

PaintImage::PaintImage() {
    setTypeName<PaintImage>();
    setName(getTypeName<PaintImage>());
}

void PaintImage::init() {
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
            layout(std140) uniform BrushBlock {
                Brush brush;
            };
            void main() {
                if (texCoord.x < limits.x || texCoord.x > limits.z || texCoord.y > limits.y || texCoord.y < limits.w) {
                    discard;
                }
                float dist = distance(texCoord, pos);
                float edge = clamp((1.0 - brush.hardness), 0.0001, 1.0);
                float falloff = 1.0 - smoothstep(brush.size * (1.0 - edge), brush.size, dist);
                if (falloff == 0.0) {
                    discard;
                }
                fragColor = vec4(brush.color.rgb, brush.opacity * falloff * brush.color.a);
            }
        );

        m_paintShader = m_sharedRes->shCol->add("paintImageShader", vert, frag);

        if (m_paintShader) {
            m_scaledBrush = m_brush.size;
            m_brushBlock.init(m_paintShader->getProgram(), "BrushBlock");
            m_brushBlock.addVarName("brush.size", &m_scaledBrush, GL_FLOAT);
            m_brushBlock.addVarName("brush.hardness", &m_brush.hardness, GL_FLOAT);
            m_brushBlock.addVarName("brush.color", &m_brush.color, GL_FLOAT_VEC4);
            m_brushBlock.addVarName("brush.opacity", &m_brush.opacity, GL_FLOAT);
        }
    }

    if (m_tex) {
        m_fbo = std::make_unique<FBO>();
        m_fbo->setGlbase(getSharedRes()->glbase);
        m_fbo->fromTexMan(m_tex);
    }
}

void PaintImage::mouseDown(hidData& data) {
    Image::mouseDown(data);
    paint(data.mousePosNodeRel);
}

void PaintImage::mouseDrag(hidData& data) {
    Image::mouseDrag(data);
    paint(data.mousePosNodeRel);
}

void PaintImage::paint(const vec2& mousePos) {
    if (!(m_tex || m_texId) || !m_paintShader || !m_sharedRes || m_locked) {
        return;
    }

    if (!m_fbo) {
        m_fbo = std::make_unique<FBO>();
        if (m_tex) {
            m_fbo->fromTexMan(m_tex);
        } else if (m_texId) {
            m_fbo->setGlbase(m_sharedRes->glbase);
            m_fbo->fromTex(m_texId, m_texSize.x, m_texSize.y, GL_RGBA8, 1, GL_LINEAR, GL_LINEAR);
        }
    }

    m_fbo->bind();
    m_paintShader->begin();

    glBlendEquation(m_paintMode == mode::add ? GL_FUNC_ADD : GL_FUNC_REVERSE_SUBTRACT);
    glBlendFunc(m_paintMode == mode::add ? GL_ONE : GL_SRC_ALPHA, GL_ONE);

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

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_fbo->unbind();

    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_sharedRes->setDrawFlag(true); // Request redrawing of the UI
}

void PaintImage::setBrushSize(const float brushSize) {
    m_brush.size = brushSize;
}

void PaintImage::setBrushHardness(const float hardness) {
    m_brush.hardness = hardness;
}

void PaintImage::saveToFile(const std::filesystem::path &filename) const {
    if (!m_fbo) {
        return;
    }
    m_fbo->saveToFile(filename, 0, GL_RGBA8);
}

} // namespace ara
