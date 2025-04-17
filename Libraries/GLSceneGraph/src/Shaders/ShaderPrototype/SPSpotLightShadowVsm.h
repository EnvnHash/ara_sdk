/**
 *	 SPSpotLightShadowVsm.h
 *
 *  Created by Sven Hahne on 14.08.17
 */

#pragma once

#include "GLUtils/FastBlurMem.h"
#include "Shaders/ShaderProperties/LightShaderProperties.h"
#include "Shaders/ShaderPrototype/ShaderProto.h"
#include "Shaders/ShaderUtils/ShaderBuffer.h"
#include "Shaders/ShadowMap/ShadowMapArray.h"
#include "Shaders/ShadowMap/ShadowMapVsmArray.h"

namespace ara {
class SPSpotLightShadowVsm : public ShaderProto {
public:
    SPSpotLightShadowVsm(sceneData* sd);
    ~SPSpotLightShadowVsm() = default;

    void rebuildShader(uint nrCameras);

    void calcActSurfaces() {}
    void calcLights(CameraSet* cs, renderPass pass);
    void calcLight(CameraSet* cs, Light* lightPtr, LightPar* lightParPtr);
    void clear(renderPass pass) override;

    void sendPar(CameraSet* cs, double time, SceneNode* scene, SceneNode* parent, renderPass pass,
                 uint loopNr = 0) override;
    bool begin(CameraSet* cs, renderPass pass, uint loopNr = 0) override;
    bool end(renderPass pass, uint loopNr = 0) override;
    void postRender(renderPass pass) override;

    // std::string getLightBufferBlock(uint nrLights);
    Shaders* getShader(renderPass pass, uint loopNr = 0) override;

    void setScreenSize(uint width, uint height) override;
    void setNrCams(int nrCams) override;

    std::string getUbPar(uint32_t nrCameras) override;

private:
    std::unique_ptr<ShadowMapVsmArray>      shadowGen;
    std::unique_ptr<ShaderBuffer<LightPar>> lightSb;

    glm::vec3              halfVector{0.f};
    std::vector<glm::mat4> pv_mats;
    std::vector<glm::mat4> shadowMat;
    std::vector<GLint>     depthTexUnits;
    std::vector<GLint>     lightColTexUnits;

    uint nrActSurfPasses = 0;
    uint nrLightPasses   = 0;
    uint maxNrParLights  = 0;

    bool m_shineThrough   = false;
    bool m_shadowGenBound = false;
};
}  // namespace ara
