//
// Created by user on 13.05.2021.
//

#include "PBO.h"

namespace ara {

PBO::PBO(uint32_t width, uint32_t height, GLenum internalFormat) : m_width(width), m_height(height), m_nrPboBufs(3) {
    m_type     = getPixelType(internalFormat);
    m_format   = getExtType(internalFormat);
    m_bitCount = getBitCount(internalFormat);
    getDataSize();
    init();
}

void PBO::resizePBOArray(int nrBufs) {
    if (!m_pbos.empty()) glDeleteBuffers(m_nrPboBufs, &m_pbos[0]);

    m_nrPboBufs = nrBufs;
    m_inited    = false;
    init();
}

void PBO::resize(uint32_t width, uint32_t height, GLenum internalFormat) {
    m_width    = width;
    m_height   = height;
    m_bitCount = getBitCount(internalFormat);
    m_type     = getPixelType(internalFormat);
    m_format   = getExtType(internalFormat);
    getDataSize();
}

void PBO::init() {
    if (!m_inited) {
        m_pbos.resize(m_nrPboBufs, 0);

        glGenBuffers(m_nrPboBufs, &m_pbos[0]);
        for (uint32_t i = 0; i < m_nrPboBufs; i++) {
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbos[i]);
            glBufferData(GL_PIXEL_UNPACK_BUFFER, m_dataSize, nullptr, GL_STREAM_DRAW);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        }
        m_inited = true;
    }
}

void PBO::upload(GLuint textureId, void *dataPtr) {
    // "index" is used to copy pixels from a PBO to a texture object
    // "nextIndex" is used to update pixels in the other PBO
    m_uplIdx    = (m_uplIdx + 1) % m_nrPboBufs;
    m_nextIndex = (m_uplIdx + 1) % m_nrPboBufs;

    // bind the texture and PBO
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbos[m_uplIdx]);

    // copy pixels from PBO to texture object
    // glTexSubImage2D: If a non-zero named buffer object is bound to the
    // GL_PIXEL_UNPACK_BUFFER target (see glBindBuffer) while a texture image is
    // specified, pixels is treated as a byte offset into the buffer object's
    // data store.
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_width, m_height, m_format, m_type, nullptr);

    //-----  pbo to texture copy operation is done at this point

    // bind PBO to upload from client side memory
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, m_pbos[m_nextIndex]);

    glBufferData(GL_PIXEL_UNPACK_BUFFER, m_dataSize, nullptr,
                 GL_STREAM_DRAW);  // discard the buffer, avoid waiting

    // map the buffer object into client's memory
    auto *ptr = (GLubyte *)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, m_dataSize, GL_MAP_WRITE_BIT);
    if (ptr) {
        // update data directly on the mapped buffer
        memcpy(ptr, dataPtr, m_dataSize);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);  // release the mapped buffer
    }

    // the client side memory data pointer is not needed anymore at this point

    // it is good idea to release PBOs with ID 0 after use.
    // Once bound with 0, all pixel operations are back to normal ways.
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

}  // namespace ara
