/**
*	 SPNoLight.h
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

class SPNoLight : public ShaderProto {
public:
    SPNoLight(sceneData* sd);
    ~SPNoLight() = default;

    void rebuildShader(uint nrCameras);
    void clear(renderPass pass) override;
    void sendPar(CameraSet* cs, double time, SceneNode* scene, SceneNode* parent, renderPass pass,
                uint loopNr = 0) override;
    bool begin(CameraSet* cs, renderPass pass, uint loopNr = 0) override;
    bool end(renderPass pass, uint loopNr = 0) override;

    void setScreenSize(uint width, uint height) override;
    void setNrCams(int nrCams) override;

    std::string getUbPar(uint32_t nrCameras) override;
    Shaders* getShader(renderPass pass, uint loopNr = 0) override;

private:
   std::unique_ptr<ShadowMapVsmArray>      shadowGen;
   std::unique_ptr<ShaderBuffer<LightPar>> lightSb;

   glm::vec3              halfVector{0.f};
   std::vector<glm::mat4> pv_mats;
   std::vector<glm::mat4> shadowMat;
   std::vector<GLint>     depthTexUnits;
   std::vector<GLint>     lightColTexUnits;
};

}  // namespace ara
