//
// Created by user on 21.01.2022.
//

#include "DrawManagerGizmo.h"

#include "Shaders/ShaderCollector.h"

using namespace std;
using namespace glm;

namespace ara {

Shaders* DrawManagerGizmo::getShader(DrawSet& ds) {
    std::string shdrName = "DrawManagerGizmo_" + std::to_string(ds.textures.size());

    m_shdr = m_shCol->get(shdrName);

    if (m_shdr) {
        return m_shdr;
    }

    std::string vert = STRINGIFY(layout(location = 0) in vec4 position;\n
        layout(location = 2) in vec2 texCoord; \n
        layout(location = 3) in vec4 color; \n
        layout(location = 6) in vec4 aux0; \n   // Div: border color
        layout(location = 7) in vec4 aux1; \n   // Div: border width, border height, border radiusX, border radiusY
        layout(location = 8) in vec4 aux2; \n   // borderAliasX, borderAliasY, objId, zPos
        layout(location = 9) in vec4 aux3; \n   // x:UINode Type (0=Div, 1=Label, 2=Image)
        out VS_FS {\n
            vec2 tex_coord; \n
            vec4 color;\n
            vec4 aux0; \n
            vec4 aux1; \n
            vec4 aux2; \n
            vec4 aux3; \n
        } vout;\n
        void main() { \n
            vout.tex_coord = texCoord; \n
            vout.color = color; \n
            vout.aux0 = aux0; \n
            vout.aux1 = aux1; \n
            vout.aux2 = aux2; \n
            vout.aux3 = aux3; \n
            gl_Position = position;\n
        });

    vert = "// DrawManagerGizmo div shader, vert\n" + ShaderCollector::getShaderHeader() + vert;

    std::string frag = "layout(location = 0) out vec4 fragColor;\n";

    frag += STRINGIFY(
    in VS_FS {\n
        vec2 tex_coord; \n
        vec4 color;\n
        vec4 aux0; \n
        vec4 aux1; \n
        vec4 aux2; \n
        vec4 aux3; \n
    } vin;\n
    uniform sampler2D optTex; \n
    uniform float maxObjId; \n);

    if (!ds.textures.empty()) {
        frag += "uniform sampler2D textures[" + std::to_string(ds.textures.size()) + "]; \n";
    }

    frag += STRINGIFY(vec4 div() {\n
        // origin to center, map all quadrants to the first quadrant
        vec2 tn = abs(vin.tex_coord - 0.5);\n

        // check if inside border and or corner area
        bool isBorder = tn.x >= (0.5 - vin.aux1.x) || tn.y >= (0.5 - vin.aux1.y);\n
        bool  isCorner = tn.x >= (0.5 - vin.aux1.z) && tn.y >= (0.5 - vin.aux1.w);\n
        bool  validBr  = vin.aux1.z != 0.0 && vin.aux1.w != 0.0;\n  // check if the borderRadius is not 0
        bool validBw = vin.aux1.x != 0.0 && vin.aux1.y != 0.0;\n  // check if the borderRadius is not 0

        // corner calculation, move left lower corner edge to the center
        // and scale by the size of the borderRadius
        // corner size = borderRadius
        vec2 cc    = (tn - (0.5 - vin.aux1.zw)) / vin.aux1.zw;\n
        float r     = length(cc);\n
        float angle = atan(cc.y, cc.x);\n

        // since borderRadius pixels translate to different normalized
        // values for x and y we have to calculate a normalized radius and
        // scale it by borderWidth
        vec2 rr = vec2(cos(angle), sin(angle)):\n

        // borderWidth in corner area coordinates (ignored if borderRadius == 0)
        vec2 bw = vin.aux1.xy / (validBr ? vin.aux1.zw : vec2(1.0));\n

        // aliasWidth in corner area coordinates (ignored if borderRadius == 0)
        vec2 aw  = vin.aux2.xy / (validBr ? vin.aux1.zw : vec2(1.0));\n
        vec2  awH = aw * vec2(0.5);\n

        // standard border (not corner)
        vec4 fc = isBorder ? vin.aux0 : vin.color;\n

        // inner edge
        // fc = isBorder && validBw ? mix(color, aux0,
        // smoothstep(length(rr*(1.0-bw-aw)), length(rr*(1.0-bw)), r) ) : fc;
        fc = isCorner && validBw
                    ? mix(vin.color, vin.aux0,
                        smoothstep(length(rr * (max(1.0 - bw - aw, 0.0))), length(rr * max(1.0 - bw, 0.0)), r))
                    : fc;\n

        // outer edge
        return mix(fc, vec4(0.0),
                    isCorner ? smoothstep(length(rr * (1.0 - awH)), length(rr * (1.0 + awH)), r) : 0.0);\n
    }\n
    \n);

    if (!ds.textures.empty()) {
        frag += STRINGIFY(vec4 image() {
            bool valid = vin.aux0.x >= 0.0 && vin.aux0.x < 1.0 && vin.aux0.y >= 0.0 && vin.aux0.y < 1.0;\n
            if (!valid) discard;\n
            ivec2 texSize = textureSize(textures[int(vin.aux1.y)], 0);\n
            bool  tp      = int(int(vin.aux1.x) & 16) != 0;\n
            int   texSel  = int(vin.aux1.y);\n
            vec4  procCol = tp ? texelFetch(textures[texSel], ivec2(vin.aux0.zw * vec2(texSize)), 0)
                                  : textureLod(textures[texSel], vec2(vin.aux0.z, 1.0 - vin.aux0.w), vin.aux1.w);\n
            procCol.rgb *= vin.color.rgb;\n
            procCol = (int(vin.aux3.y) == 8) ? vec4(procCol.r, procCol.r, procCol.r, procCol.a) : procCol;\n
            if (procCol.a < 0.001) discard;\n
            return procCol;\n
        }\n);
    } else {
        frag += "vec4 image() { return vec4(0.0); }\n";
    }

    frag += STRINGIFY(void main() {\n
        int nodeType = int(vin.aux3.x);\n

        vec4 pc = nodeType == 0 ? div() : (nodeType == 2 ? image() : vec4(0.0));\n
        vec4  op = texelFetch(optTex, ivec2(gl_FragCoord.xy), 0);
        // compare depth
        bool depthSkip = vin.aux2.w > op.y;
        // compare ids
        bool sameAxis = int(vin.aux3.z) == int(round(op.x * 10.0));
        if (depthSkip && !sameAxis) discard;
        fragColor = pc;\n
    });

    frag = "// DrawManagerGizmo div shader, frag\n" + ShaderCollector::getShaderHeader() + frag;

    return m_shCol->add(shdrName, vert, frag);
}

void DrawManagerGizmo::draw() {
    if (!m_shCol) {
        return;
    }

    for (auto& ds : m_drawSets) {
        if (ds.func) {
            ds.func();
            continue;
        }
        ds.shdr = getShader(ds);

        if (!ds.shdr && ds.vaoSize == 0) {
            continue;
        }

        ds.shdr->begin();

        if (!ds.textures.empty()) {
            if (m_texUnitMap.size() != ds.textures.size()) {
                m_texUnitMap.resize(ds.textures.size());
            }

            for (const auto& tx : ds.textures) {
                if (tx.second >= m_texUnitMap.size()) {
                    continue;
                }

                m_texUnitMap[tx.second] = static_cast<GLint>(ds.fontTex.size() + tx.second);
                glActiveTexture(GL_TEXTURE0 + m_texUnitMap[tx.second]);
                glBindTexture(GL_TEXTURE_2D, tx.first);
            }

            ds.shdr->setUniform1iv("textures", &m_texUnitMap[0], static_cast<int>(m_texUnitMap.size()));
        }

        if (m_optTex != 0) {
            glActiveTexture(GL_TEXTURE0 + static_cast<int>(ds.textures.size()));
            ds.shdr->setUniform1i("optTex", static_cast<int>(ds.textures.size()));
            glBindTexture(GL_TEXTURE_2D, m_optTex);
        }

        ds.vao.drawElements(GL_TRIANGLES, nullptr, GL_TRIANGLES, static_cast<GLsizei>(ds.indOffs));
    }
}

}  // namespace ara