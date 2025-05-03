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

    void rebuildShader(uint nrCameras);

    void calcActSurfaces() {}
    void calcLights(CameraSet* cs, renderPass pass);
    void calcLight(CameraSet* cs, Light* lightPtr, LightPar* lightParPtr);
    void clear(renderPass pass) override;

    void sendPar(CameraSet* cs, double time, SceneNode* scene, SceneNode* parent, renderPass pass, uint loopNr = 0) override;
    void sendParShadowMapPass(SceneNode *node, SceneNode *parent);
    void estimateNumPasses(uint loopNr);
    void sendParSceneAndGizmoPass(SceneNode *node, SceneNode *parent, uint loopNr);
    void sendLightPar(SceneNode *node, SceneNode *parent, int nrLightsThisPass, uint lightOffs);

    bool begin(CameraSet* cs, renderPass pass, uint loopNr = 0) override;
    bool end(renderPass pass, uint loopNr = 0) override;
    void postRender(renderPass pass) override;

    Shaders* getShader(renderPass pass, uint loopNr = 0) override;

    void setScreenSize(uint width, uint height) override;
    void setNrCams(int nrCams) override;

    std::string getUbPar(uint32_t nrCameras) override;

private:
    std::unique_ptr<ShadowMapVsmArray>      m_shadowGen;
    std::unique_ptr<ShaderBuffer<LightPar>> m_lightSb;

    glm::vec3              m_halfVector{0.f};
    std::vector<glm::mat4> m_pv_mats;
    std::vector<glm::mat4> m_shadowMat;
    std::vector<GLint>     m_depthTexUnits;
    std::vector<GLint>     m_lightColTexUnits;

    uint m_nrActSurfPasses = 0;
    uint m_nrLightPasses   = 0;
    uint m_maxNrParLights  = 0;

    bool m_shineThrough   = false;
    bool m_shadowGenBound = false;
};
}  // namespace ara
