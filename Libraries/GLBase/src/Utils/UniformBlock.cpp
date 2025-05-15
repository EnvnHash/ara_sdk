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


#include "Utils/UniformBlock.h"

namespace ara {

UniformBlock::UniformBlock(GLuint program, const std::string &blockName) {
    init(program, blockName);
}

void UniformBlock::init(GLuint program, const std::string &blockName) {
    m_program = program;
    glUseProgram(m_program);

    // Find the uniform buffer index for "Uniforms", and determine the block’s sizes
    m_uboIndex = glGetUniformBlockIndex(m_program, blockName.c_str());

    if (m_uboIndex == GL_INVALID_INDEX) {
        LOGE << "UniformBlock Error: " << m_uboIndex << ", couldn´t get the index of the requested Uniform Block";
    } else {
        glGetActiveUniformBlockiv(m_program, m_uboIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &m_uboSize);
        m_buffer.resize(m_uboSize);
        m_inited = true;
    }
}

void UniformBlock::addVarName(const std::string& name, void *inVal, GLenum type) {
    auto it =
        std::ranges::find_if(m_valPairs, [name](const ubBlockVar &ub) { return ub.name == name; });
    if (it != m_valPairs.end()) {
        return;
    }

    m_valPairs.emplace_back(ubBlockVar{name, type});

    switch (type) {
        case GL_INT:
            m_valPairs.back().iVal = static_cast<int *>(inVal);
            break;
        case GL_FLOAT:
            m_valPairs.back().fVal = static_cast<float *>(inVal);
            break;
        case GL_UNSIGNED_INT:
            m_valPairs.back().uVal = static_cast<uint32_t *>(inVal);
            break;
        case GL_BOOL:
            m_valPairs.back().bVal = static_cast<bool *>(inVal);
            break;
        default:
            m_valPairs.back().vVal = inVal;
            break;
    }
}

void UniformBlock::changeVarName(const std::string &name, void *inVal, GLenum type) {
    auto item =
        std::ranges::find_if(m_valPairs, [name](const ubBlockVar &it) { return it.name == name; });

    if (item != m_valPairs.end()) {
        item->name   = name;
        item->inType = type;

        switch (type) {
            case GL_INT:
                item->iVal = static_cast<int *>(inVal);
                break;
            case GL_FLOAT:
                item->fVal = static_cast<float *>(inVal);
                break;
            case GL_UNSIGNED_INT:
                item->uVal = static_cast<uint32_t *>(inVal);
                break;
            case GL_BOOL:
                item->bVal = static_cast<bool *>(inVal);
                break;
            default:
                item->vVal = inVal;
                break;
        }
    } else
        LOGE << "UniformBlock::changeVarName Error, name not found ";
}

ubBlockVar* UniformBlock::getVar(const std::string &name) {
    auto it = std::ranges::find_if(m_valPairs,
                                   [name](const ubBlockVar &ub) { return ub.name == name; });
    return (it != m_valPairs.end() ? &(*it) : nullptr);
}

void UniformBlock::update() {
    if (!m_buffer.empty() && !m_valPairs.empty() && m_program != 0) {
        // Query the necessary attributes to determine where in the buffer we should write the values
        m_numUniforms = static_cast<int>(m_valPairs.size());
        int i         = 0;

        if (m_names.size() != m_numUniforms) {
            m_names.resize(m_numUniforms);
            for (auto &it : m_valPairs) {
                m_names[i++] = it.name.c_str();
            }

            m_indices.resize(m_numUniforms);
            m_size.resize(m_numUniforms);
            m_offset.resize(m_numUniforms);
            m_type.resize(m_numUniforms);
        }

        glGetUniformIndices(m_program, m_numUniforms, &m_names[0], &m_indices[0]);
        glGetActiveUniformsiv(m_program, m_numUniforms, &m_indices[0], GL_UNIFORM_OFFSET, &m_offset[0]);
        glGetActiveUniformsiv(m_program, m_numUniforms, &m_indices[0], GL_UNIFORM_SIZE, &m_size[0]);
        glGetActiveUniformsiv(m_program, m_numUniforms, &m_indices[0], GL_UNIFORM_TYPE, &m_type[0]);

        // Copy the uniform values into the buffer
        i = 0;
        for (auto &it : m_valPairs) {
            void* src = it.vVal;
            switch (m_valPairs[i].inType) {
                case GL_INT:
                    src = it.iVal;
                    break;
                case GL_FLOAT:
                    src = it.fVal;
                    break;
                case GL_UNSIGNED_INT:
                    src = it.uVal;
                    break;
                case GL_BOOL:
                    src = it.bVal;
                    break;
                default:
                    break;
            }
            memcpy(&m_buffer[0] + m_offset[i], src, m_size[i] * TypeSize(m_type[i]));
            ++i;
        }

        // Create the uniform buffer object, initialize its storage, and associated it with the shader program
        if (m_ubo == 0) {
            glGenBuffers(1, &m_ubo);
        }
        glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
        glBufferData(GL_UNIFORM_BUFFER, m_uboSize, &m_buffer[0], GL_STATIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, m_uboIndex, m_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

size_t UniformBlock::TypeSize(GLenum type) {
    static std::unordered_map<GLenum, size_t> typeSizes = {
        { GL_FLOAT, sizeof(GLfloat) },
        { GL_FLOAT_VEC2, sizeof(GLfloat) * 2 },
        { GL_FLOAT_VEC3, sizeof(GLfloat) * 3 },
        { GL_FLOAT_VEC4, sizeof(GLfloat) * 4 },
        { GL_INT, sizeof(GLint) },
        { GL_INT_VEC2, sizeof(GLint) * 2 },
        { GL_INT_VEC3, sizeof(GLint) * 3 },
        { GL_INT_VEC4, sizeof(GLint) * 4 },
        { GL_UNSIGNED_INT, sizeof(GLuint) },
        { GL_UNSIGNED_INT_VEC2, sizeof(GLuint) * 2 },
        { GL_UNSIGNED_INT_VEC3, sizeof(GLuint) * 3 },
        { GL_UNSIGNED_INT_VEC4, sizeof(GLuint) * 4 },
        { GL_BOOL, sizeof(GLboolean) },
        { GL_BOOL_VEC2, sizeof(GLboolean) * 2 },
        { GL_BOOL_VEC3, sizeof(GLboolean) * 3 },
        { GL_BOOL_VEC4, sizeof(GLboolean) * 4 },
        { GL_FLOAT_MAT2, sizeof(GLfloat) * 4 },
        { GL_FLOAT_MAT2x3, sizeof(GLfloat) * 6 },
        { GL_FLOAT_MAT2x4, sizeof(GLfloat) * 8 },
        { GL_FLOAT_MAT3, sizeof(GLfloat) * 9 },
        { GL_FLOAT_MAT3x2, sizeof(GLfloat) * 6 },
        { GL_FLOAT_MAT3x4, sizeof(GLfloat) * 12 },
        { GL_FLOAT_MAT4, sizeof(GLfloat) * 16 },
        { GL_FLOAT_MAT4x2, sizeof(GLfloat) * 8 },
        { GL_FLOAT_MAT4x3, sizeof(GLfloat) * 12 },
    };
    return typeSizes[type];
}

void UniformBlock::removeGLResources() {
    if (m_ubo) {
        glDeleteBuffers(1, &m_ubo);
    }
    m_inited = false;
}

UniformBlock::~UniformBlock() {
    removeGLResources();
}

}  // namespace ara
