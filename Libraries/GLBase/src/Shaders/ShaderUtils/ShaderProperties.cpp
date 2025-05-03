//
//  ShaderProperties.cpp
//
//  Created by Sven Hahne on 16.07.14.
//

#include "ShaderProperties.h"

using namespace glm;
using namespace std;

namespace ara {

void ShaderProperties::sendToShader(GLuint prog) {
    if (m_useUbBlock) {
        if (!m_ub.isInited() && !parameters.empty()) {
            m_ub.init(prog, m_ubBlockName);

            for (auto &parameter : parameters) {
                m_ub.addVarName(parameter.first, parameter.second.valPtr, parameter.second.type);
            }

            update();
        }

        if (m_ub.isInited()) m_ub.bind();
    } else {
        for (auto &parameter : parameters) {
            auto type = &parameter.second;

            if (type->isSet) {
                type->location = glGetUniformLocation(prog, parameter.first.c_str());
                sendUniform(&parameter.first, type);
            }
        }
    }
}

void ShaderProperties::sendUniform(const std::string *name, UniformType *_par) {
    if (_par->type == GL_BOOL)
        glUniform1i(_par->location, _par->bVal);
    else if (_par->type == GL_FLOAT)
        glUniform1f(_par->location, _par->fVal);
    else if (_par->type == GL_FLOAT_VEC2)
        glUniform2fv(_par->location, 1, &_par->f2Val[0]);
    else if (_par->type == GL_FLOAT_VEC3)
        glUniform3fv(_par->location, 1, &_par->f3Val[0]);
    else if (_par->type == GL_FLOAT_VEC4)
        glUniform4fv(_par->location, 1, &_par->f4Val[0]);
}

std::string ShaderProperties::getUbString() {
    std::string str = "uniform " + m_ubBlockName + "{\n";

    for (auto &parameter : parameters) {
        str += "\t ";

        switch (parameter.second.type) {
            case GL_BOOL: str += "bool"; break;
            case GL_FLOAT: str += "float"; break;
            case GL_FLOAT_VEC2: str += "vec2"; break;
            case GL_FLOAT_VEC3: str += "vec3"; break;
            case GL_FLOAT_VEC4: str += "vec4"; break;
            case GL_INT: str += "int"; break;
            case GL_INT_VEC2: str += "ivec2"; break;
            case GL_INT_VEC3: str += "ivec3"; break;
            case GL_INT_VEC4: str += "ivec4"; break;
            case GL_UNSIGNED_INT: str += "uint"; break;
            case GL_UNSIGNED_INT_VEC2: str += "uivec2"; break;
            case GL_UNSIGNED_INT_VEC3: str += "uivec3"; break;
            case GL_UNSIGNED_INT_VEC4: str += "uivec4"; break;
            case GL_FLOAT_MAT3: str += "mat3"; break;
            case GL_FLOAT_MAT4: str += "mat4"; break;
            default: break;
        }
        str += " " + parameter.first + ";\n";
    }

    str += "};\n";

    // LOG << str;

    return std::move(str);
}

bool ShaderProperties::hasName(const std::string &name) {
    auto it = parameters.find(name);

    if (it != parameters.end())
        return true;
    else
        return false;
}

GLboolean ShaderProperties::getBool(const std::string &name) {
    if (hasName(name)) {
        return parameters[name].bVal;
    } else {
        LOGE << "ShaderProperties::getBool ERROR: no parameter with name " << name.c_str();
        return 0;
    }
}

GLint ShaderProperties::getInt(const std::string &name) {
    if (hasName(name)) {
        return parameters[name].iVal;
    } else {
        LOGE << "ShaderProperties::getInt ERROR: no parameter with name " << name.c_str();
        return 0;
    }
}

ivec2 ShaderProperties::getInt2(const std::string &name) {
    if (hasName(name)) {
        return {parameters[name].i2Val[0], parameters[name].i2Val[1]};
    } else {
        LOGE << "ShaderProperties::getInt2 ERROR: no parameter with name " << name.c_str();
        return {0, 0};
    }
}

ivec3 ShaderProperties::getInt3(const std::string &name) {
    if (hasName(name)) {
        return {parameters[name].i3Val[0], parameters[name].i3Val[1], parameters[name].i3Val[2]};
    } else {
        LOGE << "ShaderProperties::getInt3 ERROR: no parameter with name " << name.c_str();
        return {0, 0, 0};
    }
}

ivec4 ShaderProperties::getInt4(const std::string &name) {
    if (hasName(name)) {
        return {parameters[name].i4Val[0], parameters[name].i4Val[1], parameters[name].i4Val[2],
                parameters[name].i4Val[3]};
    } else {
        LOGE << "ShaderProperties::getInt3 ERROR: no parameter with name " << name.c_str();
        return {0, 0, 0, 0};
    }
}

GLfloat ShaderProperties::getFloat(const std::string &name) {
    if (hasName(name)) {
        return parameters[name].fVal;
    } else {
        LOGE << "ShaderProperties::getFloat ERROR: no parameter with name " << name.c_str();
        return 0.f;
    }
}

vec2 ShaderProperties::getFloat2(const std::string &name) {
    if (hasName(name)) {
        return {parameters[name].f2Val[0], parameters[name].f2Val[1]};
    } else {
        LOGE << "ShaderProperties::getFloat ERROR: no parameter with name " << name.c_str();
        return {0.f, 0.f};
    }
}

vec3 ShaderProperties::getFloat3(const std::string &name) {
    if (hasName(name)) {
        return {parameters[name].f3Val[0], parameters[name].f3Val[1], parameters[name].f3Val[2]};
    } else {
        LOGE << "ShaderProperties::getFloat ERROR: no parameter with name " << name.c_str();
        return {0.f, 0.f, 0.f};
    }
}

vec4 ShaderProperties::getFloat4(const std::string &name) {
    if (hasName(name)) {
        return {parameters[name].f4Val[0], parameters[name].f4Val[1], parameters[name].f4Val[2],
                parameters[name].f4Val[3]};
    } else {
        LOGE << "ShaderProperties::getFloat ERROR: no parameter with name " << name.c_str();
        return {0.f, 0.f, 0.f, 0.f};
    }
}

void ShaderProperties::setBool(const std::string &name, GLboolean b) {
    if (!parameters[name].isSet) parameters[name].isSet = true;
    parameters[name].bVal   = b;
    parameters[name].valPtr = &parameters[name].bVal;
}

void ShaderProperties::setInt(const std::string &name, GLint x) {
    if (!parameters[name].isSet) parameters[name].isSet = true;
    parameters[name].iVal   = x;
    parameters[name].valPtr = &parameters[name].iVal;
}

void ShaderProperties::setInt2(const std::string &name, GLint x, GLint y) {
    if (!parameters[name].isSet) parameters[name].isSet = true;

    parameters[name].i2Val[0] = x;
    parameters[name].i2Val[1] = y;
    parameters[name].valPtr   = &parameters[name].i2Val[0];
}

void ShaderProperties::setInt3(const std::string &name, GLint x, GLint y, GLint z) {
    if (!parameters[name].isSet) parameters[name].isSet = true;

    parameters[name].i3Val[0] = x;
    parameters[name].i3Val[1] = y;
    parameters[name].i3Val[2] = z;
    parameters[name].valPtr   = &parameters[name].i3Val[0];
}

void ShaderProperties::setInt4(const std::string &name, GLint x, GLint y, GLint z, GLint w) {
    if (!parameters[name].isSet) parameters[name].isSet = true;

    parameters[name].i4Val[0] = x;
    parameters[name].i4Val[1] = y;
    parameters[name].i4Val[2] = z;
    parameters[name].i4Val[3] = w;
    parameters[name].valPtr   = &parameters[name].i4Val[0];
}

void ShaderProperties::setFloat(const std::string &name, GLfloat _f) {
    if (!parameters[name].isSet) parameters[name].isSet = true;
    parameters[name].fVal   = _f;
    parameters[name].valPtr = &parameters[name].fVal;
}

void ShaderProperties::setFloat2(const std::string &name, GLfloat r, GLfloat g) {
    if (!parameters[name].isSet) parameters[name].isSet = true;

    parameters[name].f2Val[0] = r;
    parameters[name].f2Val[1] = g;
    parameters[name].valPtr   = &parameters[name].f2Val[0];
}

void ShaderProperties::setFloat3(const std::string &name, GLfloat r, GLfloat g, GLfloat b) {
    if (!parameters[name].isSet) parameters[name].isSet = true;

    parameters[name].f3Val[0] = r;
    parameters[name].f3Val[1] = g;
    parameters[name].f3Val[2] = b;
    parameters[name].valPtr   = &parameters[name].f3Val[0];
}

void ShaderProperties::setFloat4(const std::string &name, GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    if (!parameters[name].isSet) parameters[name].isSet = true;

    parameters[name].f4Val[0] = r;
    parameters[name].f4Val[1] = g;
    parameters[name].f4Val[2] = b;
    parameters[name].f4Val[3] = a;
    parameters[name].valPtr   = &parameters[name].f4Val[0];
}

}  // namespace ara
