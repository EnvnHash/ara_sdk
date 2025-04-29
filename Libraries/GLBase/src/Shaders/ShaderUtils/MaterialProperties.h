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
    void  setAmbient(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { setFloat4("ambient", r, g, b, a); }
    void  setDiffuse(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { setFloat4("diffuse", r, g, b, a); }
    void  setEmissive(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { setFloat4("emissive", r, g, b, a); }
    void  setSpecular(GLfloat r, GLfloat g, GLfloat b, GLfloat a) { setFloat4("specular", r, g, b, a); }
    void  setShininess(float val) { setFloat("shininess", val); }
    void  setStrength(float val) { setFloat("strength", val); }
};
}  // namespace ara
