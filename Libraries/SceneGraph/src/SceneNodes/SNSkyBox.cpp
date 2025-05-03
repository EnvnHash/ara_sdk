//  Created by Sven Hahne on 18/3/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "SceneNodes/SNSkyBox.h"

#include "CameraSets/CameraSet.h"
#include "Shaders/ShaderPrototype/SPSkyBox.h"

using namespace glm;
using namespace std;

namespace ara {

SNSkyBox::SNSkyBox(sceneData* sd) : SceneNode(sd) {
    m_protoName[GLSG_SCENE_PASS] = getTypeName<SPSkyBox>();  // force the use of the GridFloor ShaderPrototype if it is present
    init();
}

void SNSkyBox::init() {
    m_sphere = std::make_unique<Sphere>(1.f, 32);
}

void SNSkyBox::draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo) {
    if (pass != GLSG_SCENE_PASS) {
        return;
    }

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    glFrontFace(GL_CW);

    m_sphere->draw();

    glFrontFace(GL_CCW);  // counter clockwise definition means front, as default
    glDepthMask(GL_TRUE);
}

}  // namespace ara