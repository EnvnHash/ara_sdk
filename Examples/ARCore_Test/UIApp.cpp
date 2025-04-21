//
// Created by user on 24.11.2020.
//

#include "UIApp.h"

using namespace std;
using namespace ara;
using namespace ara;
using namespace ara;
using namespace ara;

UIApp::UIApp() : UIApplication() {
    m_appStateCbs[android_app_cmd::onResume].emplace_back([this](android_cmd_data* cd){ m_arCore.onResume(cd); });
    m_appStateCbs[android_app_cmd::OnDisplayGeometryChanged].emplace_back([this](android_cmd_data* cd){ m_arCore.onDisplayGeometryChanged(cd); });
    m_appStateCbs[android_app_cmd::OnTouched].emplace_back([this](android_cmd_data* cd){ m_arCore.onTouched(cd); });
}

void UIApp::init(std::function<void()> initCb) {
    // create the main UI-Window. must be on the same thread on that glbase.init happened, in order to have
    // context sharing work
    m_mainWindow = addWindow(1600, 1000, 50, 20, false);

    m_arcCam = m_mainWindow->getRootNode()->addChild<ARCoreCam>();
    m_arcCam->m_arCore = &m_arCore;

    m_scene3D = m_mainWindow->getRootNode()->addChild<Scene3D<CsPerspFbo>>("3D"); // 3D View
    m_scene3D->setPermRedraw(true);

    m_arCore.setOnCreateAnchorCb([this]{
        m_testModel->setVisibility(true);
    });

    m_arCore.setAnchorUptCb([this](mat4& m){
        auto c = m_scene3D->getSceneCamSet();
        if (c){
            auto ic = c->getInteractCam();
            if (ic){
                auto pm = m_arCore.getProjMat() * m_arCore.getViewMat();
                ic->setProjMatr(pm);
                ic->setModelMatr(m);
                c->buildCamMatrixArrays();
            }
        }
    });

    m_importer = make_unique<AssimpImport>(&m_glbase);
    m_scene3D->addInitCb([this] {

        std::string modelPath = m_scene3D->getSceneData()->dataPath + "models/Panadome.obj";

        m_testModel = m_importer->load(m_scene3D, modelPath,  [this](SceneNode* sceneCont) {

            sceneCont->m_hasNewModelMat = true;    // force rebuild of nodes m_absModelMat
            sceneCont->iterateNode(m_testModel, [this](SceneNode* node) {
                node->getMaterial()->setDiffuse(1.f, 0.f, 0.f, 1.f);
                return true;
            });

            sceneCont->setVisibility(false);
        }, true);

        return;
    });

    // check_permission("CAMERA");

    startRenderLoop();  // main UIApplication renderloop (for all windows) -> blocking on all OSes but android
}

void UIApp::exit() {
    UIApplication::exit();
}