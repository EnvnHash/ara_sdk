//
//  LightShaderProperties.h
//
//  Created by Sven Hahne on 16.07.14.
//

#pragma once

#include <Shaders/ShaderUtils/ShaderProperties.h>

namespace ara {

// SHOULD BE 32 BIT aligned!!!
class LightPar {
public:
    LightPar() = default;
    glm::vec4 ambientColor{0.1f, 0.1f, 0.1f, 0.f};  // material parameter
    glm::vec4 LColor{1.f, 1.f, 1.f, 1.f};           // Light Parameter
    glm::vec4 LPosition{0.f, 0.f, 0.f, 1.f};        // location of the light, eye space
    glm::vec4 LDirection{0.f, 0.f, -1.f, 0.f};      // location of the light, eye space
    glm::vec4 eyeDirection{0.f, 0.f, -1.f, 0.f};
    glm::vec4 halfVector{0.f, 0.f, 0.f, 0.f};
    glm::vec4 coneDirection{0.f, 0.f, -1.f, 0.f};  // adding spotlight attributes
    float     constantAttenuation  = 0.f;
    float     linearAttenuation    = 0.f;
    float     quadraticAttenuation = 0.f;
    float     spotCosCutoff        = 0.f;  // how wide the spot is, as a cosine
    float     spotExponent         = 0.f;  // control light fall-off in the spot
    float     lightMode            = 0;
    float     aspect               = 0.f;
    float     throwRatio           = 0.f;
};

class LightShaderProperties : public ShaderProperties {
public:
    LightShaderProperties();
    ~LightShaderProperties() = default;

    void enable(bool _b);

    void*       getPtr(std::string name);
    std::string getLightParStruct();

    glm::vec3 getAmbientColor();
    glm::vec3 getColor();
    glm::vec3 getSpecular();
    glm::vec3 getPosition();
    glm::vec3 getDirection();
    glm::vec3 getHalfVector();
    glm::vec3 getConeDirection();
    glm::vec3 getEyeDirection();
    GLfloat   getSpotCosCutoff();
    GLfloat   getSpotExponent();
    GLfloat   getConstantAttenuation();
    GLfloat   getLinearAttenuation();
    GLfloat   getQuadraticAttenuation();
    GLfloat   getScaleFactor();

    //	void setUpStd();
    //	void isLocal(bool _b);
    //	void isSpot(bool _b);
    void setAmbientColor(GLfloat _r, GLfloat _g, GLfloat _b);
    void setColor(GLfloat _r, GLfloat _g, GLfloat _b);
    void setPosition(GLfloat _x, GLfloat _y, GLfloat _z);
    void setDirection(GLfloat _x, GLfloat _y, GLfloat _z);
    void setHalfVector(GLfloat _x, GLfloat _y, GLfloat _z);
    void setConeDirection(GLfloat _x, GLfloat _y, GLfloat _z);
    void setEyeDirection(GLfloat _x, GLfloat _y, GLfloat _z);
    void setSpotCosCutoff(GLfloat _val);
    void setSpotExponent(GLfloat _val);
    void setConstantAttenuation(GLfloat _val);
    void setLinearAttenuation(GLfloat _val);
    void setQuadraticAttenuation(GLfloat _val);
    void setScaleFactor(GLfloat _val);
    void setLightMode(GLfloat _val);

    glm::vec4 getLightPosition();
    GLfloat*  getLightPosFv();
};
}  // namespace ara
