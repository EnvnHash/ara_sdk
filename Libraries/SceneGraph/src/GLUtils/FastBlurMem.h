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
class FastBlurMem {
public:
    enum blurKernelSize { KERNEL_3, KERNEL_5, NUM_BLUR_KERNELS };

    FastBlurMem(GLBase* glbase, float _alpha, int _blurW, int _blurH, GLenum _target = GL_TEXTURE_2D,
                GLenum _intFormat = GL_RGBA8, uint _nrLayers = 1, bool _rot180 = false,
                blurKernelSize _kSize = KERNEL_3, bool singleFbo = false);
    ~FastBlurMem();
    void proc(GLint texIn);
    void initFbo();
    void initShader();

    GLint getResult() { return pp ? pp->getSrcTexId() : 0; }
    GLint getLastResult() { return pp ? pp->getDstTexId() : 0; }
    void  setAlpha(float _alpha) { alpha = _alpha; }
    void  setOffsScale(float _offsScale) { offsScale = _offsScale; }
    void  setPVM(GLfloat* _pvm_ptr) { pvm_ptr = _pvm_ptr; }
    void  setBright(float _bright) { bright = _bright; }

private:
    std::unique_ptr<Quad> fboQuad;
    GLBase*               m_glbase = nullptr;
    Shaders*              linearV  = nullptr;
    Shaders*              linearH  = nullptr;

    FBO*         firstPassFbo = nullptr;
    PingPongFbo* pp           = nullptr;

    int blurW;
    int blurH;

    uint actKernelSize;
    uint nrLayers;

    std::vector<GLfloat> blurOffs;
    std::vector<GLfloat> blurOffsScale;
    GLfloat*             pvm_ptr = 0;

    GLenum target;
    GLenum intFormat;

    float fWidth;
    float fHeight;
    float alpha;
    float bright      = 1.f;
    float offsScale   = 1.f;
    float weightScale = 1.f;

    bool updateProc = false;
    bool rot180     = false;
    bool singleFbo  = false;

    blurKernelSize kSize;
};
}  // namespace ara
