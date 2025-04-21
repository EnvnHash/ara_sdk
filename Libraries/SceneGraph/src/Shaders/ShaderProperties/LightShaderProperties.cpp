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
    /*
    // true to apply this light in this invocation
    parameters["isEnabled"] = UniformType();
    parameters["isEnabled"].type = GL_BOOL;

    // true for a point light or a spotlight,  false for a positional light
    parameters["isLocal"] = UniformType();
    parameters["isLocal"].type = GL_BOOL;

    // true if the light is a spotlight
    parameters["isSpot"] = UniformType();
    parameters["isSpot"].type = GL_BOOL;
    */

    // light’s contribution to ambient light
    parameters["ambientColor"]      = UniformType();
    parameters["ambientColor"].type = GL_FLOAT_VEC4;

    // color of light
    parameters["LColor"]      = UniformType();
    parameters["LColor"].type = GL_FLOAT_VEC4;

    parameters["LSpecular"]      = UniformType();
    parameters["LSpecular"].type = GL_FLOAT_VEC4;

    // location of light, if is Local is true, otherwise the direction toward
    // the light
    parameters["LPosition"]      = UniformType();
    parameters["LPosition"].type = GL_FLOAT_VEC4;

    // direction of highlights for directional light
    parameters["LDirection"]      = UniformType();
    parameters["LDirection"].type = GL_FLOAT_VEC4;

    parameters["halfVector"]      = UniformType();
    parameters["halfVector"].type = GL_FLOAT_VEC4;

    // spotlight attributes
    parameters["coneDirection"]      = UniformType();
    parameters["coneDirection"].type = GL_FLOAT_VEC4;

    parameters["eyeDirection"]      = UniformType();
    parameters["eyeDirection"].type = GL_FLOAT_VEC4;

    parameters["spotCosCutoff"]      = UniformType();
    parameters["spotCosCutoff"].type = GL_FLOAT;

    parameters["spotExponent"]      = UniformType();
    parameters["spotExponent"].type = GL_FLOAT;

    parameters["constantAttenuation"]      = UniformType();
    parameters["constantAttenuation"].type = GL_FLOAT;

    parameters["linearAttenuation"]      = UniformType();
    parameters["linearAttenuation"].type = GL_FLOAT;

    parameters["quadraticAttenuation"]      = UniformType();
    parameters["quadraticAttenuation"].type = GL_FLOAT;

    parameters["scaleFactor"]      = UniformType();
    parameters["scaleFactor"].type = GL_FLOAT;

    parameters["lightMode"]      = UniformType();
    parameters["lightMode"].type = GL_FLOAT;

    parameters["aspect"]      = UniformType();
    parameters["aspect"].type = GL_FLOAT;

    parameters["throwRatio"]      = UniformType();
    parameters["throwRatio"].type = GL_FLOAT;
}

void LightShaderProperties::enable(bool _b) { setBool("isEnabled", _b); }

void* LightShaderProperties::getPtr(std::string name) {
    auto it = parameters.find(name);

    if (it != parameters.end()) {
        return (*it).second.valPtr;
    } else {
        LOG << "LightShaderProperties::getPtr ERROR no parameter with name: " << name.c_str();
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

glm::vec3 LightShaderProperties::getAmbientColor() { return getFloat4("ambientColor"); }

glm::vec3 LightShaderProperties::getColor() { return getFloat4("LColor"); }

glm::vec3 LightShaderProperties::getSpecular() { return getFloat4("LSpecular"); }

glm::vec3 LightShaderProperties::getPosition() { return getFloat4("LPosition"); }

glm::vec3 LightShaderProperties::getDirection() { return getFloat4("LDirection"); }

glm::vec3 LightShaderProperties::getHalfVector() { return getFloat4("halfVector"); }

glm::vec3 LightShaderProperties::getConeDirection() { return getFloat4("coneDirection"); }

glm::vec3 LightShaderProperties::getEyeDirection() { return getFloat4("eyeDirection"); }

GLfloat LightShaderProperties::getSpotCosCutoff() { return getFloat("spotCosCutoff"); }

GLfloat LightShaderProperties::getSpotExponent() { return getFloat("spotExponent"); }

GLfloat LightShaderProperties::getConstantAttenuation() { return getFloat("constantAttenuation"); }

GLfloat LightShaderProperties::getLinearAttenuation() { return getFloat("linearAttenuation"); }

GLfloat LightShaderProperties::getQuadraticAttenuation() { return getFloat("quadraticAttenuation"); }

GLfloat LightShaderProperties::getScaleFactor() { return getFloat("scaleFactor"); }

/*
void LightShaderProperties::isLocal(bool _b)
{
        if (!parTypes[isLoc].isSet)
                parTypes[isLoc].isSet = true;
        parTypes[isLoc].bVal = _b;
}

-
void LightShaderProperties::isSpot(bool _b)
{
        if (!parTypes[isSpt].isSet)
                parTypes[isSpt].isSet = true;
        parTypes[isSpt].bVal = _b;
}
*/

void LightShaderProperties::setAmbientColor(GLfloat _r, GLfloat _g, GLfloat _b) {
    setFloat4("ambientColor", _r, _g, _b, 1.f);
}

void LightShaderProperties::setColor(GLfloat _r, GLfloat _g, GLfloat _b) { setFloat4("LColor", _r, _g, _b, 1.f); }

void LightShaderProperties::setPosition(GLfloat _x, GLfloat _y, GLfloat _z) { setFloat4("LPosition", _x, _y, _z, 1.f); }

void LightShaderProperties::setDirection(GLfloat _x, GLfloat _y, GLfloat _z) {
    setFloat4("LDirection", _x, _y, _z, 0.f);
}

void LightShaderProperties::setHalfVector(GLfloat _x, GLfloat _y, GLfloat _z) {
    setFloat4("halfVector", _x, _y, _z, 0.f);
}

void LightShaderProperties::setConeDirection(GLfloat _x, GLfloat _y, GLfloat _z) {
    setFloat4("coneDirection", _x, _y, _z, 0.f);
}

void LightShaderProperties::setEyeDirection(GLfloat _x, GLfloat _y, GLfloat _z) {
    setFloat4("eyeDirection", _x, _y, _z, 0.f);
}

void LightShaderProperties::setSpotCosCutoff(GLfloat _val) { setFloat("spotCosCutoff", _val); }

void LightShaderProperties::setSpotExponent(GLfloat _val) { setFloat("spotExponent", _val); }

void LightShaderProperties::setConstantAttenuation(GLfloat _val) { setFloat("constantAttenuation", _val); }

void LightShaderProperties::setLinearAttenuation(GLfloat _val) { setFloat("linearAttenuation", _val); }

void LightShaderProperties::setQuadraticAttenuation(GLfloat _val) { setFloat("quadraticAttenuation", _val); }

void LightShaderProperties::setScaleFactor(GLfloat _val) { setFloat("scaleFactor", _val); }

void LightShaderProperties::setLightMode(GLfloat _val) { setFloat("lightMode", _val); }

glm::vec4 LightShaderProperties::getLightPosition() {
    return glm::vec4(parameters["LDirection"].f4Val[0], parameters["LDirection"].f4Val[1],
                     parameters["LDirection"].f4Val[2], parameters["LDirection"].f4Val[3]);
}

GLfloat* LightShaderProperties::getLightPosFv() { return &parameters["LPosition"].f3Val[0]; }
}  // namespace ara
