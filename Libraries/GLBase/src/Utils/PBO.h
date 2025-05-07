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

#include "GlbCommon/GlbCommon.h"

namespace ara {

class PBO {
public:
    PBO() = default;
    PBO(uint32_t width, uint32_t height, GLenum format);

    void init();
    void resizePBOArray(int nrBufs);
    void upload(GLuint textureId, const void *dataPtr);
    void resize(uint32_t width, uint32_t height, GLenum format);

    void setSize(uint32_t width, uint32_t height) {
        m_width  = static_cast<GLsizei>(width);
        m_height = static_cast<GLsizei>(height);
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
    GLsizei  m_width     = 0;
    GLsizei  m_height    = 0;
    GLsizei  m_nrPboBufs = 3;
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
