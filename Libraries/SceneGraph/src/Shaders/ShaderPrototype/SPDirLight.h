//
//  Author: Sven Hahne
//

#pragma once

#include "Shaders/ShaderProperties/LightShaderProperties.h"
#include "Shaders/ShaderPrototype/ShaderProto.h"

namespace ara {
class SPDirLight : public ShaderProto {
public:
    SPDirLight(sceneData* sd);
    ~SPDirLight();

    void clear(renderPass pass);
    void     sendPar(CameraSet* cs, double time, SceneNode* scene, SceneNode* parent, renderPass pass, uint loopNr = 0);
    bool     begin(CameraSet* cs, renderPass _pass, uint loopNr = 0);
    bool     end(renderPass pass, uint loopNr = 0);
    Shaders* getShader(renderPass pass, uint loopNr = 0);

private:
    glm::vec3             lightDir;
    glm::vec3             halfVector;
    LightShaderProperties lightProp;
};
}  // namespace ara
