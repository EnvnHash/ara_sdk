//
// Created by user on 09.09.2020.
//

#include "GLRendDispPart.h"

#include <GLBase.h>

#include <utility>

#include "GLRenderer.h"

using namespace glm;
using namespace std;

namespace ara {

GLRendDispPart::GLRendDispPart()
    : m_globalColor(vec3(1.f, 1.f, 1.f)), m_gamma(vec3(1.f)), m_gammaP(vec3(1.f)), m_gradient(vec3(1.f)),
      m_plateau(vec3(1.f)), m_dispNameCol(vec4(1.f, 1.f, 1.f, 1.f)), m_renderVwf(true), m_fontSize(45) {}

GLRendDispPart::~GLRendDispPart() {
    close();  // calls GLRenderer::close
}

bool GLRendDispPart::init(std::string name, glm::ivec2 pos, glm::ivec2 dimension, bool hidden) {
    GLRenderer::init(name, pos, dimension, hidden);
    m_procSteps[Callbacks] = ProcStep{true, [this] { iterateGlCallbacks(); }};
    m_procSteps[Draw]      = ProcStep{true, [this] { return drawContent(); }};

    s_inited = true;
    return true;
}

/**
m_format: typical RGBA32F or RGB32F,
warp: warpingTexture from VWF
bland: blendingTexture from VWF
name:
*/
void GLRendDispPart::initFromVwf(int width, int height, GLenum format, void *warp, void *blend, const char *name,
                                 int idx) {
    m_wbSetWarp[idx]  = warp;
    m_wbSetBlend[idx] = blend;

    // create a FBO same size as output, do this apart from the gRsrc container,
    // since we need to share this as a void* with the stage view = avoid to
    // ashare the whole ctxRsrc
    // if (m_projectorOutputFbos[string(name)])
    // m_projectorOutputFbos[string(name)].reset();
    // m_projectorOutputFbos[string(name)] = make_unique<FBO>((int)m_viewport.z,
    // (int)m_viewport.w, 1);

    if (m_vwfExportFbo) m_vwfExportFbo.reset();
    m_vwfExportFbo =
        make_unique<FBO>(m_glbase, width, height, GL_RGBA32F, GL_TEXTURE_2D, false, 1, 1, 2, GL_CLAMP_TO_BORDER, false);

    if (m_vwfExportBlendFbo) m_vwfExportBlendFbo.reset();
    m_vwfExportBlendFbo =
        make_unique<FBO>(m_glbase, width, height, GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 2, GL_CLAMP_TO_BORDER, false);

    // ? is this needed
    // s_viewPort = glm::vec4(0.f, 0.f, (float)s_viewPort.z,
    // (float)s_viewPort.w);

    // GLenum intFormat = 0 != (wb->header.flags & FLAG_SP_WARPFILE_HEADER_3D) ?
    // GL_RGB32F : GL_RGBA32F; GLenum extFormat = 0 != (wb->header.flags &
    // FLAG_SP_WARPFILE_HEADER_3D) ? GL_RGB : GL_RGBA; GLenum extFormat = format
    // == GL_RGB32F ? GL_RGB : GL_RGBA;
}

void GLRendDispPart::initGL() {
    GLRenderer::initGL();

    if (m_warpFboSummed) m_warpFboSummed.reset();
    m_warpFboSummed = make_unique<FBO>(m_glbase, (int)s_viewPort.z, (int)s_viewPort.w, GL_RGBA32F, GL_TEXTURE_2D, false,
                                       1, 1, 2, GL_CLAMP_TO_BORDER, false);

    if (m_contentFbo) m_contentFbo.reset();
    m_contentFbo = make_unique<FBO>(m_glbase, (int)s_viewPort.z, (int)s_viewPort.w, GL_RGBA8, GL_TEXTURE_2D, false, 1,
                                    1, 2, GL_CLAMP_TO_BORDER, false);

    if (m_outputFbo) m_outputFbo.reset();
    m_outputFbo = make_unique<FBO>(m_glbase, (int)s_viewPort.z, (int)s_viewPort.w, GL_RGBA8, GL_TEXTURE_2D, false, 1, 1,
                                   2, GL_CLAMP_TO_BORDER, false);
}

bool GLRendDispPart::drawContent() {
    if (!s_inited || !m_stdCol || !m_stdQuad) return false;

    m_outputFbo->bind();
    m_outputFbo->clear();

    if (!m_shutterActive) {
        if (m_extDrawFunc) {
            m_contentFbo->bind();
            m_contentFbo->clear();
            (*m_extDrawFunc)(m_contentFbo->getWidth(), m_contentFbo->getHeight(), m_nullVao);
            m_contentFbo->unbind();
        }
        if (m_renderVwf) drawVwf();
        if (m_drawDispName) drawDisplayName();
    }

    m_outputFbo->unbind();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_stdTex->begin();
    m_stdTex->setIdentMatrix4fv("m_pvm");
    m_stdTex->setUniform1i("tex", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_outputFbo->getColorImg());

    m_stdQuad->draw();

    return true;
}

void GLRendDispPart::drawVwf() {
    updateWarpBlendTextures();

    if (!m_vwfShader || !m_stdQuad) return;

    // update mask if needed
    if (m_stackRenderer.getStack()) {
        if (m_stackRenderer.isInited()) {
            if (m_stackRenderer.getWidth() != (uint32_t)m_dim.x || m_stackRenderer.getHeight() != (uint32_t)m_dim.y) {
                m_stackRenderer.resize((uint32_t)m_dim.x, (uint32_t)m_dim.y, false);
                m_stackRenderer.update();
            }

            if (m_reqMaskStackRendererUpdt) {
                m_stackRenderer.update();
                m_reqMaskStackRendererUpdt = false;
            }
        } else {
            m_stackRenderer.setSize((uint32_t)m_dim.x, (uint32_t)m_dim.y);
            m_stackRenderer.setShaderCollector(&s_shCol);
            m_stackRenderer.init(m_glbase);
            m_stackRenderer.update();
        }
    }

    bool useMask = m_stackRenderer.isInited() && m_stackRenderer.getStack()->getNrLayers();

    // glClearColor(1.f, 0.f, 0.f, 1.f);
    //  glClear(GL_COLOR_BUFFER_BIT);

    m_vwfShader->begin();
    m_vwfShader->setUniform1i("samContent", 0);
    m_vwfShader->setUniform1i("samWarp", 1);
    m_vwfShader->setUniform1i("samBlend", 2);
    m_vwfShader->setUniform1i("mask", 3);
    m_vwfShader->setUniform1ui("useMask", useMask);
    m_vwfShader->setUniform1ui("bFlipWarpH", 1);
    m_vwfShader->setUniform1ui("bDoNotBlend", !(m_displayPartBlendFbo && m_dpHasValidCalib) || !m_blendingActive);
    m_vwfShader->setUniform1ui("bBorder", 0);
    m_vwfShader->setUniform3fv("colorCorr", &m_globalColor[0]);
    m_vwfShader->setUniform3fv("gamma", &m_gamma[0]);
    m_vwfShader->setUniform3fv("gammaP", &m_gammaP[0]);
    m_vwfShader->setUniform3fv("gradient", &m_gradient[0]);
    m_vwfShader->setUniform3fv("plateau", &m_plateau[0]);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_contentFbo->getColorImg());

