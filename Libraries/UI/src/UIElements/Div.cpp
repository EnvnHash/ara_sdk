//
// Created by user on 05.10.2020.
//

#include <UIElements/Div.h>
#include <UIWindow.h>
#include <DrawManagers/DrawManager.h>

using namespace std;
using namespace glm;

namespace ara {

Div::Div() {
    setName(getTypeName<Div>());
#ifndef FORCE_INMEDIATEMODE_RENDERING
    m_indDrawBlock.stdInit();
    m_drawImmediate = false;
#endif
}

Div::Div(const std::string& styleClass) {
    setName(getTypeName<Div>());
    UINode::addStyleClass(styleClass);
#ifndef FORCE_INMEDIATEMODE_RENDERING
    m_indDrawBlock.stdInit();
    m_drawImmediate = false;
#endif
}

void Div::updateDrawData() {
    if (m_drawImmediate) {
        if (!m_uniBlock.isInited()) {
            m_uniBlock.addVarName("m_pvm", getMVPMatPtr(), GL_FLOAT_MAT4);
            m_uniBlock.addVarName("size", &m_size[0], GL_FLOAT_VEC2);
            m_uniBlock.addVarName("zPos", &m_zPos, GL_FLOAT);
            m_uniBlock.addVarName(m_objIdName, &m_objIdMin, GL_UNSIGNED_INT);
            m_uniBlock.addVarName("borderRadius", &getBorderRadiusRel()[0], GL_FLOAT_VEC2);
            m_uniBlock.addVarName("borderWidth", &getBorderWidthRel()[0], GL_FLOAT_VEC2);
            m_uniBlock.addVarName("borderColor", &m_borderColor[0], GL_FLOAT_VEC4);
            m_uniBlock.addVarName("alias", &getBorderAliasRel()[0], GL_FLOAT_VEC2);
            m_uniBlock.addVarName("color", &m_bgColor[0], GL_FLOAT_VEC4);
            m_uniBlock.addVarName("alpha", &m_absoluteAlpha, GL_FLOAT);
            // m_uniBlock.addVarName("resolution", &m_bgColor[0],
            // GL_FLOAT_VEC4);
        }

        m_uniBlock.update();
    } else {
        if (m_indDrawBlock.vaoData.empty()) {
            return;
        }

        m_uvSize.x = 1.f;
        m_uvSize.y = 1.f;

        auto dIt = m_indDrawBlock.vaoData.begin();

        // calculate position in normalized screen coordinates
        for (auto &it : stdQuadVertices) {
            dIt->pos = m_mvp * vec4(it * m_size, 0.f, 1.0);
            ++dIt;
        }

        dIt = m_indDrawBlock.vaoData.begin();

        for (int i = 0; i < 2; i++) {
            m_divRefSize[i] = ((dIt + 3)->pos[i] - dIt->pos[i]) * 0.5f * m_viewPort[i + 2];
        }

        m_divRefSize[1] *= -1.f;

        int i = 0;
        for (auto &it : stdQuadVertices) {
            limitDrawVaoToBounds(dIt, m_divRefSize, m_uvDiff, m_scIndDraw, m_viewPort);  // scissoring
            dIt->texCoord = it;

            if (m_uvDiff.x != 0.f || m_uvDiff.y != 0.f) {
                limitTexCoordsToBounds(&dIt->texCoord[0], i, m_uvSize, m_uvDiff);
            }

            dIt->color  = m_bgColor;
            dIt->aux0   = m_borderColor;
            dIt->aux1.x = getBorderWidthRel().x;
            dIt->aux1.y = getBorderWidthRel().y;
            dIt->aux1.z = getBorderRadiusRel().x;
            dIt->aux1.w = getBorderRadiusRel().y;
            dIt->aux2.x = getBorderAliasRel().x;
            dIt->aux2.y = getBorderAliasRel().y;
            dIt->aux2.z = m_excludeFromObjMap ? 0.f : static_cast<float>(m_objIdMin); // obsolete
            dIt->aux2.w = m_zPos; // obsolete
            dIt->aux3.x = 0.f;  // type indicator (0=Div)
            dIt->aux3.w = m_absoluteAlpha;

            ++dIt;
            ++i;
        }
    }
}

void Div::pushVaoUpdtOffsets() {
    if (m_indDrawBlock.drawSet) {
        m_indDrawBlock.drawSet->updtNodes.emplace_back(m_indDrawBlock.getUpdtPair());
    }
}

bool Div::draw(uint32_t *objId) {
    if (!m_uniBlock.isInited()) {
        m_uniBlock.init(m_shdr->getProgram(), "nodeData");
        updateDrawData();
    }

    m_uniBlock.bind();

    if (m_hasDepth) {
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
    }

    m_shdr->begin();
    glBindVertexArray(*m_sharedRes->nullVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (m_hasDepth) {
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
    }

    return true;  // count up objId
}

bool Div::drawIndirect(uint32_t *objId) {
    if (m_sharedRes && m_sharedRes->drawMan) {
        m_indDrawBlock.drawSet = &m_sharedRes->drawMan->push(m_indDrawBlock, this);
    }

    return true;  // count up objId
}

}  // namespace ara