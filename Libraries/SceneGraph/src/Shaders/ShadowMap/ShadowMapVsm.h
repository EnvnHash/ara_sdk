//
//  SHShadow.h
//
//  Created by Sven Hahne on 17.07.14.
//

#pragma once

#include "Shaders/Shaders.h"
#include "Shaders/ShadowMap/ShadowMap.h"
#include "Utils/Camera.h"
#include "Utils/FBO.h"
#include "glb_common/glb_common.h"

using namespace glm;

namespace ara {
class ShadowMapVsm : public ShadowMap {
public:
    ShadowMapVsm(Camera* gCam, int scrWidth, int scrHeight, sceneData* scd);
    ~ShadowMapVsm();

    void begin();
    void end();
    GLuint    getDepthImg();
    GLuint    getColorImg();
    glm::mat4 getLightProjMatr();
    glm::mat4 lightViewMatr();
    void      setLightPos(vec3 _pos);
    void      setLookAtPoint(vec3 _pos);
    int       getWidth();
    int       getHeight();
    float     getCoef();
    FBO*      getFbo();

private:
    std::unique_ptr<Shaders> shadowShader;
    Camera*                  gCam;

    glm::mat4 n_mvp;
    glm::mat3 n_mat;
    glm::mat4 scene_model_matrix;
    glm::mat4 light_view_matrix;
    glm::mat4 light_projection_matrix;

    glm::vec3 light_position;
    glm::vec3 lookAtPoint;
};
}  // namespace ara
