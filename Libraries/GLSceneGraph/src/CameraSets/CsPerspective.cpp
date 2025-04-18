#include "CsPerspective.h"

using namespace glm;
using namespace std;

namespace ara {
CsPerspective::CsPerspective(sceneData* sc) : CameraSet(sc) {
    camPos = vec3(0.f, 0.f, 1.f);

    if (s_sd) {
        float aspect = s_sd->winViewport.z / s_sd->winViewport.w;

        if (s_sd->winViewport.z > 0.f && s_sd->winViewport.w > 0.f) {
            s_intern_cam.push_back(make_unique<TrackBallCam>(Camera::camType::perspective, static_cast<float>(s_sd->winViewport.z),
                                                             static_cast<float>(s_sd->winViewport.w), -aspect, aspect, -1.0f,
                                                             1.0f,                          // left, right, bottom, top
                                                             camPos.x, camPos.y, camPos.z,  // camPos
                                                             0.f, 0.f, 0.f,                 // lookAt
                                                             0.f, 1.f, 0.f,                 // upVec
                                                             0.1f, 100.f));

            s_intern_cam.back()->setUseTrackBall(true);
            s_cam.emplace_back(s_intern_cam.back().get(), this);
            CameraSet::setInteractCam(s_intern_cam.back().get());
        }
        CameraSet::buildCamMatrixArrays();
    }
}

void CsPerspective::clearScreen(renderPass _pass) {
    if (_pass == GLSG_SCENE_PASS || _pass == GLSG_GIZMO_PASS) {
        glClearColor(0.0, 0.0, 0.0, 0.0);
        glClearDepthf(1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glScissor(s_iViewport.x, s_iViewport.y, s_iViewport.z, s_iViewport.w);
        // glViewportIndexedf(0, s_viewport.x, s_viewport.y, s_viewport.z,
        // s_viewport.w);
        glViewport(s_iViewport.x, s_iViewport.y, s_iViewport.z, s_iViewport.w);

    } else if (_pass == GLSG_SHADOW_MAP_PASS || _pass == GLSG_OBJECT_MAP_PASS) {
        for (auto& it : s_shaderProto) it.second->clear(_pass);
    }
}

void CsPerspective::clearDepth() {
    glClearDepthf(1.f);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void CsPerspective::render(SceneNode* node, SceneNode* parent, double time, double dt, uint ctxNr, renderPass pass) {
    node->update(time, dt, this);

    auto proto = getProtoForPass(pass, node);
    if (!proto) return;

    // Iterate through the CameraSets shaderPrototypes
    s_actFboSize = vec2(s_fScrWidth, s_fScrHeight);

    // if we have more light sources than parallel texture units, we need to
    // render the shadow maps in multiple passes
    int  loopNr = 0;
    bool run    = true;

    while (run)  // if ShaderProtoype->end return a value > 0 the scene will be
                 // drawn another time
    {
        if (proto->begin(this, pass,
                         loopNr))  // check if the prerendering step is really needed
        {
            proto->sendPar(this, time, node, parent, pass, loopNr);
            if (proto->getShader(pass, loopNr)) node->draw(time, dt, this, proto->getShader(pass, loopNr), pass);
        }

        run = proto->end(pass, loopNr);
        loopNr++;
    }
}

void CsPerspective::postRender(renderPass _pass, float* extDrawMatr /*, float* extViewport*/) {
    for (auto& it : s_shaderProto) it.second->postRender(_pass);

    if (_pass == GLSG_SCENE_PASS || _pass == GLSG_GIZMO_PASS) {
        glViewport(m_csVp[0], m_csVp[1], m_csVp[2], m_csVp[3]);
        glScissor(m_csVp[0], m_csVp[1], m_csVp[2], m_csVp[3]);  // wichtig!!!
    }
}

void CsPerspective::setViewport(uint x, uint y, uint width, uint height, bool resizeProto) {
    CameraSet::setViewport(x, y, width, height, resizeProto);
    buildCamMatrixArrays();
}

}  // namespace ara
