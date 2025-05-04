/*
 * ShaderBuffer.h
 *
 *  Created on: 16.09.2016
 *      Author: sven
 */

//    Simple class to represent OpenGL shader storage buffer object

#pragma once

#include <GlbCommon/GlbCommon.h>

namespace ara {

template <class T>
class ShaderBuffer {
public:
    ShaderBuffer() = default;

    explicit ShaderBuffer(size_t size) : m_size(size) {
        glGenBuffers(1, &m_buffer);
        bind();
        glBufferData(target, m_size * sizeof(T), nullptr, GL_STATIC_DRAW);
        unbind();
    }

    ~ShaderBuffer() {
        glDeleteBuffers(1, &m_buffer);
    }

    void bind() const {
        glBindBuffer(target, m_buffer);
    }

    static void unbind() {
        glBindBuffer(target, 0);
    }

    T *map(GLbitfield access = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT) {
        bind();
        return static_cast<T *>(glMapBufferRange(target, 0, m_size * sizeof(T), access));
    }

    void unmap() const {
        bind();
        glUnmapBuffer(target);
    }

    [[nodiscard]] GLuint getBuffer() const { return m_buffer; }
    [[nodiscard]] size_t getSize() const { return m_size; }

    void resize(size_t size) {
        // in opengl it's not possible to resize buffers, so create a new one
        // copy the old contents, destroy the old one, and exchange the ids
        GLuint new_buffer;
        glGenBuffers(1, &new_buffer);
        glBindBuffer(target, new_buffer);
        glBufferData(target, size * sizeof(T), nullptr,
                     GL_STATIC_DRAW);  // init buffer
        glBindBuffer(GL_COPY_READ_BUFFER, m_buffer);
        glBindBuffer(GL_COPY_WRITE_BUFFER, new_buffer);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0,
                            std::min<size_t>(size, m_size * sizeof(T)));
        glBindBuffer(GL_COPY_READ_BUFFER, 0);
        glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
        glDeleteBuffers(1, &m_buffer);
        m_buffer = new_buffer;
        m_size   = size;
    }

    void dump() {
        T *data = map(GL_MAP_READ_BIT);
        for (size_t i = 0; i < m_size; i++) {
            LOG << i << ": " << glm::to_string(data[i]);
        }
        unmap();
    }

private:
    static constexpr GLenum target   = GL_SHADER_STORAGE_BUFFER;
    size_t              m_size   = 0;
    GLuint              m_buffer = 0;
};

}  // namespace ara
