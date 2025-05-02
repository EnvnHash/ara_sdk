//
//  SPSpotLight.h
//
//
//  Created by Sven Hahne on 14.08.14.
//

#pragma once

#include "Shaders/ShaderProperties/LightShaderProperties.h"
#include "Shaders/ShaderPrototype/ShaderProto.h"

namespace ara {
class SPSpotLight : public ShaderProto {
public:
    SPSpotLight(sceneData* sd);

    void clear(renderPass _pass) override;
    void sendPar(CameraSet* cs, double time, SceneNode* scene, SceneNode* parent, renderPass pass, uint loopNr = 0) override;
    bool begin(CameraSet* cs, renderPass pass, uint loopNr = 0) override;
    bool end(renderPass pass, uint loopNr = 0) override;
    Shaders* getShader(renderPass pass, uint loopNr = 0) override;

private:
    glm::vec3             m_lightDir{};
    LightShaderProperties m_lightProp{};
};
}  // namespace ara
