//
// Created by user on 24.11.2020.
//

#pragma once

#include <UIApplication.h>
#include <Scene3D.h>
#include "../../Libraries/GLSceneGraph/src/CameraSets/CsPerspFbo.h"

#ifdef ARA_USE_ARCORE
#include "Android/ARCoreCam.h"
#endif

namespace ara
{

class UIApp : public UIApplication
{
public:
    UIApp();
    void init(std::function<void()> initCb) override;
    void exit() override;

private:
    Scene3D<CsPerspFbo>*    m_scene3D=nullptr;
    std::unique_ptr<AssimpImport>	m_importer;
    SceneNode*                    m_testModel=nullptr;

#ifdef ARA_USE_ARCORE
    ARCoreCam*                    m_arcCam=nullptr;
    cap::ARCore                         m_arCore;
#endif
};

}
