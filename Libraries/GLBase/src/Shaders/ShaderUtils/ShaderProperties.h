//
//  ShaderProperties.h
//
//  Created by Sven Hahne on 16.07.14.
//

#pragma once

#include <Shaders/Shaders.h>
#include <Utils/UniformBlock.h>

namespace ara {

class ShaderProperties {
public:
    ShaderProperties() = default;
    virtual ~ShaderProperties() = default;

    struct UniformType {
        GLboolean              isSet    = false;
        GLint                  location = -1;
        GLboolean              bVal     = false;
        GLint                  iVal     = 0;
        std::array<GLint, 2>   i2Val    = {};
        std::array<GLint, 3>   i3Val    = {};
        std::array<GLint, 4>   i4Val    = {};
        GLfloat                fVal     = 0.f;
        std::array<GLfloat, 2> f2Val    = {};
        std::array<GLfloat, 3> f3Val    = {};
        std::array<GLfloat, 4> f4Val    = {};
        void                  *valPtr   = nullptr;
        GLenum                 type     = 0;
    };

    virtual void sendToShader(GLuint prog);
    virtual void sendUniform(const std::string *name, UniformType *par);

    void update() {
        if (m_useUbBlock && m_ub.isInited()) {
            m_ub.update();
        }
    }

    bool        hasName(const std::string &name);
    std::string getUbString();
    GLboolean   getBool(const std::string &name);
    GLint       getInt(const std::string &name);
    glm::ivec2  getInt2(const std::string &name);
    glm::ivec3  getInt3(const std::string &name);
    glm::ivec4  getInt4(const std::string &name);
    GLfloat     getFloat(const std::string &name);
    glm::vec2   getFloat2(const std::string &name);
    glm::vec3   getFloat3(const std::string &name);
    glm::vec4   getFloat4(const std::string &name);

    void setBool(const std::string &name, GLboolean b);
    void setInt(const std::string &name, GLint i);
    void setInt2(const std::string &name, GLint x, GLint y);
    void setInt3(const std::string &name, GLint x, GLint y, GLint z);
    void setInt4(const std::string &name, GLint x, GLint y, GLint z, GLint w);
    void setFloat(const std::string &name, GLfloat f);
    void setFloat2(const std::string &name, GLfloat r, GLfloat g);
    void setFloat3(const std::string &name, GLfloat r, GLfloat g, GLfloat b);
    void setFloat4(const std::string &name, GLfloat r, GLfloat g, GLfloat b, GLfloat a);

    std::unordered_map<std::string, UniformType> parameters;
    std::map<Shaders *, GLint>                   uBlockShaderLoc;
    UniformBlock                                 m_ub;

    bool        debug         = false;
    bool        m_useUbBlock  = false;
    std::string m_ubBlockName = "shdrPar";
};
}  // namespace ara
