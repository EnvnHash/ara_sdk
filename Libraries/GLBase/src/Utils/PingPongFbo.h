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
    PingPongFbo(GLBase *glbase, int width, int height, GLenum type, GLenum target, bool depthBuf = false,
                int nrAttachments = 1, int mipMapLevels = 1, int nrSamples = 1, GLenum wrapMode = GL_REPEAT,
                bool layered = false);

    PingPongFbo(GLBase *glbase, int width, int height, int depth, GLenum type, GLenum target, bool depthBuf,
                int nrAttachments, int mipMapLevels, int nrSamples, GLenum wrapMode, bool layered);

    virtual ~PingPongFbo();

    void swap();
    void clear(float _alpha = 1.f);
    void clearWhite();
    void clearAlpha(float _alpha, float _col);

    FBO *operator[](int n) { return FBOs[n]; }

    GLuint getWidth() const;
    GLuint getHeight() const;
    GLuint getDepth() const;
    GLuint getBitCount() const;
    GLenum getTarget() const;

    void setMinFilter(GLenum type);
    void setMagFilter(GLenum type);

    [[nodiscard]] GLuint getSrcTexId() const;
    [[nodiscard]] GLuint getDstTexId() const;
    [[nodiscard]] GLuint getSrcTexId(int index) const;
    [[nodiscard]] GLuint getDstTexId(int index) const;

    FBO *src = nullptr;  // Source       ->  Ping
    FBO *dst = nullptr;  // Destination  ->  Pong

private:
    FBO **FBOs   = nullptr;  // Real addresses of ping/pong FBOs
    int   flag   = 0;        // Integer for making a quick swap
    bool  inited = false;
};
}  // namespace ara
