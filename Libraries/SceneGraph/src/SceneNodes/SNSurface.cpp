#include "SceneNodes/SNSurface.h"

#include <GLBase.h>

using namespace glm;
using namespace std;

namespace ara {
SNSurface::SNSurface(sceneData* sd) : SceneNode(sd), m_paramChanged(true) {
    m_transFlag |= GLSG_NO_SCALE;
    m_cullFace    = false;
    m_drawIndexed = true;
    setName(getTypeName<SNSurface>());
}

void SNSurface::draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo) {
    if (!m_fbSet) {
        m_surfGen.setUseGL32Fallback(m_glbase->getUseFallback());
        m_fbSet = true;
    }

    if (m_reqRebuildMesh) {
        rebuildMesh();
        m_reqRebuildMesh = false;
    }

    if (m_vao && m_renderPassEnabled[pass]) {
        uint texCnt = 0;
        glBlendFunc(m_blendSrc, m_blendDst);

        if (!getTextures()->empty()) {
            for (auto t = 0; t < getTextures()->size(); t++) {
                glActiveTexture(GL_TEXTURE0 + t);
                shader->setUniform1i("tex" + std::to_string(t), t);
                getTextures()->at(t)->bind();
                ++texCnt;
            }

            shader->setUniform1i("hasTexture", 1);
        } else {
            shader->setUniform1i("hasTexture", 0);
        }

        float gridCellSize = 100.f;  // mm
        shader->setUniform1i("drawGridTexture", m_drawGridTex);

        if (m_surfGen.m_type == SurfaceGenerator::FLAT)
            shader->setUniform2f("gridNrSteps", m_surfGen.m_width / gridCellSize, m_surfGen.m_height / gridCellSize);
        else if (m_surfGen.m_type == SurfaceGenerator::PANADOME)
            shader->setUniform2f("gridNrSteps",
                                  (m_surfGen.m_rightAng - m_surfGen.m_leftAng) / 360.f * static_cast<float>(M_TWO_PI) *
                                      m_surfGen.m_radius / gridCellSize,
                                  (m_surfGen.m_topAng - m_surfGen.m_bottAng) / 360.f * static_cast<float>(M_TWO_PI) *
                                      m_surfGen.m_radius / gridCellSize);
        else if (m_surfGen.m_type == SurfaceGenerator::DOME)
            shader->setUniform2f("gridNrSteps", m_surfGen.m_radius * static_cast<float>(M_TWO_PI) / gridCellSize,
                                  m_surfGen.m_radius * static_cast<float>(M_PI) / gridCellSize);
        else if (m_surfGen.m_type == SurfaceGenerator::CYLINDER)
            shader->setUniform2f("gridNrSteps",
                                  (m_surfGen.m_rightAng - m_surfGen.m_leftAng) / 360.f * static_cast<float>(M_TWO_PI) *
                                      m_surfGen.m_radius / gridCellSize,
                                  m_surfGen.m_height / gridCellSize);

        shader->setUniform1f("gridBgAlpha", m_gridBgAlpha);
        shader->setUniform2f("resolution", cs->getActFboSize()->x, cs->getActFboSize()->y);

        m_cullFace ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
        m_depthTest ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
        m_drawIndexed ? m_vao->drawElements(GL_TRIANGLES) : m_vao->draw(GL_TRIANGLES);
    }
}

bool SNSurface::rebuildMesh() {
    if (hasParentNode() && s_sd) {
        if (!m_paramChanged) {
            return true;
        }

        m_surfGen.init(m_surfGen.m_type, &m_glbase->shaderCollector());
        m_vao                                           = m_surfGen.m_vao;
        m_paramChanged                                  = false;
        s_sd->reqRenderPasses->at(GLSG_SHADOW_MAP_PASS) = true;
        return true;
    } else {
        return false;
    }
}

}  // namespace ara
