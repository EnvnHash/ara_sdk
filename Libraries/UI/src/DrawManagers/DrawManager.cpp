//
// Created by user on 30.12.2021.
//

#include "DrawManager.h"

#include "GLBase.h"
#include "UINode.h"

using namespace std;
using namespace glm;

namespace ara {

bool DrawManager::rebuildVaos() {
    if (!m_shCol) return false;

    for (auto &ds : m_drawSets) {
        if (ds.divData.empty()) {
            continue;
        }

        if (!ds.vao.isInited()) {
            ds.vao.setStoreMode(GL_STREAM_DRAW);
            ds.vao.init("position:4f,texCoord:2f,color:4f,aux0:4f,aux1:4f,aux2:4f,aux3:4f", true, true);  // use interleave buffers
        }

        if (ds.vao.getNrVertices() < ds.vaoSize) {
            ds.vao.resize(ds.vaoSize);
        }

        if (!ds.vao.getElementBuffer() || ds.vao.getNrIndices() < ds.indOffs) {
            ds.vao.setElemIndices(ds.indOffs, nullptr);
        }

        // divVao is arranged as interleave data, so just run through the DivData and upload
        auto ptr = (DivVaoData *)ds.vao.getMapBuffer(CoordType::Position);
        if (ptr) {
            for (auto &it : ds.divData) {
                std::copy((*it).begin(), (*it).end(), ptr);
                ptr += it->size();
            }

            VAO::unMapBuffer();
        }

        // build the indices
        auto ip = (GLuint *)ds.vao.mapElementBuffer();
        if (ip) {
            for (auto &it : ds.divIndices) {
                std::copy(it->begin(), it->end(), ip);
                ip += it->size();
            }

            VAO::unMapElementBuffer();
        }
    }

    return true;
}

void DrawManager::update() {
    for (auto &ds : m_drawSets) {
        if (ds.updtNodes.empty() || ds.divData.empty()) {
            continue;
        }

        // update VBO
        auto ptr = static_cast<DivVaoData*>(ds.vao.getMapBuffer(CoordType::Position));
        if (ptr) {
            for (auto &it : ds.updtNodes) {
                ptr += it.second;
                if (it.first) {
                    std::copy(it.first->begin(), it.first->end(), ptr);
                }
                ptr -= it.second;
            }
            VAO::unMapBuffer();
        }
        ds.updtNodes.clear();
    }
}

Shaders *DrawManager::getShader(DrawSet &ds) {
    std::string shdrName = "DrawManager_" + std::to_string(ds.fontTex.size()) + "_" + std::to_string(ds.textures.size());

    m_shdr = m_shCol->get(shdrName);
    if (m_shdr) {
        return m_shdr;
    }

    std::string vert = STRINGIFY(layout(location = 0) in vec4 position;\n
        layout(location = 2) in vec2 texCoord;  \n
        layout(location = 3) in vec4 color;     \n
        layout(location = 6) in vec4 aux0;      \n   // Div: border color
        layout(location = 7) in vec4 aux1;      \n   // Div: border width, border height, border radiusX, border radiusY
        layout(location = 8) in vec4 aux2;      \n   // borderAliasX, borderAliasY, objId, zPos
        layout(location = 9) in vec4 aux3;      \n   // x:UINode Type (0=Div, 1=Label, 2=Image), w:alpha
        out VS_FS {                             \n
        \t   vec2 tex_coord;                    \n
        \t   vec4 color;                        \n
        \t   vec4 aux0;                         \n
        \t   vec4 aux1;                         \n
        \t   vec4 aux2;                         \n
        \t   vec4 aux3;                         \n
        } vout;                                 \n
        void main() {                           \n
        \t   vout.tex_coord = texCoord;         \n
        \t   vout.color = color;                \n
        \t   vout.aux0 = aux0;                  \n
        \t   vout.aux1 = aux1;                  \n
        \t   vout.aux2 = aux2;                  \n
        \t   vout.aux3 = aux3;                  \n
        \t   gl_Position = position;            \n
        });

    vert = m_shCol->getShaderHeader() + "// " + shdrName + ", vert\n" + vert;

    std::string frag = "layout(location = 0) out vec4 fragColor;\n";
    frag += STRINGIFY(
         in VS_FS {\n
             vec2 tex_coord;
             vec4 color;
             vec4 aux0;
             vec4 aux1;
             vec4 aux2;
             vec4 aux3;
        } vin;\n
        uniform float maxObjId; \n);

    if (!ds.fontTex.empty()) {
        frag += "uniform sampler3D fontTex[" + std::to_string(std::max<int>(1, (int)ds.fontTex.size())) + "]; \n";
        frag += "uniform float fontTexSize[" + std::to_string(std::max<int>(1, (int)ds.fontTex.size())) + "]; \n";
    }

    if (!ds.textures.empty()) {
        frag += "uniform sampler2D textures[" + std::to_string(std::max<int>(1, (int)ds.textures.size())) + "]; \n";
    }

    frag += STRINGIFY(
        vec4 div(vec4 col, vec4 borderColor) { \n
            // origin to center, map all quadrants to the first quadrant
            vec2 tn = abs(vin.tex_coord - 0.5); \n

            // check if inside border and or corner area
            bool isBorder   = tn.x >= (0.5 - vin.aux1.x) || tn.y >= (0.5 - vin.aux1.y); \n
            bool isCorner   = tn.x >= (0.5 - vin.aux1.z) && tn.y >= (0.5 - vin.aux1.w); \n
            bool validBr    = vin.aux1.z != 0.0 && vin.aux1.w != 0.0; \n  // check if the borderRadius is not 0
            bool validBw    = vin.aux1.x != 0.0 && vin.aux1.y != 0.0;  \n  // check if the borderRadius is not 0

            // corner calculation, move left lower corner edge to the center and scale by the size of the borderRadius
            // corner size = borderRadius
            vec2 cc     = (tn - (0.5 - vin.aux1.zw)) / vin.aux1.zw; \n
            float r     = length(cc);                               \n
            float angle = atan(cc.y, cc.x);                         \n

            // since borderRadius pixels translate to different normalized values for x and y we have to calculate a
            // normalized radius and scale it by borderWidth
            vec2 rr = vec2(cos(angle), sin(angle)); \n
            vec2 bw = vin.aux1.xy / (validBr ? vin.aux1.zw : vec2(1.0)); \n  // borderWidth in corner area coordinates (ignored if borderRadius == 0)
            vec2 aw = vin.aux2.xy / (validBr ? vin.aux1.zw : vec2(1.0)); \n  // aliasWidth in corner area coordinates (ignored if borderRadius == 0)
            vec2 awH = aw * 0.5; \n
            vec4 fc  = isBorder ? borderColor : col; \n  // standard border (not corner)
            fc = isCorner && validBw ? mix(col, borderColor, smoothstep(length(rr * (max(1.0 - bw - aw, 0.0))), length(rr * max(1.0 - bw, 0.0)), r)) : fc; \n  // inner edge
            return mix(fc, vec4(0.0), isCorner ? smoothstep(length(rr * (1.0 - awH)), length(rr * (1.0 + awH)), r) : 0.0); \n  // outer edge
        }\n
        \n
        vec4 genQuad() { return vin.color; }\n);

    if (!ds.fontTex.empty()) {
        frag += STRINGIFY(vec4 label() { \n
                int li = int(vin.aux2.x); \n
                return vec4(
                    vin.color.rgb,
                    vin.color.a *
                        texture(fontTex[li], vec3(vin.tex_coord, vin.aux2.y / fontTexSize[li] + 0.5 / fontTexSize[li])).r); \n  // needs "half pixel offset" same as 2d tex access
        }\n);
    }

    if (!ds.textures.empty()) {
        frag += STRINGIFY(
            vec4 image() {                                                                                              \n
                bool valid = (vin.aux0.x >= 0.0) && (vin.aux0.x < 1.0) && (vin.aux0.y >= 0.0) && (vin.aux0.y < 1.0);    \n
                if (!valid) discard;                                                                                    \n
                int   li      = int(vin.aux2.z);                                                                        \n
                ivec2 texSize = textureSize(textures[li], 0);                                                           \n
                bool  p       = int(int(vin.aux2.w) & 16) != 0;                                                         \n
        );

#ifdef __ANDROID__
                // illogical shader linker error here on android gles when fontTex size == 0
        frag += STRINGIFY(
                vec4 lod = textureLod(textures[li], vec2(vin.aux0.z, 1.0 - vin.aux0.w), vin.aux3.z);                    \n
                vec4 procCol = (vin.aux2.z < 0.0) ? vec4(0.0) : p ? texelFetch(textures[li], ivec2(ivec2(vin.aux0.zw) * texSize), 0) : lod; \n);
#else
        frag += "vec4 procCol = (vin.aux2.z < 0.0) ? vec4(0.0) : p "
                    "? texelFetch(textures[li], ivec2(ivec2(vin.aux0.zw) * texSize), int(vin.aux3.z))"
                    ": textureLod(textures[li], vec2(vin.aux0.z, 1.0 - vin.aux0.w), vin.aux3.z);  \n";
#endif
        frag += STRINGIFY(
                // procCol.rgb *= vin.color.rgb;                                                                           \n
                procCol = (int(vin.aux3.y) == 8) ? vec4(procCol.r, procCol.r, procCol.r, procCol.a) : procCol;          \n
                if (procCol.a < 0.001) discard;                                                                         \n
                return procCol;                                                                                         \n
            }\n
            \n
            vec4 imageFrame() {\n
                vec2 ts = vec2(vin.aux0.xy);    \n  // the size of the element in virtual pixels
                vec2 v = vec2(vin.aux0.zw);     \n  // texCoords top/left in virtual pixels
                vec2 sp = vec2(vin.aux1.xy);    \n  // section pos in texture pixels
                vec2 ss = vec2(vin.aux1.zw);    \n  // section size, (section = area of the frame that contains a corner)
                vec2 sd = vec2(vin.aux2.xy);    \n    // section distance, (area between the corners that gets repeated)
                vec2 pv = vec2(v.x < ss.x ? v.x : v.x >= ts.x - ss.x ? ss.x + sd.x + v.x - ts.x + ss.x : mod(v.x - ss.x, sd.x) + ss.x,\n
                    v.y < ss.y ? v.y : v.y >= ts.y - ss.y ? ss.y + sd.y + v.y - ts.y + ss.y : mod(v.y - ss.y, sd.y) + ss.y );\n
                ivec2 pp = ivec2(pv + sp); \n
                return texelFetch(textures[int(vin.aux2.z)],
                                     ivec2(pp.x, textureSize(textures[int(vin.aux2.z)], 0).y - pp.y - 1), 0); \n
            }\n);
    }

    frag += "void main() {\n";
    frag += "\tint nodeType = int(vin.aux3.x); \n";
    frag += "\tvec4 out_col = (nodeType == 0) ? div(vin.color, vin.aux0)";
    if (!ds.fontTex.empty()) {
        frag += ": (nodeType == 1) ? label()";
    }
    if (!ds.textures.empty()) {
        frag += ": (nodeType == 2) ? div(image(), vin.color)";
        frag += ": (nodeType == 3) ? imageFrame()";
    }
    frag += " : (nodeType == 4) ? genQuad() : vec4(0.0);\n";
    frag += "\tfragColor = vec4(out_col.rgb, out_col.a * vin.aux3.w);\n";
    frag += "}\n";

    frag = m_shCol->getShaderHeader() + "// " + shdrName + " shader, frag\n" + frag;

    return m_shCol->add(shdrName, vert, frag);
}

void DrawManager::draw() {
    if (!m_shCol) {
        return;
    }

    for (auto &ds : m_drawSets) {
        if (ds.func) {
            ds.func();
            continue;
        }

        ds.shdr = getShader(ds);

        if (!ds.shdr || ds.vaoSize == 0) {
            continue;
        }

        ds.shdr->begin();

        if (!ds.layerSizes.empty() && !ds.fontTex.empty()) {
            ds.shdr->setUniform1fv("fontTexSize", &ds.layerSizes[0], static_cast<int>(ds.layerSizes.size()));
            ds.shdr->setUniform1iv("fontTex", &ds.layerUnits[0], static_cast<int>(ds.layerUnits.size()));

            for (auto &ft : ds.fontTex) {
                glActiveTexture(GL_TEXTURE0 + ft.second);
                glBindTexture(GL_TEXTURE_3D, ft.first);
            }
        }

        if (!ds.textures.empty()) {
            if (m_texUnitMap.size() != ds.textures.size()) {
                m_texUnitMap.resize(ds.textures.size());
            }

            for (auto &tx : ds.textures) {
                if (tx.second >= m_texUnitMap.size()) {
                    continue;
                }
                m_texUnitMap[tx.second] = static_cast<GLint>(ds.fontTex.size() + tx.second);
                glActiveTexture(GL_TEXTURE0 + m_texUnitMap[tx.second]);
                glBindTexture(GL_TEXTURE_2D, tx.first);
            }
            ds.shdr->setUniform1iv("textures", &m_texUnitMap[0], static_cast<int>(m_texUnitMap.size()));
        }
        ds.vao.drawElements(GL_TRIANGLES, nullptr, GL_TRIANGLES, static_cast<GLsizei>(ds.indOffs));
    }
}

list<DrawSet>::reference DrawManager::push(IndDrawBlock &block, UINode *node) {
    m_drawSets.back().divData.emplace_back(&block.vaoData);

    // save the actual position in the DrawSets VBO for later update
    m_vaoOffset = m_drawSets.back().vaoSize;
    m_drawSets.back().vaoSize += (GLuint)block.vaoData.size();

    // update indices
    for (int i = 0; i < block.indices.size(); i++) {
        block.indices[i] = stdQuadInd[i % 6] + int((float)i / 6.f) * 4 + m_vaoOffset;
    }

    m_drawSets.back().divIndices.emplace_back(&block.indices);
    m_drawSets.back().indOffs += (GLuint)block.indices.size();

    block.vaoOffset = m_vaoOffset;

    // keep track of all nodes drawn indirectly, in order to invalidate their references to the drawSet when deleted
    m_nodeList.emplace_back(node);

    return m_drawSets.back();
}

float DrawManager::pushFont(GLuint texId, float nrLayers) {
    if (m_drawSets.empty()) {
        return 0.f;
    }

    size_t newFontTexSize = m_drawSets.back().fontTex.size() +
        static_cast<size_t>(m_drawSets.back().fontTex.find(texId) == m_drawSets.back().fontTex.end());

    // check if the maximum number of parallel texture units are used if this is the case, create a new draw set
    if (newFontTexSize + m_drawSets.back().textures.size() > (size_t)m_glbase->maxTexUnits()) m_drawSets.emplace_back();

    // in case this is a new texId or the nrLayers has changed
    if (m_drawSets.back().fontTex.find(texId) == m_drawSets.back().fontTex.end()) {
        m_drawSets.back().fontTex[texId] = (int)m_drawSets.back().fontTex.size();  // associate a texUnit to this new texId

        if (m_drawSets.back().layerSizes.size() < m_drawSets.back().fontTex.size()) {
            m_drawSets.back().layerSizes.emplace_back((float)nrLayers);
            m_drawSets.back().layerUnits.emplace_back(m_drawSets.back().fontTex[texId]);
        } else if (m_drawSets.back().layerSizes[m_drawSets.back().fontTex[texId]] != nrLayers) {
            m_drawSets.back().layerSizes[m_drawSets.back().fontTex[texId]] = nrLayers;
        }
    }

    return (float)m_drawSets.back().fontTex[texId];
}

float DrawManager::pushTexture(GLuint texId) {
    if (m_drawSets.empty()) {
        return 0.f;
    }
    return pushTexture(m_drawSets.back(), texId);
}

float DrawManager::pushTexture(DrawSet &ds, GLuint texId) {
    size_t newTexSize = ds.textures.size() + static_cast<size_t>(ds.textures.find(texId) == ds.textures.end());

    // check if the maximum number of parallel texture units are used if this is the case, create a new draw set
    if (newTexSize + ds.textures.size() > (size_t)m_glbase->maxTexUnits()) {
        m_drawSets.emplace_back();
    }

    // in case this is a new texId or the nrLayers has changed
    if (ds.textures.find(texId) == ds.textures.end()) {
        ds.textures[texId] = (int)ds.textures.size();  // associate a texUnit to this new texId
    }

    return (float)ds.textures[texId];
}

void DrawManager::popTexture(DrawSet &ds, GLuint texId) {
    if (ds.textures.find(texId) != ds.textures.end()) {
        ds.textures.erase(texId);
    }
}

void DrawManager::pushFunc(const std::function<void()>& f) {
    m_drawSets.emplace_back();
    m_drawSets.back().func = f;
    m_drawSets.emplace_back();
}

void DrawManager::clear() {
    // remove shaders if necessary
    if (m_shCol) {
        for (auto &ds : m_drawSets) {
            if (ds.shdr) {
                m_shCol->deleteShader(ds.shdr);
            }
        }
    }

    for (auto &it : m_nodeList) {
        if (it) {
            it->clearDs();
        }
    }

    m_nodeList.clear();
    m_drawSets.clear();
}

void DrawManager::clearFonts() {
    for (auto& it : m_drawSets) {
        it.fontTex.clear();
    }
}

}  // namespace ara
