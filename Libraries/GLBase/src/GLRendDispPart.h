// Created by user on 09.09.2020.
//

#pragma once

#include <Conditional.h>
#include <GeoPrimitives/Quad.h>
#include <Shaders/Shaders.h>
#include <Utils/FBO.h>
#include <Utils/MaskStackRenderer.h>
#include <Utils/Texture.h>

#include "GLBase.h"
#include "GLRenderer.h"

namespace ara {

class GLRendDispPart : public GLRenderer {
public:
    GLRendDispPart();
    ~GLRendDispPart() override;
    bool init(std::string name, glm::ivec2 pos, glm::ivec2 dimension, bool hidden = false) override;
    void initFromVwf(int width, int height, GLenum format, void *warp, void *blend, const char *name, int idx);
    void initGL() override;
    bool drawContent();
    void drawVwf();
    void drawDisplayName();
    void updateWarpBlendTextures();
    void exportVwf(std::vector<std::condition_variable> &cond, std::vector<std::mutex> &mtx, std::vector<bool> &done,
                   uint32_t idx);
    void setDisplayName(std::string dispName);

    FBO *getOutput() { return m_outputFbo ? m_outputFbo.get() : nullptr; }
    void setShutter(bool val) { m_shutterActive = val; }
    void setBlendingActive(bool val) { m_blendingActive = val; }
    void setProjWarpFbo(FBO *warpFbo) { m_projectorWarpFbo = warpFbo; }
    void setProjMaskStack(MaskStack *stack) {
        if (stack) m_stackRenderer.setStack(stack);
    }
    void updateProjMask() { m_reqMaskStackRendererUpdt = true; }
    // void setProjMaskFbo(FBO* maskFbo)     { m_projectorMaskFbo = maskFbo; }
    void setProjWarpActive(bool val) { m_projectorWarpActive = val; }
    void setDcWarpFbo(FBO *warpFbo) { m_displayCanvWarpFbo = warpFbo; }
    void setDpHasValidCalib(bool val) { m_dpHasValidCalib = val; }
    void setDpWarpFbo(FBO *warpFbo) { m_displayPartWarpFbo = warpFbo; }
    void setDpBlendFbo(FBO *warpFbo) { m_displayPartBlendFbo = warpFbo; }
    void setRenderVwf(bool val) { m_renderVwf = val; }
    void setDrawDispName(bool val) { m_drawDispName = val; }
    void setGlobalColor(glm::vec3 col) { m_globalColor = col; }
    void setGamma(glm::vec3 val) { m_gamma = val; }
    void setGammaP(glm::vec3 val) { m_gammaP = val; }
    void setGradient(glm::vec3 val) { m_gradient = val; }
    void setPlateau(glm::vec3 val) { m_plateau = val; }
    void setExtDrawFunc(std::function<void(int, int, GLuint)> *f) { m_extDrawFunc = f; }
    //    virtual void close(bool direct=false);
    bool closeEvtLoopCb() override;
    //    void freeGLResources();
    FBO *m_postWarpFbo = nullptr;

private:
    bool m_dispNameChanged          = false;
    bool m_blendingActive           = true;
    bool m_renderVwf                = false;
    bool m_drawDispName             = false;
    bool m_gotValidFboSummed        = false;
    bool m_dpHasValidCalib          = false;
    bool m_shutterActive            = false;
    bool m_projectorWarpActive      = true;
    bool m_reqMaskStackRendererUpdt = false;

    int                   m_exportVwf = -1;
    int                   m_fontSize;
    int                   m_bakedTextHeight{};
    std::string           m_dispName;
    std::map<int, void *> m_wbSetWarp;
    std::map<int, void *> m_wbSetBlend;
    std::filesystem::path m_dataPath;

    // Texture
    // m_warpTex; Texture
    // m_blendTex;

    Shaders *m_stdColBrd = nullptr;

    std::unique_ptr<FBO> m_vwfExportFbo;
    std::unique_ptr<FBO> m_vwfExportBlendFbo;
    std::unique_ptr<FBO> m_warpFboSummed;
    std::unique_ptr<FBO> m_contentFbo;
    std::unique_ptr<FBO> m_outputFbo;
    FBO                 *m_postWarpUVFbo       = nullptr;
    FBO                 *m_projectorWarpFbo    = nullptr;
    FBO                 *m_projectorMaskFbo    = nullptr;
    FBO                 *m_displayCanvWarpFbo  = nullptr;
    FBO                 *m_displayPartWarpFbo  = nullptr;
    FBO                 *m_displayPartBlendFbo = nullptr;
    MaskStackRenderer    m_stackRenderer;

    bool m_resinited = false;
    bool m_vwfInited = false;

    glm::vec4 m_dispNameCol;

    glm::vec3 m_globalColor;
    glm::vec3 m_gamma;
    glm::vec3 m_gammaP;
    glm::vec3 m_gradient;
    glm::vec3 m_plateau;

    glm::vec2 m_renderedTextSize{};

    std::function<void(int, int, GLuint)> *m_extDrawFunc = nullptr;
};
}  // namespace ara