//
//  SHShadow.h
//
//  Created by Sven Hahne on 17.07.14.
//

#pragma once

#include "GLUtils/FastBlurMem.h"
#include "Shaders/ShadowMap/ShadowMap.h"

namespace ara {
class ShadowMapVsmArray : public ShadowMap {
public:
    ShadowMapVsmArray(CameraSet* cs, int scrWidth, int scrHeight, int32_t initNrLights);
    ~ShadowMapVsmArray() override = default;

    void  rebuildShader(uint nrLights);
    void  rebuildFbo(uint nrLights);
    static void  setShadowTexPar(GLenum type);
    void  begin() override;
    void  end() override;
    void  clear() override;
    void  setNrLights(uint nrLights);
    void  setScreenSize(uint width, uint height) override;
    void  bindDepthTexViews(GLuint baseTexUnit, uint nrTexs, uint texOffs) const;
    void  blur() const;

    [[nodiscard]] GLint getTex() const { return m_fboBlur ? m_fboBlur->getLastResult() : 0; }

private:
    GLint                        m_maxShaderInvoc = 0;
    float                        m_blurAlpha = 0.f;
    std::vector<GLuint>          m_depthTexViews;
    ShaderCollector*             m_shCol = nullptr;
    uint                         m_nrLights = 0;
    std::unique_ptr<FastBlurMem> m_fboBlur;
};
}  // namespace ara
