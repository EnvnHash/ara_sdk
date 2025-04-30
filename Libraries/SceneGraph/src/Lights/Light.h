#pragma once

#include <GLUtils/sceneData.h>
#include <Meshes/Mesh.h>
#include <glsg_common/glsg_common.h>

#include "SceneNodes/SceneNode.h"
#include "Shaders/ShaderProperties/LightShaderProperties.h"

namespace ara {
class Light : public SceneNode {
public:
    explicit Light(sceneData* sd = nullptr);

    void draw(double time, double dt, CameraSet* cs, Shaders* shader, renderPass pass, TFO* tfo) override {}
    virtual void setup(bool force) = 0;
    virtual void bindColTex(GLuint unit) {}

    GLint                  getColTex() const { return s_colTex; }
    virtual std::string    getLightShaderBlock();
    LightShaderProperties* getLightShdrProp() { return &s_lightProp; }
    void*                  getPtr(const std::string &name) { return s_lightProp.getPtr(name); }
    GLuint*                getShadowMapView() { return &s_shadowMapView; }
    void                   setColTex(GLint val) { s_colTex = val; }

    LightShaderProperties s_lightProp;
    Mesh*                 s_mesh = nullptr;
    GLuint                s_shadowMapView = 0;
    GLint                 s_colTex = 0;

    glm::vec3 s_direction{0.f, 0.f, -1.f};
    glm::vec3 s_lookAt{0.f, 0.f, 0.f};
    glm::mat4 s_view_mat = glm::mat4(1.f);
    glm::mat4 s_proj_mat = glm::mat4(1.f);
    glm::mat4 s_pvm_mat = glm::mat4(1.f);
    glm::mat4 s_shadow_mat = glm::mat4(1.f);

    float s_near = 1.f;
    float s_far = 1000.f;
    float s_fov = 45.f;

    std::atomic<bool> s_needsRecalc = true;
};
}  // namespace ara
