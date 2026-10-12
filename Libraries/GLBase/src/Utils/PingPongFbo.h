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

class FBO;
class GLBase;
class Texture;

class PingPongFbo {
public:
    PingPongFbo() = default;
    explicit PingPongFbo(const FboInitParams&);
    virtual ~PingPongFbo() = default;

    void swap();
    void bind() const;
    void unbind() const;
    void clear(float alpha = 1.f) const;
    void clearWhite() const;
    void clearAlpha(float alpha, float col) const;
    void saveDstToFile(const std::filesystem::path &filename, size_t attachNr, GLenum intFormat) const;
    void saveSrcToFile(const std::filesystem::path &filename, size_t attachNr, GLenum intFormat) const;

    FBO* operator[](const int n) const { return m_fbos[n].get(); }

    [[nodiscard]] GLuint getWidth() const;
    [[nodiscard]] GLuint getHeight() const;
    [[nodiscard]] GLuint getDepth() const;
    [[nodiscard]] GLuint getBitCount() const;
    [[nodiscard]] GLenum getTarget() const;

    void setMinFilter(GLenum type) const;
    void setMagFilter(GLenum type) const;
    void setGLBase(GLBase *glbase);

    [[nodiscard]] GLuint getSrcTexId() const;
    [[nodiscard]] GLuint getDstTexId() const;
    [[nodiscard]] GLuint getSrcTexId(int index) const;
    [[nodiscard]] GLuint getDstTexId(int index) const;

    FBO *m_src = nullptr;  // Source
    FBO *m_dst = nullptr;  // Destination

private:
    std::array<std::unique_ptr<FBO>, 2> m_fbos;
    GLBase*                             m_glBase = nullptr;
    int                                 m_flag   = 0;        // Integer for making a quick swap
    bool                                m_inited = false;
};
}  // namespace ara
