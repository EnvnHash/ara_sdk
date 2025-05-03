#pragma once

#include <Meshes/SurfaceGenerator.h>

#include "CameraSets/CameraSet.h"
#include "SceneNodes/SceneNode.h"

namespace ara {

class SNSurface : public SceneNode {
public:
    SNSurface(sceneData* sd = nullptr);

    void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo = nullptr);
    bool rebuildMesh();

    SurfaceGenerator m_surfGen;
    bool             m_paramChanged;
    bool             m_reqRebuildMesh = false;
    bool             m_fbSet          = false;
};

}  // namespace ara
