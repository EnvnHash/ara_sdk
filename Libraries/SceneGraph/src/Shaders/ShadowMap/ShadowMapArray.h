//
//  SHShadow.h
//
//  Created by Sven Hahne on 17.07.14.
//

#pragma once
#include "Shaders/ShadowMap/ShadowMap.h"

namespace ara {

class ShaderCollector;

class ShadowMapArray : public ShadowMap {
public:
    ShadowMapArray(CameraSet* cs, int scrWidth, int scrHeight, int32_t initNrLights);
    ~ShadowMapArray() override = default;

    void rebuildShader(uint nrLights);
    void rebuildFbo(int32_t nrLights);
    static void setShadowTexPar(GLenum type);
    void begin() override;
    void end() override;
    void clear() override;
    void setNrLights(int32_t nrLights);
    void setScreenSize(uint width, uint height) override;
    void bindDepthTexViews(GLuint baseTexUnit, uint nrTexs, uint texOffs) const;

private:
    GLint               m_maxShaderInvoc = 0;
    GLint               m_maxNumberLayers = 0;
    std::vector<GLuint> m_depthTexViews;
    ShaderCollector*    m_shCol = nullptr;
    int32_t             m_nrLights = 0;
};
}  // namespace ara
