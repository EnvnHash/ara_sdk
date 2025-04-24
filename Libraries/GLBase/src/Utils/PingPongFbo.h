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
    void clear(float _alpha = 1.f);
    void clearWhite();
    void clearAlpha(float _alpha, float _col);

    FBO* operator[](int n) { return fbos[n].get(); }

    [[nodiscard]] GLuint getWidth() const;
    [[nodiscard]] GLuint getHeight() const;
    [[nodiscard]] GLuint getDepth() const;
    [[nodiscard]] GLuint getBitCount() const;
    [[nodiscard]] GLenum getTarget() const;

    void setMinFilter(GLenum type);
    void setMagFilter(GLenum type);

    [[nodiscard]] GLuint getSrcTexId() const;
    [[nodiscard]] GLuint getDstTexId() const;
    [[nodiscard]] GLuint getSrcTexId(int index) const;
    [[nodiscard]] GLuint getDstTexId(int index) const;

    FBO *src = nullptr;  // Source       ->  Ping
    FBO *dst = nullptr;  // Destination  ->  Pong

private:
    std::array<std::unique_ptr<FBO>, 2> fbos;
    int                                 flag   = 0;        // Integer for making a quick swap
    bool                                inited = false;
};
}  // namespace ara
