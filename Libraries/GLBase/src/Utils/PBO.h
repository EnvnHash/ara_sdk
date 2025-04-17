//
// Created by user on 13.05.2021.
//

#pragma once

#include "glb_common/glb_common.h"

namespace ara {

class PBO {
public:
    PBO() = default;
    PBO(uint32_t width, uint32_t height, GLenum format);

    void init();
    void resizePBOArray(int nrBufs);
    void upload(GLuint textureId, void *dataPtr);
    void resize(uint32_t width, uint32_t height, GLenum format);

    void setSize(uint32_t width, uint32_t height) {
        m_width  = width;
        m_height = height;
        getDataSize();
    }

    void setFormat(GLenum internalFormat) {
        m_type     = getPixelType(internalFormat);
        m_format   = getExtType(internalFormat);
        m_bitCount = getBitCount(internalFormat);
        getDataSize();
    }

    void               getDataSize() { m_dataSize = m_width * m_height * m_bitCount / 8; }
    [[nodiscard]] bool isInited() const { return m_inited; }

private:
    bool     m_inited    = false;
    uint32_t m_width     = 0;
    uint32_t m_height    = 0;
    uint32_t m_nrPboBufs = 3;
    uint32_t m_uplIdx    = 0;
    uint32_t m_nextIndex = 0;
    uint32_t m_bitCount  = 0;
    uint32_t m_dataSize  = 0;
#ifdef ARA_USE_GLES31
    GLenum m_format = GL_RGBA;
#else
    GLenum m_format = GL_BGRA;
#endif
    GLenum m_type = GL_UNSIGNED_BYTE;

    std::vector<GLuint> m_pbos;
};

}  // namespace ara
