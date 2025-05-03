//
//  LightShaderProperties.cpp
//
//  Created by Sven Hahne on 14.08.17
//

#include "LightShaderProperties.h"

using namespace glm;
using namespace std;

namespace ara {

LightShaderProperties::LightShaderProperties() : ShaderProperties() {
    // light’s contribution to ambient light
    parameters["ambientColor"].type = GL_FLOAT_VEC4;

    // color of light
    parameters["LColor"].type       = GL_FLOAT_VEC4;
    parameters["LSpecular"].type    = GL_FLOAT_VEC4;

    // location of light, if is Local is true, otherwise the direction toward the light
    parameters["LPosition"].type    = GL_FLOAT_VEC4;

    // direction of highlights for directional light
    parameters["LDirection"].type   = GL_FLOAT_VEC4;
    parameters["halfVector"].type   = GL_FLOAT_VEC4;

    // spotlight attributes
    parameters["coneDirection"].type        = GL_FLOAT_VEC4;
    parameters["eyeDirection"].type         = GL_FLOAT_VEC4;
    parameters["spotCosCutoff"].type        = GL_FLOAT;
    parameters["spotExponent"].type         = GL_FLOAT;
    parameters["constantAttenuation"].type  = GL_FLOAT;
    parameters["linearAttenuation"].type    = GL_FLOAT;
    parameters["quadraticAttenuation"].type = GL_FLOAT;
    parameters["scaleFactor"].type          = GL_FLOAT;
    parameters["lightMode"].type            = GL_FLOAT;
    parameters["aspect"].type               = GL_FLOAT;
    parameters["throwRatio"].type           = GL_FLOAT;
}

void LightShaderProperties::enable(bool b) {
    setBool("isEnabled", b);
}

void* LightShaderProperties::getPtr(const std::string& name) {
    auto it = parameters.find(name);

    if (it != parameters.end()) {
        return it->second.valPtr;
    } else {
        LOGE << "LightShaderProperties::getPtr ERROR no parameter with name: " << name.c_str();
        return nullptr;
    }
}

std::string LightShaderProperties::getLightParStruct() {
    // SHOULD BE 32 BIT ALIGNED
    return "\nstruct LightPar {\n"
           "vec4 ambientColor; \n"  // material parameter
           "vec4 LColor; \n"        // Light Parameter
           "vec4 LPosition; \n"     // location of the light, eye space
           "vec4 LDirection; \n"    // location of the light, eye space
           "vec4 eyeDirection; \n"
           "vec4 halfVector; \n"
           "vec4 coneDirection; \n	"  // adding spotlight attributes
           "float constantAttenuation; \n"
           "float linearAttenuation; \n"
           "float quadraticAttenuation; \n"
           "float spotCosCutoff; \n"  // how wide the spot is, as a cosine
           "float spotExponent; \n"   // control light fall-off in the spot
           "float lightMode; \n"      // material parameter
           "float aspect; \n"
           "float throwRatio; \n"
           "};\n";
}

vec3 LightShaderProperties::getAmbientColor() {
    return getFloat4("ambientColor");
}

vec3 LightShaderProperties::getColor() {
    return getFloat4("LColor");
}

vec3 LightShaderProperties::getSpecular() {
    return getFloat4("LSpecular");
}

vec3 LightShaderProperties::getPosition() {
    return getFloat4("LPosition");
}

vec3 LightShaderProperties::getDirection() {
    return getFloat4("LDirection");
}

vec3 LightShaderProperties::getHalfVector() {
    return getFloat4("halfVector");
}

vec3 LightShaderProperties::getConeDirection() {
    return getFloat4("coneDirection");
}

vec3 LightShaderProperties::getEyeDirection() {
    return getFloat4("eyeDirection");
}

GLfloat LightShaderProperties::getSpotCosCutoff() {
    return getFloat("spotCosCutoff");
}

GLfloat LightShaderProperties::getSpotExponent() {
    return getFloat("spotExponent");
}

GLfloat LightShaderProperties::getConstantAttenuation() {
return getFloat("constantAttenuation"); }

GLfloat LightShaderProperties::getLinearAttenuation() {
    return getFloat("linearAttenuation");
}

GLfloat LightShaderProperties::getQuadraticAttenuation() {
    return getFloat("quadraticAttenuation");
}

GLfloat LightShaderProperties::getScaleFactor() {
    return getFloat("scaleFactor");
}

void LightShaderProperties::setAmbientColor(GLfloat r, GLfloat g, GLfloat b) {
    setFloat4("ambientColor", r, g, b, 1.f);
}

void LightShaderProperties::setColor(GLfloat r, GLfloat g, GLfloat b) {
    setFloat4("LColor", r, g, b, 1.f);
}

void LightShaderProperties::setPosition(GLfloat x, GLfloat y, GLfloat z) {
    setFloat4("LPosition", x, y, z, 1.f);
}

void LightShaderProperties::setDirection(GLfloat x, GLfloat y, GLfloat z) {
    setFloat4("LDirection", x, y, z, 0.f);
}

void LightShaderProperties::setHalfVector(GLfloat x, GLfloat y, GLfloat z) {
    setFloat4("halfVector", x, y, z, 0.f);
}

void LightShaderProperties::setConeDirection(GLfloat x, GLfloat y, GLfloat z) {
    setFloat4("coneDirection", x, y, z, 0.f);
}

void LightShaderProperties::setEyeDirection(GLfloat x, GLfloat y, GLfloat z) {
    setFloat4("eyeDirection", x, y, z, 0.f);
}

void LightShaderProperties::setSpotCosCutoff(GLfloat val) {
    setFloat("spotCosCutoff", val);
}

void LightShaderProperties::setSpotExponent(GLfloat val) {
    setFloat("spotExponent", val);
}

void LightShaderProperties::setConstantAttenuation(GLfloat val) {
    setFloat("constantAttenuation", val);
}

void LightShaderProperties::setLinearAttenuation(GLfloat val) {
    setFloat("linearAttenuation", val);
}

void LightShaderProperties::setQuadraticAttenuation(GLfloat val) {
    setFloat("quadraticAttenuation", val);
}

void LightShaderProperties::setScaleFactor(GLfloat val) {
    setFloat("scaleFactor", val);
}

void LightShaderProperties::setLightMode(GLfloat val) {
    setFloat("lightMode", val);
}

vec4 LightShaderProperties::getLightPosition() {
    return {parameters["LDirection"].f4Val[0], parameters["LDirection"].f4Val[1],
              parameters["LDirection"].f4Val[2], parameters["LDirection"].f4Val[3]};
}

GLfloat* LightShaderProperties::getLightPosFv() { return &parameters["LPosition"].f3Val[0]; }
}  // namespace ara
