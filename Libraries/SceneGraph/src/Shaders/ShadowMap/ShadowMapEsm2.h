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

namespace ara {
class ShadowMapEsm2 : public ShadowMap {
public:
    ShadowMapEsm2(Camera* _gCam, int _scrWidth, int _scrHeight, glm::vec3 _lightPos, float _near, float _far,
                  sceneData* _scd);
    ~ShadowMapEsm2();

    void begin();
    void end();
    GLuint    getDepthImg();
    GLuint    getColorImg();
    glm::mat4 getLightProjMatr();
    glm::mat4 lightViewMatr();
    float*    getShadowMatr();
    void      setLightPos(glm::vec3 _pos);
    void      setLookAtPoint(glm::vec3 _pos);
    int       getWidth();
    int       getHeight();
    float     getCoef();
    FBO*      getFbo();
    float     getLinearDepthScalar();
    GLenum    getColorBufType();

private:
    Shaders* depthShader;
    Camera*  gCam;
    Camera*  lightCam;
    // SceneNode*  scene;
    std::unique_ptr<FBO> fbo;
    glm::mat4            n_mvp;
    glm::mat3            n_mat;
    glm::mat4            light_view_matrix;
    glm::mat4            light_projection_matrix;
    glm::mat4            shadowMatr;
    glm::mat4            scale_bias_matrix;

    glm::vec3 light_position;
    glm::vec3 lookAtPoint;

    int nmatLoc;

    float linearDepthScalar;
    float f_near;
    float f_far;

    GLenum colBufType;
};
}  // namespace ara
