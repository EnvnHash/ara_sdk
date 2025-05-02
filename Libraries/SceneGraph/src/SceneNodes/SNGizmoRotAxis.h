#pragma once

// #define GIZMO_ROT_SHADER_ONLY

#include "SceneNodes/SNGizmoAxis.h"

namespace ara {

class SNGizmoRotAxis : public SNGizmoAxis {
public:
    explicit SNGizmoRotAxis(sceneData* sd = nullptr);
    ~SNGizmoRotAxis() override = default;

    void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo = nullptr) override;

    std::array<std::unique_ptr<VAO>, 2> m_gizVao;
#ifdef GIZMO_ROT_SHADER_ONLY
    void initShader();

private:
    ShaderCollector* m_shCol        = nullptr;
    Shaders*         m_torusShader  = nullptr;
    Quad*            m_quad         = nullptr;
    bool             m_rotMatInited = false;
    glm::mat4        m_gRot         = glm::mat4(1.f);
    glm::mat4        m_invCamMat    = glm::mat4(1.f);
    glm::mat4        m_locPvm       = glm::mat4(1.f);
#endif
};

}  // namespace ara