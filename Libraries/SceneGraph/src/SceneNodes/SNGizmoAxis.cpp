#include "SNGizmoAxis.h"

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

SNGizmoAxis::SNGizmoAxis(sceneData* sd) : SceneNode(sd) {
    m_nodeType = sceneNodeType::gizmo;
}

}  // namespace ara