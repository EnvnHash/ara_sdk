//
//  ShadowMapEsm.cpp
//
//  Created by Sven Hahne on 12.07.14.
//
//  Generate a Shadow map for Exponential Shadow Mapping
//

#include "Shaders/ShadowMap/ShadowMapEsm.h"

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {
ShadowMapEsm::ShadowMapEsm(CameraSet* cs, int scrWidth, int scrHeight, vec3 lightPos, float near_lim, float far_lim,
                           sceneData* scd)
    : ShadowMap(cs->getGLBase()), colBufType(GL_R32F) {
    s_shadow_map_coef = 1.f;
    s_scrWidth        = static_cast<int>(static_cast<float>(scrWidth) * s_shadow_map_coef);
    s_scrHeight       = static_cast<int>(static_cast<float>(scrHeight) * s_shadow_map_coef);

    // s_fbo for saving the depth information
    s_fbo =
        make_unique<FBO>(FboInitParams{s_glbase, s_scrWidth, s_scrHeight, 1, colBufType, GL_TEXTURE_2D, true, 1, 1, 1, GL_REPEAT, false});

    string vSmapShader = ShaderCollector::getShaderHeader();
    vSmapShader +=
        STRINGIFY(layout(location = 0) in vec4 position; layout(location = 4) in mat4 modMatr;
                  uniform int useInstancing; uniform mat4 modelMatrix; uniform mat4 viewMatrix;
                  uniform mat4 projectionMatrix; mat4 model_view_matrix; out vec4 v_position; void main() {
                      model_view_matrix = viewMatrix * (useInstancing == 0 ? modelMatrix : modMatr);
                      v_position        = model_view_matrix * position;
                      gl_Position       = projectionMatrix * model_view_matrix * position;
                  });

    string fSmapShader = ShaderCollector::getShaderHeader();
    fSmapShader += STRINGIFY(uniform float u_LinearDepthConstant; in vec4 v_position;
                             layout(location = 0) out vec4 color; void main() {
                                 float linearDepth = length(v_position.xyz) * u_LinearDepthConstant;
                                 color.r           = linearDepth;
                             });

    s_shadowShader = s_glbase->shaderCollector().add("ShadowMapEsm", vSmapShader, fSmapShader);
}

void ShadowMapEsm::begin() {
    /*
// Matrices used when rendering from the light’s position
s_shadowShader->begin();
s_shadowShader->setUniform1f("u_LinearDepthConstant", linearDepthScalar);
s_shadowShader->setUniformMatrix4fv("modelMatrix", s_cs->model_matrix, 1);
s_shadowShader->setUniformMatrix4fv("viewMatrix", &light_view_matrix[0][0], 1);
s_shadowShader->setUniformMatrix4fv("projectionMatrix",
&light_projection_matrix[0][0], 1);

s_fbo->bind();
s_fbo->clearWhite(); // clear on white, since we need depth values

glEnable(GL_CULL_FACE); // cull front faces - this helps with artifacts and
shadows with exponential shadow mapping glCullFace(GL_BACK);

// scene needs to be rendered here
_scene->draw(time, dt, cp, s_shadowShader);

glDisable(GL_CULL_FACE);

s_fbo->unbind();

s_shadowShader->end();
*/
}

void ShadowMapEsm::end() {}

GLenum ShadowMapEsm::getColorBufType() const {
    return colBufType;
}
}  // namespace ara