    glActiveTexture(GL_TEXTURE1);
    if (m_gotValidFboSummed && m_warpFboSummed) {
        glBindTexture(GL_TEXTURE_2D, m_warpFboSummed->getColorImg());
    } else if (m_projectorWarpFbo) {
        glBindTexture(GL_TEXTURE_2D, m_projectorWarpFbo->getColorImg());
    }

    if (m_displayPartBlendFbo) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_displayPartBlendFbo->getColorImg());
    }

    if (useMask) {
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, m_stackRenderer.getFbo()->getColorImg());
    }

    if (m_stdQuad) m_stdQuad->draw();
}

void GLRendDispPart::drawDisplayName() {
    if (!m_typo) loadTypo(filesystem::current_path() / "resdata" / "Fonts" / "NeoSans" / "NeoSansMedium.TTF");

    if (m_dispNameChanged) {
        m_renderedTextSize = m_typo->getPixTextWidth(m_dispName.c_str(), m_fontSize);
        m_bakedTextHeight  = m_typo->m_getPixExtent("Wj",
                                                    m_fontSize)
                                .y;  // Let's use some major "Wj" to keep the height constant
                                     // and as spread as possible
        m_dispNameChanged = false;
    }

    int xPos = static_cast<int>((s_viewPort.z - m_renderedTextSize.x) * 0.5f);
    int yPos = static_cast<int>((s_viewPort.w - m_renderedTextSize.y) * 0.5f);

    // black background
    // note: TypoGlyphMap aligns to base line, but we need to have the centers
    // of the background rect and the typo texture match
    mat4 backGr =
        glm::scale(vec3(m_renderedTextSize.x / s_viewPort.z * 1.3f, m_renderedTextSize.y / s_viewPort.w * 1.3f, 1.f));

    m_stdCol->begin();
    m_stdCol->setUniformMatrix4fv("m_pvm", &backGr[0][0]);
    m_stdQuad->draw();

    // create a texture with the name of the monitor, (note if the name doesn't
    // change this is done only once)
    m_typo->print(xPos, (int)(yPos + m_bakedTextHeight + m_typo->getDescent(m_fontSize)), m_dispName, m_fontSize,
                  &m_dispNameCol[0]);
}

