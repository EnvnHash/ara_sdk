#include "SceneNodes/SNGridFloor.h"

#include <GLBase.h>

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

SNGridFloor::SNGridFloor(sceneData* sd) : SceneNode(sd) {
    setName(getTypeName<SNGridFloor>());

    m_quad = s_sd->getStdQuad();
    m_transFlag |= GLSG_NO_SCALE;
    m_selectable                 = false;
    m_excludeFromObjMap          = true;
    m_protoName[GLSG_SCENE_PASS] = getTypeName<SPGridFloor>();  // force the use of the GridFloor
                                                                // ShaderPrototype if it is present

    SceneNode::scale(vec3(20.0f, 20.0f, 1.0f));
}

void SNGridFloor::update(double time, double dt, CameraSet* cs) {
    if (!m_rotationSet) {
        switch (m_basePlane) {
            case basePlane::xz: SceneNode::rotate(glm::radians(90.f), vec3(1.f, 0.f, 0.f)); break;
            case basePlane::xy: SceneNode::rotate(glm::radians(0.f), vec3(1.f, 0.f, 0.f)); break;
            case basePlane::yz: SceneNode::rotate(glm::radians(90.f), vec3(0.f, 1.f, 0.f)); break;
        }

        // immediately update the model matrix
        rebuildModelMat();

        m_rotationSet = true;
    }
}

void SNGridFloor::draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo) {
    if (pass == GLSG_OBJECT_MAP_PASS || pass == GLSG_SCENE_PASS) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(m_depthMask);

        shader->setUniform1f("ambientAmt", 0.25f);
        shader->setUniform2fv("floorGridSize", &m_gridSize[0]);
        m_quad->draw();

        if (!m_depthMask) {
            glDepthMask(true);
        }
    }
}

}  // namespace ara
