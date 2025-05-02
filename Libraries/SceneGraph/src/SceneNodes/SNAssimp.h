#pragma once

#ifdef ARA_USE_ASSIMP

#include "SceneNodes/SceneNode.h"

namespace ara {

class AssimpImport;
class CameraSet;

class SNAssimp : public SceneNode {
public:
    explicit SNAssimp(sceneData* sd = nullptr);
    ~SNAssimp() override = default;

    bool loadMesh();

    std::string m_fileName;
    bool        m_loaded = false;

protected:
    std::unique_ptr<AssimpImport> m_aImport;
};

}  // namespace ara

#endif