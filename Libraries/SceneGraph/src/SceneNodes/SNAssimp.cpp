/**
 *  Representation of a network camera inside the scene
 */

#ifdef ARA_USE_ASSIMP

#include "SceneNodes/SNAssimp.h"

using namespace glm;
using namespace std;

namespace ara {

SNAssimp::SNAssimp(sceneData* sd) : SceneNode(sd) { setName(getTypeName<ara::SNAssimp>()); }

bool SNAssimp::loadMesh() {
    if (hasParentNode()) {
        std::string file = (s_sd->dataPath + m_fileName);
        static_cast<AssimpImport*>(s_sd->aImport)
            ->load(
                file, this,
                [this](SceneNode* rootNode) {
                    rootNode->setVisibility(true);
                    for (const auto& it : *rootNode->getChildren()) {
                        it->setVisibility(true);
                    }
                },
                true);
        return true;
    } else {
        return false;
    }
}

}  // namespace ara
#endif