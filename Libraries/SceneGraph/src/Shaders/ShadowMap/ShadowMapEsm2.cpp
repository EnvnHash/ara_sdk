//
//  ShadowMapEsm2.cpp
//
//  Created by Sven Hahne on 11.08.17.
//
//  Generate a Shadow map for Variance Shadow Mapping
//

#include "ShadowMapEsm2.h"

#include <GLBase.h>

#include "CameraSets/CameraSet.h"

using namespace glm;
using namespace std;

namespace ara {
ShadowMapEsm2::ShadowMapEsm2(Camera* gCam, int scrWidth, int scrHeight, vec3 lightPos, float near_lim, float far_lim,
                             sceneData* scd)
    : ShadowMap(scd->glbase), m_gCam(gCam), m_lightPosition(lightPos), f_near(near_lim), f_far(far_lim),
      nmatLoc(-1) {
    s_scrWidth        = static_cast<int>(static_cast<float>(scrWidth) * s_shadow_map_coef);
    s_scrHeight       = static_cast<int>(static_cast<float>(scrHeight) * s_shadow_map_coef);
    colBufType        = GL_R32F;
    s_shadow_map_coef = 0.5f;
    lookAtPoint       = vec3(0.0f, 0.0f, 0.0f);
    m_shadowMatr        = mat4(1.0f);
    m_scaleBiasMatrix = mat4(vec4(0.5f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.5f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.5f, 0.0f),
                             vec4(0.5f, 0.5f, 0.5f, 1.0f));

    m_lightCam = new Camera();
    m_lightCam->setupPerspective(45.0f, f_near, f_far, s_scrWidth, s_scrHeight);
    m_lightCam->setCamPos(m_lightPosition);
    m_lightCam->setLookAt(lookAtPoint);
    m_lightCam->setType(camType::frustum);
    m_lightCam->buildMatrices();
    m_lightCam->setModelMatr(m_gCam->getModelMatr());
    linearDepthScalar = 1.f / (f_far - f_near);  // this helps us remap depth values to be linear

    string vSmapShader = ShaderCollector::getShaderHeader();
    vSmapShader +=
        STRINGIFY(layout(location = 0) in vec4 position; layout(location = 4) in mat4 modMatr;
                  uniform int useInstancing; uniform mat4 model_matrix; uniform mat4 view_matrix;
                  uniform mat4 projection_matrix; out vec4 v_position; mat4 model_view_matrix; void main() {
                      model_view_matrix = view_matrix * (useInstancing == 0 ? model_matrix : modMatr);
                      v_position        = model_view_matrix * position;
                      gl_Position       = projection_matrix * (model_view_matrix * position);
                  });

    string fSmapShader = ShaderCollector::getShaderHeader();
    fSmapShader += STRINGIFY(uniform float u_LinearDepthConstant; in vec4 v_position;
                             layout(location = 0) out vec4 color; void main() {
                                 float linearDepth = length(v_position.xyz) * u_LinearDepthConstant;
                                 color.r           = linearDepth;
                             });

    m_depthShader = s_glbase->shaderCollector().add("ShadowMapEsm2", vSmapShader, fSmapShader);

    // s_fbo for saving the depth information
    m_fbo = make_unique<FBO>(FboInitParams{s_glbase, s_scrWidth, s_scrHeight, 1, colBufType, GL_TEXTURE_2D, false, 1, 1, 1,
                                         GL_REPEAT, false});
}

void ShadowMapEsm2::begin() {
}

void ShadowMapEsm2::end() {
}

GLuint ShadowMapEsm2::getDepthImg() {
    return m_fbo->getDepthImg();
}

GLuint ShadowMapEsm2::getColorImg() {
    return m_fbo->getColorImg();
}

mat4 ShadowMapEsm2::getLightProjMatr() const {
    return m_lightCam->getProjectionMatr();
}

mat4 ShadowMapEsm2::lightViewMatr() const {
    return m_lightCam->getViewMatr();
}

float* ShadowMapEsm2::getShadowMatr() {
    m_shadowMatr = m_scaleBiasMatrix * m_lightCam->getProjectionMatr() * m_lightCam->getViewMatr();
    return &m_shadowMatr[0][0];
}

// noch schmutzig, könnte noch eleganter sein....
void ShadowMapEsm2::setLightPos(vec3 pos) {
    m_lightPosition = pos;
    m_lightCam->setupPerspective(45.0f, f_near, f_far, s_scrWidth, s_scrHeight);
    m_lightCam->setCamPos(m_lightPosition);
    m_lightCam->setLookAt(lookAtPoint);
    m_lightCam->setType(camType::frustum);
    m_lightCam->buildMatrices();
    m_lightCam->setModelMatr(m_gCam->getModelMatr());
}

void ShadowMapEsm2::setLookAtPoint(vec3 pos) {
    lookAtPoint = pos;
}

int ShadowMapEsm2::getWidth() {
    return s_scrWidth;
}

int ShadowMapEsm2::getHeight() {
    return s_scrHeight;
}

float ShadowMapEsm2::getCoef() const {
    return s_shadow_map_coef;
}

FBO* ShadowMapEsm2::getFbo() {
    return m_fbo.get();
}

float ShadowMapEsm2::getLinearDepthScalar() const {
    return linearDepthScalar;
}

GLenum ShadowMapEsm2::getColorBufType() const {
    return colBufType;
}
}  // namespace ara
