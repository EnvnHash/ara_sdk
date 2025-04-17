//
// Created by user on 12.05.2021.
//

#include "MaskLayerRenderer.h"

using namespace std;
using namespace glm;

namespace ara {

MaskLayerRenderer::MaskLayerRenderer(MaskLayer *layer, uint32_t width, uint32_t height, ShaderCollector *shCol)
    : m_layer(layer), m_width(width), m_height(height), m_shCol(shCol) {
    init();
}

void MaskLayerRenderer::init() {
    if (!m_tessShader) initTesselationShader();
    if (!m_shdr) initPolygonShdr();

    // init the FBO
    if (m_width && m_height && m_shCol) {
        m_texShdr = m_shCol->getStdTex();
        m_inited  = true;
    }
}

void MaskLayerRenderer::initPolygonShdr() {
    std::string vert = STRINGIFY(layout(location = 0) in vec4 position; \n uniform mat4 m_pvm; void main() {
        \n gl_Position = m_pvm * position;
        \n
    });
    vert             = m_shCol->getShaderHeader() + "// MaskLayerRenderer polygon shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        layout(location = 0) out vec4 fragColor; \n layout(location = 1) out vec4 objMap; \n uniform vec4 color; \n uniform uint objId; \n void
            main() {
                \n       fragColor = color;
                \n float fObjId    = float(objId);
                objMap             = vec4(mod(floor(fObjId * 1.52587890625e-5), 256.0) / 255.0,
                                          mod(floor(fObjId * 0.00390625), 256.0) / 255.0, mod(fObjId, 256.0) / 255.0, 1.0);
                \n;
            });
    frag = m_shCol->getShaderHeader() + "// MaskLayerRenderer polygon shader, frag\n" + frag;

    m_shdr = m_shCol->add("MaskLayerRendererPoly", vert, frag);
}

