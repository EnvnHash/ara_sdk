//
//  FastBlurMem.h
//
//  Created by Sven Hahne on 23/5/15.
//

#pragma once

#include <Shaders/ShaderCollector.h>
#include <Shaders/Shaders.h>
#include <Utils/FBO.h>
#include <Utils/PingPongFbo.h>

#include "GLUtils/sceneData.h"
#include "glsg_common/glsg_common.h"

namespace ara {

enum blurKernelSize { KERNEL_3, KERNEL_5, NUM_BLUR_KERNELS };

struct FastBlurMemParams {
    GLBase* glbase = nullptr;
    float alpha;
    glm::ivec2 blurSize{};
    GLenum target = GL_TEXTURE_2D;
    GLenum intFormat = GL_RGBA8;
    uint nrLayers = 1;
    bool rot180 = false;
    blurKernelSize kSize = KERNEL_3;
    bool singleFbo = false;
};

class FastBlurMem {
public:

    FastBlurMem(const FastBlurMemParams& params);

    void proc(GLint texIn);
    void initFbo();
    void initShader();

    GLint getResult() const { return m_pp ? m_pp->getSrcTexId() : 0; }
    GLint getLastResult() const { return m_pp ? m_pp->getDstTexId() : 0; }
    void  setAlpha(float alpha) { m_alpha = alpha; }
    void  setOffsScale(float offsScale) { m_offsScale = offsScale; }
    void  setBright(float bright) { m_bright = bright; }

private:
    std::unique_ptr<Quad> m_fboQuad;
    GLBase*               m_glbase = nullptr;
    Shaders*              m_linearV  = nullptr;
    Shaders*              m_linearH  = nullptr;

    std::unique_ptr<FBO>         m_firstPassFbo;
    std::unique_ptr<PingPongFbo> m_pp;

    int m_blurW = 0;
    int m_blurH = 0;

    uint    m_actKernelSize = 0;
    int     m_nrLayers = 0;

    std::vector<GLfloat> m_blurOffs;
    std::vector<GLfloat> m_blurOffsScale;

    GLenum m_target{};
    GLenum m_intFormat{};

    float m_fWidth        = 0.f;
    float m_fHeight       = 0.f;
    float m_alpha         = 0.f;
    float m_bright        = 1.f;
    float m_offsScale     = 1.f;
    float m_weightScale   = 1.f;

    bool m_rot180     = false;
    bool m_singleFbo  = false;

    blurKernelSize m_kSize{};
};
}  // namespace ara
