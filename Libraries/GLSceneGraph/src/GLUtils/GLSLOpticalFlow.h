//
//  GLSLOpticalFlow.h
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <GLBase.h>
#include <GeoPrimitives/Quad.h>
#include <Shaders/ShaderCollector.h>
#include <Utils/PingPongFbo.h>
#include <Utils/Texture.h>

namespace ara {

class GLSLOpticalFlow {
public:
    GLSLOpticalFlow(GLBase* glbase, int width, int height);
    virtual ~GLSLOpticalFlow();

    void initShader(ShaderCollector* shCol);
    void update(GLint tex1, GLint tex2, float fdbk = 0.f);

    void   setMedian(float median) { median = median; }
    void   setBright(float val) { bright = val; }
    void   setPVM(GLfloat* pvm_ptr) { pvm_ptr = pvm_ptr; }
    GLuint getResTexId() { return texture->getSrcTexId(); }

private:
    GLBase*          m_glbase   = nullptr;
    ShaderCollector* shCol      = nullptr;
    Shaders*         flowShader = nullptr;
    Shaders*         texShader  = nullptr;
    Quad*            quad       = nullptr;
    PingPongFbo*     texture    = nullptr;

    int          width    = 0;
    int          height   = 0;
    unsigned int isInited = 0;

    GLuint   srcId     = 0;
    GLuint   lastSrcId = 0;
    GLfloat* pvm_ptr   = 0;
    float    lambda    = 0.f;
    float    median    = 0.f;
    float    bright    = 0.f;
};
}  // namespace ara
