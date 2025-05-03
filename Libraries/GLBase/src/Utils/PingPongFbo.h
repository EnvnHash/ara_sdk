//
//  PingPongFbo.h
//
//  Created by Sven Hahne on 4/5/15.
//

#pragma once

#include "glb_common/glb_common.h"

namespace ara {

class FBO;
class GLBase;

class PingPongFbo {
public:
    explicit PingPongFbo(const FboInitParams&);
    virtual ~PingPongFbo() = default;

    void swap();
    void clear(float alpha = 1.f) const;
    void clearWhite() const;
    void clearAlpha(float alpha, float col) const;

    FBO* operator[](int n) const { return m_fbos[n].get(); }

    [[nodiscard]] GLuint getWidth() const;
    [[nodiscard]] GLuint getHeight() const;
    [[nodiscard]] GLuint getDepth() const;
    [[nodiscard]] GLuint getBitCount() const;
    [[nodiscard]] GLenum getTarget() const;

    void setMinFilter(GLenum type) const;
    void setMagFilter(GLenum type) const;

    [[nodiscard]] GLuint getSrcTexId() const;
    [[nodiscard]] GLuint getDstTexId() const;
    [[nodiscard]] GLuint getSrcTexId(int index) const;
    [[nodiscard]] GLuint getDstTexId(int index) const;

    FBO *m_src = nullptr;  // Source       ->  Ping
    FBO *m_dst = nullptr;  // Destination  ->  Pong

private:
    std::array<std::unique_ptr<FBO>, 2> m_fbos;
    int                                 m_flag   = 0;        // Integer for making a quick swap
    bool                                m_inited = false;
};
}  // namespace ara
