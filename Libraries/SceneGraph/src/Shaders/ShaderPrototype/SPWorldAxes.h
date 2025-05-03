/**
 *	 SPWorldAxes.h
 *
 *  Created by Sven Hahne on 23.09.21
 */

#pragma once

#include "Shaders/ShaderPrototype/ShaderProto.h"

namespace ara {
class SPWorldAxes : public ShaderProto {
public:
    SPWorldAxes(sceneData* sd);
    ~SPWorldAxes() = default;

    void rebuildShader(uint nrCameras);
    void clear(renderPass _pass) override;
    void sendPar(CameraSet* cs, double time, SceneNode* scene, SceneNode* parent, renderPass pass, uint loopNr = 0) override;
    bool begin(CameraSet* cs, renderPass pass, uint loopNr = 0) override;
    bool end(renderPass pass, uint loopNr = 0) override;
    void postRender(renderPass pass) override;

    Shaders* getShader(renderPass pass, uint loopNr = 0) override;

    void setScreenSize(uint width, uint height) override;
    void setNrCams(int nrCams) override;

private:
    std::vector<glm::mat4> pv_mats;
};
}  // namespace ara
