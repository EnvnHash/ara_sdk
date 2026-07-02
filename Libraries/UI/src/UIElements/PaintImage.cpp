#include "PaintImage.h"
#include <UISharedRes.h>
#include <Shaders/ShaderCollector.h>
#include <Utils/Texture.h>
#include <GLBase.h>

namespace ara {

PaintImage::PaintImage() {
    setTypeName<PaintImage>();
    setName(getTypeName<PaintImage>());
}

void PaintImage::init() {
    Image::init();

    if (m_sharedRes && m_sharedRes->shCol) {
        std::string vert = ShaderCollector::getShaderHeader() + STRINGIFY(
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

        std::string frag = ShaderCollector::getShaderHeader() + STRINGIFY(
            layout(location = 0) out vec4 fragColor;
            in vec2 texCoord;
            struct Brush {
                float size;
                float hardness;
                vec4 color;
                float opacity;
            };
            uniform vec2 pos;
            layout(std140) uniform BrushBlock {
                Brush brush;
            };
            void main() {
                float dist = distance(texCoord, pos);
                float edge = clamp((1.0 - brush.hardness), 0.0001, 1.0);
                float falloff = 1.0 - smoothstep(brush.size * (1.0 - edge), brush.size, dist);
                fragColor = brush.color * falloff * brush.opacity;
                fragColor.a = brush.opacity;
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

void PaintImage::paint(const glm::vec2& mousePos) {
    if (!m_tex || !m_paintShader) {
        return;
    }

    if (!m_fbo) {
        m_fbo = std::make_unique<FBO>();
        m_fbo->fromTexMan(m_tex);
    }

    m_fbo->bind();
    m_paintShader->begin();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Set uniform block
    m_scaledBrush = m_brush.size / static_cast<float>(m_fbo->getWidth());
    m_brushBlock.update();
    m_brushBlock.bind();

    const auto pos = glm::vec2{mousePos.x / static_cast<float>(m_fbo->getWidth()) * 2.f - 1.f,
                          mousePos.y / static_cast<float>(m_fbo->getHeight()) * -2.f + 1.f};
    m_paintShader->setUniform2f("pos", pos.x, pos.y);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    m_fbo->unbind();
    
    // Request redraw of the UI
    if (m_sharedRes) {
        m_sharedRes->setDrawFlag(true);
    }
}

} // namespace ara
