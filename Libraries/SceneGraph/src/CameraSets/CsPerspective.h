//
//  CsPerspective.h
//  Created by Sven Hahne on 11.08.17
//

#pragma once

#include "CameraSets/CameraSet.h"

namespace ara {
class CsPerspective : public CameraSet {
public:
    CsPerspective(sceneData* sc);
    ~CsPerspective() = default;

    void clearScreen(renderPass pass);
    void clearDepth();
    void clearFbo() {}
    void render(SceneNode* scene, SceneNode* parent, double time, double dt, uint ctxNr, renderPass pass);
    void postRender(renderPass pass, float* extDrawMatr = nullptr /*, float* extViewport=nullptr*/);
    void renderFbos(float* extDrawMatr = nullptr /*, float* extViewport=nullptr*/) {}
    void initClearShader();
    void setViewport(uint x, uint y, uint width, uint height, bool resizeProto);

    void onKey(int key, int scancode, int action, int mods) {}

    std::array<GLint, 4> m_csVp;
    glm::vec3            camPos;
    Shaders*             m_clearShdr = nullptr;
    Quad*                m_quad      = nullptr;
};
}  // namespace ara