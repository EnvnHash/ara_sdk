//
//  GLSLOpticalFlowDepth.h
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <glsg_common/glsg_common.h>

namespace ara {

class GLBase;
class Shaders;
class Quad;
class UniformBlock;
class ShaderCollector;
class PingPongFbo;

class GLSLOpticalFlowDepth {
public:
    GLSLOpticalFlowDepth(GLBase* glbase, int width, int height);

    void update();
    void initShaders();

    void setMedian(float median) { median = median; }
    void setBright(float val) { m_bright = val; }
    void setMaxDist(float val) { m_maxDist = val; }
    void setDiffAmp(float val) { m_diffAmp = val; }
    GLuint getLastTexId() { return m_srcId; }

    GLuint getResTexId();
    GLuint getDiffTexId();

    void setCurTexId(GLuint id) {
        m_lastSrcId = m_srcId;
        m_srcId     = id;
    }
    void setLastTexId(GLuint id) { m_lastSrcId = id; }

private:
    ShaderCollector* m_shCol      = nullptr;
    Shaders*         m_flowShader = nullptr;
    Shaders*         m_texShader  = nullptr;
    GLBase*          m_glbase   = nullptr;

    std::unique_ptr<Quad>            m_quad;
    std::unique_ptr<PingPongFbo>     m_texture;

    int m_width  = 0;
    int m_height = 0;

    GLuint m_srcId     = 0;
    GLuint m_lastSrcId = 0;

    float m_lambda  = 0.f;
    float m_median  = 0.f;
    float m_bright  = 0.f;
    float m_maxDist = 0.f;
    float m_diffAmp = 0.f;
};
}  // namespace ara
