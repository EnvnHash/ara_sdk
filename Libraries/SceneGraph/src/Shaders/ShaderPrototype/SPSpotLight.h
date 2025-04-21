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
    ~SPSpotLight();

    void clear(renderPass _pass);
    void sendPar(CameraSet* cs, double time, SceneNode* _scene, SceneNode* _parent, renderPass _pass, uint loopNr = 0);
    bool begin(CameraSet* cs, renderPass _pass, uint loopNr = 0);
    bool end(renderPass _pass, uint loopNr = 0);
    Shaders* getShader(renderPass pass, uint loopNr = 0);

private:
    glm::vec3             lightDir;
    LightShaderProperties lightProp;
};
}  // namespace ara
