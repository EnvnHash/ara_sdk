/**
 *	 SPSkyBox.h
 *
 *  Created by Sven Hahne on 23.09.21
 */

#pragma once

#include "Shaders/ShaderPrototype/ShaderProto.h"
#include "Shaders/ShaderUtils/ShaderBuffer.h"

namespace ara {

class Texture;

class SPSkyBox : public ShaderProto {
public:
    explicit SPSkyBox(sceneData* sd);
    ~SPSkyBox() override = default;

    void rebuildShader(uint nrCameras);
    void clear(renderPass pass) override;
    void sendPar(CameraSet* cs, double time, SceneNode* scene, SceneNode* parent, renderPass pass, uint loopNr = 0) override;
    bool begin(CameraSet* cs, renderPass pass, uint loopNr = 0) override;
    bool end(renderPass pass, uint loopNr = 0) override;
    void postRender(renderPass pass) override { }

    Shaders* getShader(renderPass pass, uint loopNr = 0) override;

    void setScreenSize(uint width, uint height) override;
    void setNrCams(int nrCams) override;

private:
    std::unique_ptr<Texture> m_cubeTex;
};
}  // namespace ara
