//
// Created by user on 05.10.2020.
//

#include "CrossHair.h"

using namespace std;
using namespace glm;

namespace ara {

CrossHair::CrossHair() : Div() {
    setName(getTypeName<CrossHair>());
    excludeFromObjMap(true);
#ifndef FORCE_INMEDIATEMODE_RENDERING
    m_drawImmediate = false;
#endif
}

CrossHair::CrossHair(std::string* styleClass) : Div() {
    setName(getTypeName<CrossHair>());
    addStyleClass(std::move(*styleClass));
    excludeFromObjMap(true);
#ifndef FORCE_INMEDIATEMODE_RENDERING
    m_drawImmediate = false;
#endif
}

CrossHair::CrossHair(std::string&& styleClass) : Div() {
    setName(getTypeName<CrossHair>());
    addStyleClass(std::move(styleClass));
    excludeFromObjMap(true);
#ifndef FORCE_INMEDIATEMODE_RENDERING
    m_drawImmediate = false;
#endif
}

void CrossHair::init() {
    std::string vert = STRINGIFY(layout(location = 0) in vec4 position; \n layout(location = 2) in vec2 texCoord; \n uniform mat4 m_pvm; \n uniform vec2 size; \n out vec2 tex_coord;\n const vec2[4] quadVertices = vec2[4](vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.));\n void
                                     main() {
                                         \n tex_coord = quadVertices[gl_VertexID];
                                         \n gl_Position =
                                             m_pvm * vec4(quadVertices[gl_VertexID] * size, position.z, 1.0);
                                         \n
                                     });

    vert = getSharedRes()->shCol->getShaderHeader() + "// ui crosshair shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        layout(location = 0) out vec4 fragColor;\n
        in vec2 tex_coord; \n
        uniform vec2 borderRadius; \n
        uniform vec2 borderWidth; \n
        uniform vec4 borderColor; \n
        uniform vec2 alias; \n
        uniform vec4 color;\n
        uniform vec4 bgColor;\n
        uniform float radius;\n
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

            // since border-radius pixels translate to different normalized values for x and y
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

            // circle
            float dist = distance(tex_coord, vec2(0.5));
            float delta = fwidth(dist)*2.5;
            float a = smoothstep(radius - delta * 2.0, radius - delta*1.5, dist);
            float a2 = smoothstep(radius, radius - delta*0.5, dist);
            a = a * a2;

            // cross
            vec2 dp = fwidth(tex_coord);
            dp.x /= aspect;
            vec2 aliasRng = dp * 0.25;

            float cV = smoothstep(0.5 - aliasRng.x -dp.x, 0.5 - dp.x, tex_coord.x)
                    * smoothstep(0.5 + aliasRng.x + dp.x, 0.5 + dp.x, tex_coord.x);

            float cH = smoothstep(0.5 - aliasRng.y - dp.y, 0.5 - dp.y, tex_coord.y)
                    * smoothstep(0.5 + aliasRng.y + dp.y, 0.5 + dp.y, tex_coord.y);

            fragColor = mix(vec4(0.0), color, cV + cH + a) + baseCol;
        });

    frag = getSharedRes()->shCol->getShaderHeader() + "// ui crosshair shader, frag\n" + frag;

    m_crossHairShdr = getSharedRes()->shCol->add("CrossHair", vert, frag);
}

bool CrossHair::draw(uint32_t* objId) { return drawFunc(objId); }

bool CrossHair::drawIndirect(uint32_t* objId) {
    if (m_sharedRes && m_sharedRes->drawMan) {
        m_tempObjId = *objId;
        m_sharedRes->drawMan->pushFunc([this] {
            m_dfObjId = m_tempObjId;
            drawFunc(&m_dfObjId);
        });
    }

    return false;
}

bool CrossHair::drawFunc(uint32_t* objId) {
#ifndef ARA_USE_GLES31
    glBlendFuncSeparatei(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
#endif
    m_crossHairShdr->begin();
    m_crossHairShdr->setUniformMatrix4fv("m_pvm", getMVPMatPtr());
    m_crossHairShdr->setUniform2fv("size", &m_size[0]);
    m_crossHairShdr->setUniform4fv("color", &m_color[0]);
    m_crossHairShdr->setUniform4fv("bgColor", &m_bgColor[0]);
    m_crossHairShdr->setUniform2fv("borderWidth", &getBorderWidthRel()[0]);
    m_crossHairShdr->setUniform4fv("aux0", &m_borderColor[0]);
    m_crossHairShdr->setUniform2fv("borderRadius", &getBorderRadiusRel()[0]);
    m_crossHairShdr->setUniform2fv("alias", &getBorderAliasRel()[0]);
    m_crossHairShdr->setUniform1f("radius", 0.5f);
    m_crossHairShdr->setUniform1f("aspect", m_aspect);

    glBindVertexArray(*m_sharedRes->nullVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

#ifndef ARA_USE_GLES31
    glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

    return false;  // dont't count up objId
}

}  // namespace ara