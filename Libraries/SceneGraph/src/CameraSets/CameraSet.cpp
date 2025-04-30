//
//  CameraSet.cpp
//
//  Created by Sven Hahne on 16.08.17
//

#include "CameraSets/CameraSet.h"
#include "Utils/BoundingBoxer.h"

using namespace glm;
using namespace std;

namespace ara {

CameraSet::CameraSet(sceneData* sc)
    : m_glbase(sc->glbase),
    s_shCol(&sc->glbase->shaderCollector()),
    s_viewport(vec4{0.f, 0.f, 0.f, 0.f}),
    s_sd(sc) {
    if (sc) {
        s_quad     = sc->getStdQuad();
        s_viewport = sc->winViewport;
    }

    s_maskQuad = make_unique<Quad>(QuadInitParams{ .color = { 0.f, 0.f, 0.f, 1.f} });
    // standard shader for the background clearing
    s_clearShader = s_shCol->getStdClear();
}

ShaderProto* CameraSet::addShaderProto(const string& protoName, const list<renderPass>& protoPasses) {
    // create the Shader Prototype from a String Name
    s_shaderProto[protoName] = m_spf.Create(protoName, s_sd);

    // sort shaderProtos by renderPasses
    for (const auto& it : protoPasses) {
        s_renderPassShProto[it].emplace_back(s_shaderProto[protoName].get());
    }

    s_shaderProto[protoName]->setNrCams((int)s_cam.size());
    return s_shaderProto[protoName].get();
}

ShaderProto* CameraSet::getShaderProto(const string& protoName) {
    return (s_shaderProto.find(protoName) != s_shaderProto.end()) ? s_shaderProto[protoName].get() : nullptr;
}

void CameraSet::removeShaderProto(const std::string& protoName, const std::list<renderPass>& protoPasses) {
    if (s_shaderProto.find(protoName) == s_shaderProto.end()) {
        return;
    }

    auto shdr = s_shaderProto[protoName].get();
    for (const auto& it : protoPasses) {
        auto res = std::find_if(s_renderPassShProto[it].begin(), s_renderPassShProto[it].end(), [shdr](auto& it) {
            return shdr == it;
        });
        if (res != s_renderPassShProto[it].end()) {
            s_renderPassShProto[it].erase(res);
        }
    }
}

ShaderProto* CameraSet::getProtoForPass(renderPass pass, SceneNode* node) {
    // if there is at least one entry for this pass, return it
    if (s_renderPassShProto.find(pass) == s_renderPassShProto.end()) {
        return nullptr;
    }

    ShaderProto* foundCust = nullptr;

    if (!node->m_protoName[pass].empty()) {
        if (node->m_cachedCustShdr[pass]) {
            foundCust = node->m_cachedCustShdr[pass];
        } else if (!s_renderPassShProto[pass].empty()) {
            auto it = std::find_if(s_renderPassShProto[pass].begin(), s_renderPassShProto[pass].end(),
                                   [node, pass](const ShaderProto* e) { return e->s_name == node->m_protoName[pass]; });

            if (it != s_renderPassShProto[pass].end()) {
                foundCust                    = (*it);
                node->m_cachedCustShdr[pass] = foundCust;
            }
        }
    }

    if (foundCust) {
        return foundCust;
    } else if (!s_renderPassShProto[pass].empty()) {
        return s_renderPassShProto[pass].front();
    } else {
        return nullptr;
    }
}

void CameraSet::setInteractCam(TrackBallCam* cam) {
    if (s_interactCam) {
        s_interactCam->removeCamSetUpdtCb();
    }
    s_interactCam = cam;
    s_interactCam->setCamSetUpdtCb([this] { buildCamMatrixArrays(); });
}

vector<pair<TrackBallCam*, void*>>::iterator CameraSet::addCamera(TrackBallCam* camDef, void* name) {
    s_cam.emplace_back(camDef, name);
    buildCamMatrixArrays();
    for (auto&[fst, snd] : s_shaderProto) {
        snd->setNrCams(static_cast<int>(s_cam.size()));
    }
    return s_cam.end() - 1;
}

void CameraSet::removeCamera(void* name) {
    if (s_cam.empty()) return;

    // check if there's callback registered to this camera
    if (s_updtCb.contains(name)) s_updtCb.erase(name);
    const auto cIt =
        ranges::find_if(s_cam, [name](const pair<Camera*, void*>& p) { return p.second == name; });
    if (cIt != s_cam.end()) {
        s_cam.erase(cIt);
    }

    buildCamMatrixArrays();

    for (auto&[fst, snd] : s_shaderProto) {
        snd->setNrCams(static_cast<int>(s_cam.size()));
    }
}

// Merge all cameras of the set into one array for uniform upload to Shaders
void CameraSet::buildCamMatrixArrays() {
    s_modelMatrixList.resize(s_cam.size());
    s_viewMatrixList.resize(s_cam.size());
    s_projectionMatrixList.resize(s_cam.size());
    s_clearColors.resize(s_cam.size());
    s_floorSwitches.resize(s_cam.size());
    s_fishEyeSwitches.resize(s_cam.size());
    s_fishEyeParam.resize(s_cam.size());

    for (size_t i = 0; i < s_cam.size(); i++) {
        s_modelMatrixList[i]      = s_cam[i].first->getModelMatr();
        s_viewMatrixList[i]       = s_cam[i].first->getViewMatr();
        s_projectionMatrixList[i] = s_cam[i].first->getProjectionMatr();
        s_clearColors[i]          = s_cam[i].first->getClearColor();
        s_floorSwitches[i]        = s_cam[i].first->getFloorSwitch();
        s_fishEyeSwitches[i]      = s_cam[i].first->getFishEyeSwitch();
        s_fishEyeParam[i]         = s_cam[i].first->getFishEyeParam();
    }
}

void CameraSet::swapCameras(TrackBallCam* cam1, TrackBallCam* cam2) {
    if (s_cam.size() < 2) return;

    auto cam1It =
        std::find_if(s_cam.begin(), s_cam.end(), [cam1](const pair<Camera*, void*>& p) { return p.first == cam1; });
    if (cam1It == s_cam.end()) return;

    auto cam2It =
        std::find_if(s_cam.begin(), s_cam.end(), [cam2](const pair<Camera*, void*>& p) { return p.first == cam2; });
    if (cam2It == s_cam.end()) return;

    std::iter_swap(cam1It, cam2It);

    // in case any of the cameras to swap is now the first in the s_cam array,
    // assign the trackball to it
    cam1->setUseTrackBall(cam1 == s_cam[0].first);
    cam2->setUseTrackBall(cam2 == s_cam[0].first);

    // in case one of the cameras to swap is the first (scene) camera, update
    // corresponding settings
    setInteractCam(s_cam[0].first);
}

void CameraSet::renderTree(SceneNode* node, double time, double dt, uint ctxNr, renderPass pass) {
    if (node->m_calcMatrixStack.load()) {
        s_matrixStack.clear();
    }

    iterateNode(node, time, dt, ctxNr, pass, node->m_calcMatrixStack.load());

    if (node->m_calcMatrixStack.load()) {
        node->m_calcMatrixStack = false;
    }
}

void CameraSet::iterateNode(SceneNode* node, double time, double dt, uint ctxNr, renderPass pass, bool calcMatrixStack) {
    if (calcMatrixStack && node) {
        s_matrixStack.emplace_back(&node->getRelModelMat());
    }

    if (node) {
        for (const auto& it : *node->getChildren()) {
            // if the node was translated, scaled or rotated since the last loop
            if (calcMatrixStack && it->m_hasNewModelMat) {
                // since all matrices inside the matrixstack are relative to its respective parent, we need to multiply
                // them down to get the actual parent matrix
                s_sumMat = mat4(1.f);
                for (const auto& mIt : s_matrixStack) {
                    s_sumMat *= *mIt;
                }

                // calculate it's new absolute matrix in respect to its parent nodes
                it->setParentModelMat(node, s_sumMat);

                // this node has been updated, so remove the update flag
                it->m_hasNewModelMat = false;

                // all subnodes have to be updated, so set the flag to all children
                for (const auto& child : *it->getChildren()) {
                    child->m_hasNewModelMat = true;
                }
            }

            // check if the BoundingBox of the SceneNode is calculated, if not do so NOTE: the bounding box is
            // independent of the nodes actual translation
            if (!it->m_hasBoundingBox) {
                auto bBox = static_cast<BoundingBoxer*>(s_sd->boundBoxer);
                bBox->begin();
                it->draw(time, dt, this, bBox->getShader(), pass);
                bBox->end();

                if (!glm::all(glm::isnan(bBox->getBoundMin())) && !glm::all(glm::isinf(bBox->getBoundMin())) &&
                    !glm::all(glm::isnan(bBox->getBoundMax())) && !glm::all(glm::isinf(bBox->getBoundMax())))
                    it->setBoundingBox(&bBox->getBoundMin(), &bBox->getBoundMax());
            }

            if (it->m_visible && !(pass == GLSG_OBJECT_MAP_PASS && it->excludeFromObjMap())) {
                render(it, node, time, dt, ctxNr, pass);
                iterateNode(it, time, dt, ctxNr, pass, calcMatrixStack);
            }
        }
    }

    if (calcMatrixStack && !s_matrixStack.empty()) {
        s_matrixStack.pop_back();
    }
}

void CameraSet::mask() {
    glViewport((GLint)s_viewport.x, (GLint)s_viewport.y, (GLint)s_viewport.z, (GLint)s_viewport.w);

    glClear(GL_DEPTH_BUFFER_BIT);

    s_clearShader->begin();
    s_clearShader->setIdentMatrix4fv("m_pvm");
    s_clearShader->setUniform4fv("clearCol", &s_clearColors[0][0], (int)s_clearColors.size());

    glDisable(GL_DEPTH_TEST);
    s_maskQuad->draw();
    glEnable(GL_DEPTH_TEST);

    Shaders::end();
}

inline void CameraSet::setMask(vec3 scale, vec3 trans) {
    s_maskQuad->scale(scale);
    s_maskQuad->translate(trans);
}

void CameraSet::setViewport(uint x, uint y, uint width, uint height, bool resizeProto) {
    s_viewport.x = static_cast<float>(x);
    s_viewport.y = static_cast<float>(y);
    s_viewport.z = static_cast<float>(width);
    s_viewport.w = static_cast<float>(height);
    s_iViewport  = s_viewport;

    s_fScrWidth  = static_cast<float>(width);
    s_fScrHeight = static_cast<float>(height);

    for (const auto &key: s_cam | views::keys) {
        if (key) {
            key->setScreenSize(width, height);
        }
    }

    if (resizeProto) {
        for (const auto& proto : s_shaderProto  | views::values) {
            proto->setScreenSize(width, height);
        }
    }
}

}  // namespace ara
