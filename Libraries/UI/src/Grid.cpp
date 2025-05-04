//
// Created by user on 05.10.2020.
//

#include "Grid.h"

#include <DrawManagers/DrawManager.h>

using namespace std;
using namespace glm;

namespace ara {

Grid::Grid() {
    setName(getTypeName<Grid>());
    excludeFromObjMap(true);
#ifndef FORCE_INMEDIATEMODE_RENDERING
    m_drawImmediate = false;
#endif
}

Grid::Grid(const std::string& styleClass) {
    setName(getTypeName<Grid>());
    UINode::addStyleClass(styleClass);
    excludeFromObjMap(true);
#ifndef FORCE_INMEDIATEMODE_RENDERING
    m_drawImmediate = false;
#endif
}

void Grid::init() {
    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position; \n
        layout(location = 2) in vec2 texCoord; \n
        uniform mat4 m_pvm; \n
        uniform vec2 size; \n
        out vec2 tex_coord;\n
        const vec2[4] quadVertices = vec2[4](vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.));\n
        void main() { \n
            tex_coord = quadVertices[gl_VertexID];\n
            gl_Position = m_pvm * vec4(quadVertices[gl_VertexID] * size, position.z, 1.0); \n
        });

    vert = ShaderCollector::getShaderHeader() + "// ui grid shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        layout(location = 0) out vec4 fragColor;\n
        in vec2 tex_coord; \n
        uniform vec2 borderRadius; \n
        uniform vec2 borderWidth; \n
        uniform vec4 borderColor; \n
        uniform vec2 alias; \n
        uniform vec4 color;\n
        uniform vec4 bgColor;\n
        uniform float nrSep;\n
        uniform float aspect;\n
        uniform vec2 size; \n // in pixels
        // uniform vec2 halfPix; \n // in pixels
        //uniform vec2 aliasRng; \n // in pixels
        const float pi_2 = 1.57079632679489661923;\n
        \n
        void main() {\n
            // origin to center, map all quadrants to the first quadrant
            vec2 tn = abs(tex_coord - 0.5);

            // check if inside border and or corner area
            bool isBorder = tn.x >= (0.5 - borderWidth.x) || tn.y >= (0.5 - borderWidth.y);
            bool isCorner = tn.x >= (0.5 - borderRadius.x) && tn.y >= (0.5 - borderRadius.y);
            bool validBr = borderRadius.x != 0.0 && borderRadius.y != 0.0; // check if the borderRadius is not 0
            bool validBw = borderWidth.x != 0.0 && borderWidth.y != 0.0; // check if the borderRadius is not 0

            // corner calculation, move left lower corner edge to the center
            // and scale by the size of the borderRadius
            // corner size = borderRadius
            vec2 cc = (tn - (0.5 - borderRadius)) / borderRadius;
            float r = length(cc);
            float angle = atan(cc.y,cc.x);

            // since borderradius pixels translate to different normalized values for x and y
            // we have to calculate a normalized radius and scale it by borderWidth
            vec2 rr = vec2(cos(angle), sin(angle));

            // borderWidth in corner area coordinates (ignored if borderRadius == 0)
            vec2 bw = borderWidth / (validBr ? borderRadius : vec2(1.0));

            // aliasWidth in corner area coordinates (ignored if borderRadius == 0)
            vec2 aw = alias / (validBr ? borderRadius : vec2(1.0));
            vec2 awH = aw * 0.5;

            // standard border (not corner)
            vec4 fc = isBorder ? borderColor : bgColor;

            // inner edge
            fc = isCorner && validBw ? mix(bgColor, borderColor, smoothstep(length(rr * (max(1.0-bw-aw,0.0))), length(rr * max(1.0-bw, 0.0)), r) ) : fc;

            // outer edge
            vec4 baseCol = mix(fc, vec4(0.0), isCorner ? smoothstep(length(rr *(1.0-awH)), length(rr*(1.0+awH)), r) : 0.0);

            // cross
            vec2 fp = mod(tex_coord * vec2(nrSep), vec2(1.0));
            vec2 dp = fwidth(fp);
            float cV = float(fp.x < dp.x || fp.x > (1.0 -dp.x));
            float cH = float(fp.y < dp.y || fp.y > (1.0 -dp.y));
            fragColor = mix(vec4(0.0), color, cV + cH) + baseCol;
        });

    frag = ShaderCollector::getShaderHeader() + "// ui grid shader, frag\n" + frag;

    m_gridShdr = getSharedRes()->shCol->add("UIGrid", vert, frag);
}

bool Grid::draw(uint32_t* objId) {
    return drawFunc(objId);
}

bool Grid::drawIndirect(uint32_t* objId) {
    if (m_sharedRes && m_sharedRes->drawMan) {
        m_tempObjId = *objId;
        m_sharedRes->drawMan->pushFunc([this] {
            m_dfObjId = m_tempObjId;
            drawFunc(&m_dfObjId);
        });
    }

    return false;
}

bool Grid::drawFunc(uint32_t* objId) {
#ifndef ARA_USE_GLES31
    glBlendFuncSeparatei(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
#endif
    m_gridShdr->begin();
    m_gridShdr->setUniformMatrix4fv("m_pvm", getMVPMatPtr());
    m_gridShdr->setUniform2fv("size", &m_size[0]);
    m_gridShdr->setUniform4fv("color", &m_color[0]);
    m_gridShdr->setUniform4fv("bgColor", &m_bgColor[0]);
    m_gridShdr->setUniform2fv("borderWidth", &getBorderWidthRel()[0]);
    m_gridShdr->setUniform4fv("aux0", &m_borderColor[0]);
    m_gridShdr->setUniform2fv("borderRadius", &getBorderRadiusRel()[0]);
    m_gridShdr->setUniform2fv("alias", &getBorderAliasRel()[0]);
    m_gridShdr->setUniform1f("nrSep", 4.0f);
    m_gridShdr->setUniform1f("aspect", m_aspect);

    glBindVertexArray(*m_sharedRes->nullVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // glBindVertexArray(0);

#ifndef ARA_USE_GLES31
    glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

    return false;  // don't count up objId
}

}  // namespace ara