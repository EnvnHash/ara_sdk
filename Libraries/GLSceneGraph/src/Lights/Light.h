#pragma once

#include <GLUtils/sceneData.h>
#include <Meshes/Mesh.h>
#include <glsg_common/glsg_common.h>

#include "SceneNodes/SceneNode.h"
#include "Shaders/ShaderProperties/LightShaderProperties.h"

namespace ara {
class Light : public SceneNode {
public:
    Light(sceneData* sd = nullptr);
    virtual ~Light() = default;

    virtual void draw(double time, double dt, CameraSet* cs, Shaders* _shader, renderPass _pass, TFO* _tfo = nullptr) {}

    virtual void setup(bool force = false) = 0;
    virtual void bindColTex(GLuint unit) {}
    GLint                  getColTex() { return s_colTex; }
    virtual std::string    getLightShaderBlock();
    LightShaderProperties* getLightShdrProp() { return &s_lightProp; }
    void*                  getPtr(std::string name) { return s_lightProp.getPtr(name); }
    GLuint*                getShadowMapView() { return &s_shadowMapView; }
    void setColTex(GLint _val) { s_colTex = _val; }

    LightShaderProperties s_lightProp;
    Mesh*                 s_mesh = nullptr;
    GLuint                s_shadowMapView;
    GLint                 s_colTex;

    glm::vec3 s_direction;
    glm::vec3 s_lookAt;
    glm::mat4 s_view_mat;
    glm::mat4 s_proj_mat;
    glm::mat4 s_pvm_mat;
    glm::mat4 s_shadow_mat;

    float s_near;
    float s_far;
    float s_fov;

    std::atomic<bool> s_needsRecalc;
};
}  // namespace ara
