#pragma once

#ifdef ARA_USE_ASSIMP

#include "Assimp/AssimpImport.h"
#include "SceneNodes/SceneNode.h"

namespace ara {

class CameraSet;

class SNAssimp : public SceneNode {
public:
    SNAssimp(sceneData* sd = nullptr);
    ~SNAssimp() {};

    bool loadMesh();

    std::string m_fileName;
    bool        loaded = false;

protected:
    std::unique_ptr<AssimpImport> aImport;
};

}  // namespace ara

#endif