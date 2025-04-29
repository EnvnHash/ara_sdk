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
ShadowMapEsm2::ShadowMapEsm2(Camera* _gCam, int _scrWidth, int _scrHeight, vec3 _lightPos, float _near, float _far,
                             sceneData* _scd)
    : ShadowMap((GLBase*)_scd->glbase), gCam(_gCam), light_position(_lightPos), f_near(_near), f_far(_far),
      nmatLoc(-1) {
    s_scrWidth        = static_cast<int>(static_cast<float>(_scrWidth) * s_shadow_map_coef);
    s_scrHeight       = static_cast<int>(static_cast<float>(_scrHeight) * s_shadow_map_coef);
    colBufType        = GL_R32F;
    s_shadow_map_coef = 0.5f;
    lookAtPoint       = vec3(0.0f, 0.0f, 0.0f);
    shadowMatr        = mat4(1.0f);
    scale_bias_matrix = mat4(vec4(0.5f, 0.0f, 0.0f, 0.0f), vec4(0.0f, 0.5f, 0.0f, 0.0f), vec4(0.0f, 0.0f, 0.5f, 0.0f),
                             vec4(0.5f, 0.5f, 0.5f, 1.0f));

    lightCam = new Camera();
    lightCam->setupPerspective(45.0f, f_near, f_far, s_scrWidth, s_scrHeight);
    lightCam->setCamPos(light_position);
    lightCam->setLookAt(lookAtPoint);
    lightCam->setType(camType::frustum);
    lightCam->buildMatrices();
    lightCam->setModelMatr(gCam->getModelMatr());
    linearDepthScalar = 1.0f / (f_far - f_near);  // this helps us remap depth values to be linear

    string vSmapShader = s_glbase->shaderCollector().getShaderHeader();
    vSmapShader +=
        STRINGIFY(layout(location = 0) in vec4 position; layout(location = 4) in mat4 modMatr;
                  uniform int useInstancing; uniform mat4 model_matrix; uniform mat4 view_matrix;
                  uniform mat4 projection_matrix; out vec4 v_position; mat4 model_view_matrix; void main() {
                      model_view_matrix = view_matrix * (useInstancing == 0 ? model_matrix : modMatr);
                      v_position        = model_view_matrix * position;
                      gl_Position       = projection_matrix * (model_view_matrix * position);
                  });

    string fSmapShader = s_glbase->shaderCollector().getShaderHeader();
    fSmapShader += STRINGIFY(uniform float u_LinearDepthConstant; in vec4 v_position;
                             layout(location = 0) out vec4 color; void main() {
                                 float linearDepth = length(v_position.xyz) * u_LinearDepthConstant;
                                 color.r           = linearDepth;
                             });

    depthShader = s_glbase->shaderCollector().add("ShadowMapEsm2", vSmapShader.c_str(), fSmapShader.c_str());

    // s_fbo for saving the depth information
    fbo = make_unique<FBO>(FboInitParams{s_glbase, s_scrWidth, s_scrHeight, 1, colBufType, GL_TEXTURE_2D, false, 1, 1, 1,
                                         GL_REPEAT, false});
}

ShadowMapEsm2::~ShadowMapEsm2() {}

void ShadowMapEsm2::begin() {
}

void ShadowMapEsm2::end() {}

GLuint ShadowMapEsm2::getDepthImg() { return fbo->getDepthImg(); }

GLuint ShadowMapEsm2::getColorImg() { return fbo->getColorImg(); }

mat4 ShadowMapEsm2::getLightProjMatr() { return lightCam->getProjectionMatr(); }

mat4 ShadowMapEsm2::lightViewMatr() { return lightCam->getViewMatr(); }

float* ShadowMapEsm2::getShadowMatr() {
    shadowMatr = scale_bias_matrix * lightCam->getProjectionMatr() * lightCam->getViewMatr();
    return &shadowMatr[0][0];
}

// noch schmutzig, könnte noch eleganter sein....
void ShadowMapEsm2::setLightPos(vec3 _pos) {
    light_position = _pos;
    lightCam->setupPerspective(45.0f, f_near, f_far, s_scrWidth, s_scrHeight);
    lightCam->setCamPos(light_position);
    lightCam->setLookAt(lookAtPoint);
    lightCam->setType(camType::frustum);
    lightCam->buildMatrices();
    lightCam->setModelMatr(gCam->getModelMatr());
}

void ShadowMapEsm2::setLookAtPoint(vec3 _pos) { lookAtPoint = _pos; }

int ShadowMapEsm2::getWidth() { return s_scrWidth; }

int ShadowMapEsm2::getHeight() { return s_scrHeight; }

float ShadowMapEsm2::getCoef() { return s_shadow_map_coef; }

FBO* ShadowMapEsm2::getFbo() { return fbo.get(); }

float ShadowMapEsm2::getLinearDepthScalar() { return linearDepthScalar; }

GLenum ShadowMapEsm2::getColorBufType() { return colBufType; }
}  // namespace ara
