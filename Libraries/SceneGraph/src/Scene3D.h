/**
 * \brief Scene3D, Entry point for everything that needs to be displayed in 3D
 *
 */

#pragma once

#include <Scene3DBase.h>

namespace ara {

template <class T>
class Scene3D : public Scene3DBase {
public:
    Scene3D() : Scene3DBase() {}
    ~Scene3D() override = default;

    void initCamSet() override      { Scene3DBase::initCamSet<T>(); }
    T*           getSceneCamSet()   { return m_sceneRenderCam ?  static_cast<T *>(m_sceneRenderCam.get()) : nullptr; }
};

}  // namespace ara
