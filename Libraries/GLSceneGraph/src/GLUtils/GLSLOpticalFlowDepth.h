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
    virtual ~GLSLOpticalFlowDepth();
    void update();
    void initShaders();

    void setMedian(float median) { median = median; }
    void setBright(float val) { bright = val; }
    void setMaxDist(float val) { maxDist = val; }
    void setDiffAmp(float val) { diffAmp = val; }
    GLuint getLastTexId() { return srcId; }

    GLuint getResTexId();
    GLuint getDiffTexId();

    void setCurTexId(GLuint id) {
        lastSrcId = srcId;
        srcId     = id;
    }
    void setLastTexId(GLuint id) { lastSrcId = id; }

private:
    ShaderCollector* shCol      = nullptr;
    Shaders*         flowShader = nullptr;
    Shaders*         texShader  = nullptr;
    Quad*            quad       = nullptr;
    UniformBlock*    uBlock     = nullptr;
    PingPongFbo*     texture    = nullptr;
    GLBase*          m_glbase   = nullptr;

    int width  = 0;
    int height = 0;

    GLuint srcId     = 0;
    GLuint lastSrcId = 0;

    float lambda  = 0.f;
    float median  = 0.f;
    float bright  = 0.f;
    float maxDist = 0.f;
    float diffAmp = 0.f;
};
}  // namespace ara
