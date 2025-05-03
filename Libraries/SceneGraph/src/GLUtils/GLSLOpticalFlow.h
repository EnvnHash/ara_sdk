//
//  GLSLOpticalFlow.h
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved..
//

#pragma once

#include <glb_common/glb_common.h>

namespace ara {

class GLBase;
class ShaderCollector;
class Shaders;
class Quad;
class PingPongFbo;

class GLSLOpticalFlow {
public:
    GLSLOpticalFlow(GLBase* glbase, int width, int height);

    void initShader(ShaderCollector* shCol);
    void update(GLint tex1, GLint tex2, float fdbk = 0.f);

    void   setMedian(float med) { m_median = med; }
    void   setBright(float val) { m_bright = val; }
    void   setPVM(GLfloat* ptr) { m_pvm_ptr = ptr; }
    [[nodiscard]] GLuint getResTexId() const;

private:
    GLBase*          m_glbase   = nullptr;
    ShaderCollector* m_shCol      = nullptr;
    Shaders*         m_flowShader = nullptr;
    Shaders*         m_texShader  = nullptr;

    std::unique_ptr<Quad>            m_quad;
    std::unique_ptr<PingPongFbo>     m_texture;

    int          m_width    = 0;
    int          m_height   = 0;
    unsigned int m_isInited = 0;

    GLuint   m_srcId     = 0;
    GLuint   m_lastSrcId = 0;
    GLfloat* m_pvm_ptr   = nullptr;
    float    m_lambda    = 0.1f;
    float    m_median    = 3.f;
    float    m_bright    = 4.f;
};
}  // namespace ara
