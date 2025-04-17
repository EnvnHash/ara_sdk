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
    ~SPSpotLightShadow();

    void rebuildShader();

    void calcActSurfaces() {}
    void calcLights(CameraSet* cs, renderPass pass);
    void calcLight(CameraSet* cs, Light* lightPtr, LightPar* lightParPtr);
    void clear(renderPass pass);
    void sendPar(CameraSet* cs, double time, SceneNode* scene, SceneNode* parent, renderPass pass, uint loopNr = 0);
    bool begin(CameraSet* cs, renderPass pass, uint loopNr = 0);
    bool end(renderPass pass, uint loopNr = 0);

    std::string getLightBufferBlock(uint nrLights);
    Shaders*    getShader(renderPass pass, uint loopNr = 0);

    void setScreenSize(uint width, uint height);

private:
    GLint                                   max_tex_units = 0;
    std::unique_ptr<ShadowMapArray>         shadowGen;
    std::unique_ptr<ShaderBuffer<LightPar>> lightSb;

    glm::vec3              halfVector{0.f};
    std::vector<glm::mat4> pv_mats;
    std::vector<glm::mat4> shadowMat;
    std::vector<GLint>     depthTexUnits;
    std::vector<GLint>     lightColTexUnits;

    uint nrActSurfPasses = 0;
    uint nrLightPasses   = 0;
    uint maxNrParLights  = 0;
};
}  // namespace ara
