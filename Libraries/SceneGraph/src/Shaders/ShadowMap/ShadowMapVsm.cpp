//
//  ShadowMapVsm.cpp
//
//  Created by Sven Hahne on 12.07.14.
//
//  Generate a Shadow map for Variance Shadow Mapping
//

#include "Shaders/ShadowMap/ShadowMapVsm.h"

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {

ShadowMapVsm::ShadowMapVsm(Camera* gCam, int scrWidth, int scrHeight, sceneData* scd)
    : ShadowMap(scd->glbase) {
    m_gCam            = gCam;
    s_shadow_map_coef = 1.0f;
    s_scrWidth        = static_cast<int>(static_cast<float>(scrWidth) * s_shadow_map_coef);
    s_scrHeight       = static_cast<int>(static_cast<float>(scrHeight) * s_shadow_map_coef);

    string vSmapShader = ShaderCollector::getShaderHeader();
    vSmapShader += STRINGIFY(
        layout(location = 0) in vec4 position;
        layout(location = 4) in mat4 modMatr;
        uniform int useInstancing;
        uniform mat4 model_matrix;
        uniform mat4 view_matrix;
        uniform mat4 projection_matrix;
        mat4 model_view_matrix;
        out vec4 v_position;
        void main() {
              model_view_matrix = view_matrix * (useInstancing == 0 ? model_matrix : modMatr);
              gl_Position       = projection_matrix * (model_view_matrix * position);
              v_position        = gl_Position;
        });

    string fSmapShader = ShaderCollector::getShaderHeader();
    fSmapShader += STRINGIFY(in vec4 v_position; layout(location = 0) out vec4 color; void main() {
        float depth = v_position.z / v_position.w;
        // Don't forget to move away from unit cube ([-1,1]) to [0,1] coordinate system
        depth         = depth * 0.5 + 0.5;
        float moment1 = depth;
        // Adjusting moments (this is sort of bias per pixel) using derivate
        float dx      = dFdx(depth);
        float dy      = dFdy(depth);
        float moment2 = depth * depth + 0.25 * (dx * dx + dy * dy);
        color         = vec4(moment1, moment2, 0.0, 0.0);
    });

    m_shadowShader = make_unique<Shaders>(vSmapShader, nullptr, fSmapShader, false);

    // s_fbo for saving the depth information
    s_fbo = make_unique<FBO>(FboInitParams{s_glbase, s_scrWidth, s_scrHeight, 1, GL_RGB16F, GL_TEXTURE_2D, true, 1, 1, 1,
                                       GL_REPEAT, false});

    m_lightPosition = {-0.5f, 1.0f, 2.0f};
    m_lookAtPoint   = {0.0f, 0.0f, 0.0f};
}

void ShadowMapVsm::begin() {
}

void ShadowMapVsm::end() {
}

GLuint ShadowMapVsm::getDepthImg() {
    return s_fbo->getDepthImg();
}

GLuint ShadowMapVsm::getColorImg() {
    return s_fbo->getColorImg();
}

mat4 ShadowMapVsm::getLightProjMatr() const {
    return m_lightProjectionMatrix;
}

mat4 ShadowMapVsm::lightViewMatr() const {
    return m_lightViewMatrix;
}

void ShadowMapVsm::setLightPos(vec3 pos) {
    m_lightPosition = pos;
}

void ShadowMapVsm::setLookAtPoint(vec3 pos) {
    m_lookAtPoint = pos;
}

int ShadowMapVsm::getWidth() {
    return s_scrWidth;
}

int ShadowMapVsm::getHeight() {
    return s_scrHeight;
}

float ShadowMapVsm::getCoef() const {
    return s_shadow_map_coef;
}

FBO* ShadowMapVsm::getFbo() {
    return s_fbo.get();
}
}  // namespace ara
