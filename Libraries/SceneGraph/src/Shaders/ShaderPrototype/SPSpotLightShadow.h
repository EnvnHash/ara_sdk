/**
 *	 SPSpotLightShadow.h
 *
 *  Created by Sven Hahne on 14.08.17
 */

#pragma once

#include "Shaders/ShaderProperties/LightShaderProperties.h"
#include "Shaders/ShaderPrototype/ShaderProto.h"
#include "Shaders/ShaderUtils/ShaderBuffer.h"
#include "Shaders/ShadowMap/ShadowMapArray.h"

namespace ara {
class SPSpotLightShadow : public ShaderProto {
public:
    SPSpotLightShadow(sceneData* sd);

    void rebuildShader();

    void calcActSurfaces() {}
    void calcLights(CameraSet* cs, renderPass pass);
    void calcLight(CameraSet* cs, Light* lightPtr, LightPar* lightParPtr);
    void clear(renderPass pass) override;
    void sendPar(CameraSet* cs, double time, SceneNode* scene, SceneNode* parent, renderPass pass, uint loopNr = 0) override;
    bool begin(CameraSet* cs, renderPass pass, uint loopNr = 0) override;
    bool end(renderPass pass, uint loopNr = 0) override;

    std::string getLightBufferBlock(uint nrLights);
    Shaders*    getShader(renderPass pass, uint loopNr = 0) override;

    void setScreenSize(uint width, uint height) override;

private:
    GLint                                   m_max_tex_units = 0;
    std::unique_ptr<ShadowMapArray>         m_shadowGen;
    std::unique_ptr<ShaderBuffer<LightPar>> m_lightSb;

    glm::vec3              m_halfVector{0.f};
    std::vector<glm::mat4> m_pv_mats;
    std::vector<glm::mat4> m_shadowMat;
    std::vector<GLint>     m_depthTexUnits;
    std::vector<GLint>     m_lightColTexUnits;

    uint m_nrActSurfPasses = 0;
    uint m_nrLightPasses   = 0;
    uint m_maxNrParLights  = 0;
};
}  // namespace ara
