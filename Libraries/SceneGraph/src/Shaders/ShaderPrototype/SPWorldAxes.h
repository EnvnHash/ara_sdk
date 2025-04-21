/**
 *	 SPWorldAxes.h
 *
 *  Created by Sven Hahne on 23.09.21
 */

#pragma once

#include "Shaders/ShaderPrototype/ShaderProto.h"
#include "Shaders/ShaderUtils/ShaderBuffer.h"

namespace ara {
class SPWorldAxes : public ShaderProto {
public:
    SPWorldAxes(sceneData* sd);
    ~SPWorldAxes() = default;

    void rebuildShader(uint nrCameras);
    void clear(renderPass _pass);
    void sendPar(CameraSet* cs, double time, SceneNode* scene, SceneNode* parent, renderPass pass, uint loopNr = 0);
    bool begin(CameraSet* cs, renderPass pass, uint loopNr = 0);
    bool end(renderPass pass, uint loopNr = 0);
    void postRender(renderPass pass);

    Shaders* getShader(renderPass pass, uint loopNr = 0);

    void setScreenSize(uint width, uint height);
    void setNrCams(int nrCams);

private:
    std::vector<glm::mat4> pv_mats;
};
}  // namespace ara
