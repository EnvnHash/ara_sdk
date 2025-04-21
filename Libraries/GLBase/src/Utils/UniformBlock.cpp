//
//  UniformBlock.cpp
//
//  Created by Sven Hahne on 14.08.17
//
//  should be used after the shader is linked
//  uniform block has to be used in shader, otherwise the compiler optimizes it, means kill it

#include "Utils/UniformBlock.h"

namespace ara {

UniformBlock::UniformBlock(GLuint program, const std::string &blockName) { init(program, blockName); }

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

void UniformBlock::addVarName(std::string name, void *inVal, GLenum type) {
    auto it =
        std::find_if(m_valPairs.begin(), m_valPairs.end(), [name](const ubBlockVar &ub) { return ub.name == name; });
    if (it != m_valPairs.end()) {
        return;
    }

    m_valPairs.emplace_back(ubBlockVar{std::move(name), type});

    switch (type) {
        case GL_INT: m_valPairs.back().iVal = static_cast<int *>(inVal); break;
        case GL_FLOAT: m_valPairs.back().fVal = static_cast<float *>(inVal); break;
        case GL_UNSIGNED_INT: m_valPairs.back().uVal = static_cast<uint32_t *>(inVal); break;
        case GL_BOOL: m_valPairs.back().bVal = static_cast<bool *>(inVal); break;
        default: m_valPairs.back().vVal = inVal; break;
    }
}

void UniformBlock::changeVarName(const std::string &name, void *inVal, GLenum type) {
    auto item =
        std::find_if(m_valPairs.begin(), m_valPairs.end(), [name](const ubBlockVar &it) { return it.name == name; });

    if (item != m_valPairs.end()) {
        item->name   = name;
        item->inType = type;

        switch (type) {
            case GL_INT: item->iVal = static_cast<int *>(inVal); break;
            case GL_FLOAT: item->fVal = static_cast<float *>(inVal); break;
            case GL_UNSIGNED_INT: item->uVal = static_cast<uint32_t *>(inVal); break;
            case GL_BOOL: item->bVal = static_cast<bool *>(inVal); break;
            default: item->vVal = inVal; break;
        }
    } else
        LOGE << "UniformBlock::changeVarName Error, name not found ";
}

ubBlockVar* UniformBlock::getVar(const std::string &name) {
    auto it = std::find_if(m_valPairs.begin(), m_valPairs.end(),
                          [name](const ubBlockVar &ub) { return ub.name == name; });
    return (it != m_valPairs.end() ? &(*it) : nullptr);
}

void UniformBlock::update() {
    if (!m_buffer.empty() && !m_valPairs.empty() && m_program != 0) {
        // Query the necessary attributes to determine
        // where in the buffer we should write the values
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
            auto ptr = &m_buffer[0] + m_offset[i];
            auto dataSize = m_size[i];
            switch (m_valPairs[i].inType) {
                case GL_INT:
                    std::copy_n(it.iVal, dataSize, ptr);
                    break;
                case GL_FLOAT:
                    std::copy_n(it.fVal, dataSize, ptr);
                    break;
                case GL_UNSIGNED_INT:
                    std::copy_n(it.uVal, dataSize, ptr);
                    break;
                case GL_BOOL:
                    std::copy_n(it.bVal, dataSize, ptr);
                    break;
                default:
                    std::copy_n(static_cast<GLubyte*>(it.vVal), dataSize, ptr);
                    break;
            }
            ++i;
        }

        // Create the uniform buffer object, initialize its storage, and
        // associated it with the shader program
        if (m_ubo == 0) glGenBuffers(1, &m_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
        glBufferData(GL_UNIFORM_BUFFER, m_uboSize, &m_buffer[0], GL_STATIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, m_uboIndex, m_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}

size_t UniformBlock::TypeSize(GLenum type) {
    switch (type) {
        case GL_FLOAT: return sizeof(GLfloat);
        case GL_FLOAT_VEC2: return sizeof(GLfloat) * 2;
        case GL_FLOAT_VEC3: return sizeof(GLfloat) * 3;
        case GL_FLOAT_VEC4: return sizeof(GLfloat) * 4;
        case GL_INT: return sizeof(GLint);
        case GL_INT_VEC2: return sizeof(GLint) * 2;
        case GL_INT_VEC3: return sizeof(GLint) * 3;
        case GL_INT_VEC4: return sizeof(GLint) * 4;
        case GL_UNSIGNED_INT: return sizeof(GLuint);
        case GL_UNSIGNED_INT_VEC2: return sizeof(GLuint) * 2;
        case GL_UNSIGNED_INT_VEC3: return sizeof(GLuint) * 3;
        case GL_UNSIGNED_INT_VEC4: return sizeof(GLuint) * 4;
        case GL_BOOL: return sizeof(GLboolean);
        case GL_BOOL_VEC2: return sizeof(GLboolean) * 2;
        case GL_BOOL_VEC3: return sizeof(GLboolean) * 3;
        case GL_BOOL_VEC4: return sizeof(GLboolean) * 4;
        case GL_FLOAT_MAT2: return sizeof(GLfloat) * 4;
        case GL_FLOAT_MAT2x3: return sizeof(GLfloat) * 6;
        case GL_FLOAT_MAT2x4: return sizeof(GLfloat) * 8;
        case GL_FLOAT_MAT3: return sizeof(GLfloat) * 9;
        case GL_FLOAT_MAT3x2: return sizeof(GLfloat) * 6;
        case GL_FLOAT_MAT3x4: return sizeof(GLfloat) * 12;
        case GL_FLOAT_MAT4: return sizeof(GLfloat) * 16;
        case GL_FLOAT_MAT4x2: return sizeof(GLfloat) * 8;
        case GL_FLOAT_MAT4x3: return sizeof(GLfloat) * 12;

        default: 
            LOGE << "UniformBlock ERROR: Unknown type: " << type; 
            return 0;
    }
}

UniformBlock::~UniformBlock() {
    if (m_ubo) {
        glDeleteBuffers(1, &m_ubo);
    }
}

}  // namespace ara
