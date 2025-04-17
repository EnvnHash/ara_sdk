#include "SNGizmoAxis.h"

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

SNGizmoAxis::SNGizmoAxis(sceneData* sd) : SceneNode(sd), totNrIndices(0), totNrPoints(0), emisBright(0.3f) {
    m_nodeType = GLSG_GIZMO;
}

}  // namespace ara