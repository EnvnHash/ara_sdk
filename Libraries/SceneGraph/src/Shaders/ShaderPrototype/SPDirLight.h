//
//  Author: Sven Hahne
//

#pragma once

#include "Shaders/ShaderProperties/LightShaderProperties.h"
#include "Shaders/ShaderPrototype/ShaderProto.h"

namespace ara {
class SPDirLight : public ShaderProto {
public:
    explicit SPDirLight(sceneData* sd);

    void     clear(renderPass pass) override;
    void     sendPar(CameraSet* cs, double time, SceneNode* scene, SceneNode* parent, renderPass pass, uint loopNr = 0) override;
    bool     begin(CameraSet* cs, renderPass _pass, uint loopNr = 0) override;
    bool     end(renderPass pass, uint loopNr = 0) override;
    Shaders* getShader(renderPass pass, uint loopNr = 0) override;

private:
    glm::vec3             lightDir{};
    glm::vec3             halfVector{};
    LightShaderProperties lightProp{};
};
}  // namespace ara
