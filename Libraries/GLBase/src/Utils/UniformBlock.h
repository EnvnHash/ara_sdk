//
//  UniformBlock.h
//
//  Created by Sven Hahne on 22/7/15.
//

#pragma once

#include <glb_common/glb_common.h>

namespace ara {

class ubBlockVar {
public:
    ubBlockVar() = default;

    std::string name;
    GLenum      inType  = 0;
    float      *fVal    = nullptr;
    bool       *bVal    = nullptr;
    uint32_t   *uVal    = nullptr;
    int        *iVal    = nullptr;
    void       *vVal    = nullptr;
    GLuint      indices = 0;
    GLint       size    = 0;
    GLint       offset  = 0;
    GLint       type    = 0;
};

class UniformBlock {
public:
    UniformBlock() = default;
    UniformBlock(GLuint program, const std::string &blockName);
    ~UniformBlock();

    void init(GLuint program, const std::string &blockName);
    void bind() const {
        glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
        glBindBufferBase(GL_UNIFORM_BUFFER, m_uboIndex, m_ubo);
    }
    static void              unbind() { glBindBuffer(GL_UNIFORM_BUFFER, 0); }
    void                     addVarName(std::string name, void *inVal, GLenum type);
    void                     changeVarName(const std::string &name, void *inVal, GLenum type);
    void                     update();
    static std::size_t       TypeSize(GLenum type);
    bool                     isInited() { return m_inited; }
    std::vector<ubBlockVar> *getValPairs() { return &m_valPairs; }
    ubBlockVar              *getVar(const std::string &name) {
        auto it = std::find_if(m_valPairs.begin(), m_valPairs.end(),
                                            [name](const ubBlockVar &ub) { return ub.name == name; });
        return (it != m_valPairs.end() ? &(*it) : nullptr);
    }
    GLuint getProgram() { return m_program; }

private:
    std::vector<ubBlockVar> m_valPairs;

    GLuint m_uboIndex = 0;
    GLint  m_uboSize  = 0;
    GLuint m_ubo      = 0;

    std::vector<GLubyte>      m_buffer;
    std::vector<GLuint>       m_indices;
    std::vector<GLint>        m_size;
    std::vector<GLint>        m_offset;
    std::vector<GLint>        m_type;
    std::vector<const char *> m_names;

    GLuint m_program     = 0;
    int    m_numUniforms = 0;
    bool   m_inited      = false;
};
}  // namespace ara