// for bitmap filled polygon masks
void MaskLayerRenderer::initTesselationShader() {
    string vert = STRINGIFY(precise layout(location = 0) in vec4 position;\n
                                        layout(location = 2) in vec2 texCoord;\n
                                        out FS_TC { \n
                                        vec2 texCoord; \n
                                } vertOut; \n
                                        void main() { \n
                                        vertOut.texCoord = texCoord; \n
                                        gl_Position = vec4(position.xy, 0.f, 1.f); \n
                                });
    vert = m_shCol->getShaderHeader() + "// masklayer bitmap tesselation shader, vert\n" + vert;

    string cont = STRINGIFY(layout(vertices = 4) out;\n
                                        in FS_TC { \n
                                        vec2 texCoord; \n
                                } vertIn[]; \n  // note although TC execution is per vertex, inputs provide access to all patches vertices
                                        \n
                                        out TC_TE { \n
                                        vec2 texCoord; \n
                                } vertOut[]; \n // note: per vertex, can only we indexed via gl_InvocationID, TC execution is per vertex
                                        \n
                                        void main() { \n
                                        // if this triangle side is not an edge, subdivide it
                                        for (int i=0; i<4;i++) { \n
                                        gl_TessLevelOuter[i] = 16.0;  \n
                                }\n
                                        for (int i=0; i<2;i++) { \n
                                        gl_TessLevelInner[i] = 16.0;  \n
                                }
                                        gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position; \n
                                        vertOut[gl_InvocationID].texCoord = vertIn[gl_InvocationID].texCoord; \n
                                });
    cont = m_shCol->getShaderHeader() + "// masklayer bitmap tesselation shader, vert\n" + cont;

    string eval = STRINGIFY(layout(quads, equal_spacing, ccw) in;
                                        in TC_TE{\n
                        vec2 texCoord; \n
                } vertIn[]; \n  // access to all patches vertices from the TC stage, although TE is invocated per Vertex
                                        out TE_FS { \n
                                        vec2 texCoord; \n
                                } vertOut; \n
                                        uniform mat4 m_pvm;
                                        void main() {
                                        float u = gl_TessCoord.x;
                                        float omu = 1.0 - gl_TessCoord.x;
                                        float v = gl_TessCoord.y;
                                        float omv = 1.0 - gl_TessCoord.y;
                                        vec4 iPoint = omu * omv * gl_in[0].gl_Position +
                                        u * omv * gl_in[1].gl_Position +
                                        u * v * gl_in[2].gl_Position +
                                        omu * v * gl_in[3].gl_Position;
                                        vec2 texPoint = omu * omv * vertIn[0].texCoord +
                                        u * omv * vertIn[1].texCoord +
                                        u * v * vertIn[2].texCoord +
                                        omu * v * vertIn[3].texCoord;

                                        vertOut.texCoord = texPoint;
                                        gl_Position = iPoint;
                                });
    eval = m_shCol->getShaderHeader() + "// masklayer bitmap tesselation shader, vert\n" + eval;

    string frag = STRINGIFY(layout(location = 0) out vec4 fragColor; \n
                                        layout(location = 1) out vec4 objMap; \n
                                        in TE_FS { \n
                                        vec2 texCoord; \n
                                } vertIn; \n  // access to all patches vertices from the TC stage, although TE is invocated per Vertex
                                        uniform sampler2D tex;\n
                                        uniform int invert;\n
                                        uniform uint objId; \n
                                        void main() { \n
                                        vec4 col = texture(tex, vertIn.texCoord); \n
                                        float bw = dot(col.rgb, vec3(1.0)); \n
                                        col = vec4(vec3(0.0), (1.0 - bw) * col.a); \n
                                        fragColor = bool(invert) ? vec4(col.rgb, 1.0 - col.a) : col; \n
                                        \n
                                        float fObjId = float(objId); \n
                                        objMap = vec4(mod(floor(fObjId * 1.52587890625e-5), 256.0) / 255.0, mod(floor(fObjId * 0.00390625), 256.0) / 255.0, mod(fObjId, 256.0) / 255.0, 1.0);\n;
                                });
    frag = m_shCol->getShaderHeader() + "// masklayer bitmap  tesselation shader, vert\n" + frag;

    m_tessShader = m_shCol->add("MaskLayerDist", vert, cont, eval, std::string(), frag);
}

void MaskLayerRenderer::resize() {
    if (!m_inited) init();
}

void MaskLayerRenderer::update() {
    if (!m_inited) init();
}

void MaskLayerRenderer::render(uint32_t ind) {
    if (m_layer->getType() == maskType::Vector && m_layer->isValid()) {
        // glEnable(GL_POLYGON_SMOOTH);
        // glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE); // aliasing optimization

        m_shdr->begin();
        m_shdr->setUniform4fv("color", m_layer->getFillColorPtr());
        m_shdr->setUniformMatrix4fv("m_pvm", m_layer->getTransformPtr());
        m_shdr->setUniform1ui("objId", ind);

        if (!m_layer->isInverted() && m_layer->getPolygon()) {
            m_layer->getPolygon()->draw();
        } else if (m_layer->getPolygonInv() && m_layer->getPolygonInv()->getPoints(1) &&
                 m_layer->getPolygonInv()->getPoints(1)->size() > 2) {
            m_layer->getPolygonInv()->draw();
        }

        // glDisable(GL_POLYGON_SMOOTH);

    } else if (m_layer->getType() == maskType::Bitmap) {
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

        m_tessShader->begin();
        m_tessShader->setUniformMatrix4fv("m_pvm", m_layer->getTransformPtr());
        m_tessShader->setUniform1i("tex", 0);
        m_tessShader->setUniform1i("invert", m_layer->isInverted());
        m_tessShader->setUniform1ui("objId", ind);

        if (m_layer->getTexture()) {
            m_layer->getTexture()->bind(0);
        }

        if (m_layer->getPolygon()) {
            m_layer->getPolygon()->drawAsPatch();
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
}

}  // namespace ara