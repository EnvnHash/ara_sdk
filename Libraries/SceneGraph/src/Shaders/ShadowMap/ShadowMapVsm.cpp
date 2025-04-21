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

ShadowMapVsm::ShadowMapVsm(Camera* _gCam, int _scrWidth, int _scrHeight, sceneData* _scd)
    : ShadowMap((GLBase*)_scd->glbase) {
    gCam              = _gCam;
    s_shadow_map_coef = 1.0f;
    s_scrWidth        = static_cast<int>(static_cast<float>(_scrWidth) * s_shadow_map_coef);
    s_scrHeight       = static_cast<int>(static_cast<float>(_scrHeight) * s_shadow_map_coef);

    string vSmapShader = s_glbase->getShaderHeader();
    vSmapShader +=
        STRINGIFY(layout(location = 0) in vec4 position; layout(location = 4) in mat4 modMatr;
                  uniform int useInstancing; uniform mat4 model_matrix; uniform mat4 view_matrix;
                  uniform mat4 projection_matrix; mat4 model_view_matrix; out vec4 v_position; void main(void) {
                      model_view_matrix = view_matrix * (useInstancing == 0 ? model_matrix : modMatr);
                      gl_Position       = projection_matrix * (model_view_matrix * position);
                      v_position        = gl_Position;
                  });

    string fSmapShader = "#version 330 core\n";
    fSmapShader += STRINGIFY(in vec4 v_position; layout(location = 0) out vec4 color; void main() {
        float depth = v_position.z / v_position.w;
        // Don't forget to move away from unit cube ([-1,1]) to [0,1]
        // coordinate system
        depth         = depth * 0.5 + 0.5;
        float moment1 = depth;
        // Adjusting moments (this is sort of bias per pixel) using
        // derivative
        float dx      = dFdx(depth);
        float dy      = dFdy(depth);
        float moment2 = depth * depth + 0.25 * (dx * dx + dy * dy);
        color         = vec4(moment1, moment2, 0.0, 0.0);
    });

    shadowShader = make_unique<Shaders>((char*)vSmapShader.c_str(), nullptr, (char*)fSmapShader.c_str(), false);

    // s_fbo for saving the depth information
    s_fbo =
        make_unique<FBO>(s_glbase, s_scrWidth, s_scrHeight, GL_RGB16F, GL_TEXTURE_2D, true, 1, 1, 1, GL_REPEAT, false);

    light_position = vec3(-0.5f, 1.0f, 2.0f);
    lookAtPoint    = vec3(0.0f, 0.0f, 0.0f);
}

ShadowMapVsm::~ShadowMapVsm() {}

void ShadowMapVsm::begin() {
    /*
// Matrices for rendering the scene
scene_model_matrix = gCam->getModelMatr();

// Matrices used when rendering from the light’s position
light_view_matrix = s_lookAt(light_position, lookAtPoint, vec3(0,1,0));
light_projection_matrix = frustum(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 10.0f);
//n_mvp = light_projection_matrix * light_view_matrix * scene_model_matrix;

s_shadowShader->begin();
//s_shadowShader->setUniformMatrix4fv("m_pvm", &n_mvp[0][0], 1);
s_shadowShader->setUniformMatrix4fv("model_matrix", &scene_model_matrix[0][0],
1); s_shadowShader->setUniformMatrix4fv("view_matrix", &light_view_matrix[0][0],
1); s_shadowShader->setUniformMatrix4fv("projection_matrix",
&light_projection_matrix[0][0], 1);

s_fbo->bind();
s_fbo->clearWhite();   // s_fbo must clear on white, for rendering depth values

glEnable(GL_CULL_FACE);
glCullFace(GL_BACK);    // render only backface, for avoiding self shadowing

// scene needs to be rendered here
_scene->draw(time, dt, cp, s_shadowShader);

glDisable(GL_CULL_FACE);

s_fbo->unbind();
s_shadowShader->end();
*/
}

void ShadowMapVsm::end() {}

GLuint ShadowMapVsm::getDepthImg() { return s_fbo->getDepthImg(); }

GLuint ShadowMapVsm::getColorImg() { return s_fbo->getColorImg(); }

mat4 ShadowMapVsm::getLightProjMatr() { return light_projection_matrix; }

mat4 ShadowMapVsm::lightViewMatr() { return light_view_matrix; }

void ShadowMapVsm::setLightPos(vec3 _pos) { light_position = _pos; }

void ShadowMapVsm::setLookAtPoint(vec3 _pos) { lookAtPoint = _pos; }

int ShadowMapVsm::getWidth() { return s_scrWidth; }

int ShadowMapVsm::getHeight() { return s_scrHeight; }

float ShadowMapVsm::getCoef() { return s_shadow_map_coef; }

FBO* ShadowMapVsm::getFbo() { return s_fbo.get(); }
}  // namespace ara