void GLRendDispPart::updateWarpBlendTextures() {
    if (!m_warpFboSummed || !m_projectorWarpFbo || !m_displayCanvWarpFbo) {
        return;
    }

    m_warpFboSummed->bind();
    m_warpFboSummed->clear();

    m_flatWarpShader->begin();
    m_flatWarpShader->setUniform1i("warpProjector", 0);
    m_flatWarpShader->setUniform1i("warpCanvas", 2);

    m_flatWarpShader->setUniform1i("useProjWarp", (int)m_projectorWarpActive);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_projectorWarpFbo->getColorImg());

    m_flatWarpShader->setUniform1i("useCalib", (int)(m_displayPartWarpFbo && m_dpHasValidCalib));

    if (m_displayPartWarpFbo) {
        m_flatWarpShader->setUniform1i("warpCalib", 1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_displayPartWarpFbo->getColorImg());
    }

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_displayCanvWarpFbo->getColorImg());

    m_stdQuad->draw();

    m_warpFboSummed->unbind();
    m_gotValidFboSummed = true;
}

void GLRendDispPart::exportVwf(vector<condition_variable> &cond, vector<mutex> &mtx, vector<bool> &done, uint32_t idx) {
    // if (!done[idx]) // be sure it runs only once
    //{
    //  render warping to s_fbo
    m_vwfExportFbo->bind();
    m_exportShader->begin();
    m_exportShader->setUniform1i("warpToolUV", 0);
    m_exportShader->setUniform1i("samWarp", 1);
    m_exportShader->setUniform1ui("bBorder", 0);
    m_exportShader->setUniform1ui("useWarpToolUV", m_postWarpUVFbo ? 1 : 0);

    if (m_postWarpUVFbo) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_postWarpUVFbo->getColorImg(0));
    }

    if (m_displayPartWarpFbo) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_displayPartWarpFbo->getColorImg());
    }

    if (m_displayPartBlendFbo) {
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_displayPartBlendFbo->getColorImg());
    }

    m_flipQuad->draw();
    m_vwfExportFbo->unbind();

    // render blending to FBO
    m_vwfExportBlendFbo->bind();
    m_exportBlendShader->begin();
    m_exportBlendShader->setUniform1i("samBlend", 0);
    m_exportBlendShader->setUniform3fv("gamma", &m_gamma[0]);
    m_exportBlendShader->setUniform3fv("gammaP", &m_gammaP[0]);
    m_exportBlendShader->setUniform3fv("gradient", &m_gradient[0]);
    m_exportBlendShader->setUniform3fv("plateau", &m_plateau[0]);

    // m_blendTex.bind(0);
    if (m_displayPartBlendFbo) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_displayPartBlendFbo->getColorImg());
    }

    m_flipQuad->draw();
    m_vwfExportBlendFbo->unbind();

    // download the warping texture, brute force!!!!
#ifndef ARA_USE_GLES31
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_vwfExportFbo->getColorImg(0));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, m_wbSetWarp[idx]);

    // download the blending, brute force!!!!
    glBindTexture(GL_TEXTURE_2D, m_vwfExportBlendFbo->getColorImg(0));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_wbSetBlend[idx]);
#endif

    std::unique_lock<std::mutex> l(mtx[idx]);
    done[idx] = true;
    cond[idx].notify_all();
    //}
}

void GLRendDispPart::setDisplayName(std::string dispName) {
    std::unique_lock<std::mutex> lock(s_drawMtx);
    m_dispName        = std::move(dispName);
    m_dispNameChanged = true;
}

bool GLRendDispPart::closeEvtLoopCb() {
#ifdef ARA_USE_GLFW

    // check if the window was already closed;
    if (!s_inited) return true;

    if (m_winHandle) m_winHandle->stopDrawThread();  // also calls close() on GLFWWindow

    if (m_glbase) {
        // remove all gl resources, since all gl contexts are shared, this can
        // be done on any context
        m_glbase->addGlCbSync([this]() {
            m_stdQuad.reset();
            m_flipQuad.reset();
            s_shCol.clear();
            glDeleteVertexArrays(1, &m_nullVao);
            if (m_typo) m_typo.reset();
            if (m_stdQuad) m_stdQuad.reset();
            if (m_flipQuad) m_flipQuad.reset();
            if (m_warpFboSummed) m_warpFboSummed->remove();
            if (m_contentFbo) m_contentFbo->remove();
            if (m_outputFbo) m_outputFbo->remove();
            if (m_vwfExportFbo) m_vwfExportFbo.reset();
            if (m_vwfExportBlendFbo) m_vwfExportBlendFbo.reset();
            m_stackRenderer.freeGLResources();

            return true;
        });

        // clearGlCbQueue();

        // if this was called from key event callback, it will cause a crash,
        // because we are still iterating through GLFWWindow member variables
        m_glbase->getWinMan()->removeWin(m_winHandle,
                                         false);  // removes the window from the ui_windows array
    }

    s_inited = false;
#endif
    return true;
}

}  // namespace ara