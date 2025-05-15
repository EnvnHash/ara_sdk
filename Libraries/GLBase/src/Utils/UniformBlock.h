//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include <GlbCommon/GlbCommon.h>

namespace ara {

class ubBlockVar {
public:
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
    void                     addVarName(const std::string& name, void *inVal, GLenum type);
    void                     changeVarName(const std::string &name, void *inVal, GLenum type);
    void                     update();
    void                     removeGLResources();
    static std::size_t       TypeSize(GLenum type);
    [[nodiscard]] bool       isInited() const { return m_inited; }
    std::vector<ubBlockVar> *getValPairs() { return &m_valPairs; }
    ubBlockVar              *getVar(const std::string &name);
    [[nodiscard]] GLuint     getProgram() const { return m_program; }

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
