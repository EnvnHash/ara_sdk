//
//  CsPerspFbo.h
//
//  Created by Sven Hahne last modified on 22.08.17
//

#pragma once

#include "CameraSets/CameraSet.h"

namespace ara {

class FBO;

class CsPerspFbo : public CameraSet {
public:
    explicit CsPerspFbo(sceneData* sc);
    ~CsPerspFbo() override = default;

    void initLayerTexShdr();
    void initClearShader(int nrLayers);
    void rebuildFbo();
    void clearScreen(renderPass pass) override;
    void clearDepth() override;
    void clearFbo() override {}

    void renderTree(SceneNode* scene, double time, double dt, uint ctxNr, renderPass pass) override;
    void render(SceneNode* scene, SceneNode* parent, double time, double dt, uint ctxNr, renderPass pass) override;
    void postRender(renderPass pass, float* extDrawMatr = nullptr) override;
    void renderFbos(float* extDrawMatr = nullptr) override;
    void setViewport(uint x, uint y, uint width, uint height, bool resizeProto = false) override;
    std::vector<std::pair<TrackBallCam*, void*>>::iterator addCamera(TrackBallCam* camDef, void* name) override;
    void onKey(int key, int scancode, int action, int mods) override {}

    FBO*     getFbo() override { return &m_fbo; }
    Shaders* getLayerTexShdr() { return m_layerTexShdr; }

private:
    FBO                   m_fbo;
    std::unique_ptr<Quad> m_quad;
    Shaders*              m_layerTexShdr = nullptr;
    Shaders*              m_clearShdr    = nullptr;
    Shaders*              m_stdParCol    = nullptr;
    int                   m_savedNr;
    bool                  m_reqSnapshot;
    glm::vec3             m_camPos{};
};

}  // namespace ara
