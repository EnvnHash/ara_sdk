//
//	Standard OpenGL Material Definition (Obj-style)
//

#pragma once

#include "Shaders/ShaderUtils/ShaderProperties.h"

namespace ara {
class MaterialProperties : public ShaderProperties {
public:
    MaterialProperties();

    void setupStd();

    glm::vec4 getAmbient();
    glm::vec4 getDiffuse();
    glm::vec4 getEmissive();
    glm::vec4 getSpecular();

    float getShininess() { return parameters["shininess"].fVal; }
    float getStrength() { return parameters["strength"].fVal; }
    void  setAmbient(GLfloat _r, GLfloat _g, GLfloat _b, GLfloat _a) { setFloat4("ambient", _r, _g, _b, _a); }
    void  setDiffuse(GLfloat _r, GLfloat _g, GLfloat _b, GLfloat _a) { setFloat4("diffuse", _r, _g, _b, _a); }
    void  setEmissive(GLfloat _r, GLfloat _g, GLfloat _b, GLfloat _a) { setFloat4("emissive", _r, _g, _b, _a); }
    void  setSpecular(GLfloat _r, GLfloat _g, GLfloat _b, GLfloat _a) { setFloat4("specular", _r, _g, _b, _a); }
    void  setShininess(float _val) { setFloat("shininess", _val); }
    void  setStrength(float _val) { setFloat("strength", _val); }
};
}  // namespace ara
