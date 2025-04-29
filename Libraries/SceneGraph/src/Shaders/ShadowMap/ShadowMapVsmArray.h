//
//  SHShadow.h
//
//  Created by Sven Hahne on 17.07.14.
//

#pragma once

#include "GLUtils/FastBlurMem.h"
#include "Shaders/ShadowMap/ShadowMap.h"

namespace ara {
class ShadowMapVsmArray : public ShadowMap {
public:
    ShadowMapVsmArray(CameraSet* _cs, int _scrWidth, int _scrHeight, uint _initNrLights);
    ~ShadowMapVsmArray() = default;

    void  rebuildShader(uint _nrLights);
    void  rebuildFbo(uint _nrLights);
    static void  setShadowTexPar(GLenum type);
    void  begin();
    void  end();
    void  clear();
    void  setNrLights(uint _nrLights);
    void  setScreenSize(uint _width, uint _height);
    void  bindDepthTexViews(GLuint baseTexUnit, uint nrTexs, uint texOffs);
    GLint getTex() { return fboBlur ? fboBlur->getLastResult() : 0; }
    void  blur();

private:
    GLint                        max_shader_invoc;
    float                        blurAlpha;
    std::vector<GLuint>          depthTexViews;
    ShaderCollector*             shCol;
    uint                         nrLights;
    std::unique_ptr<FastBlurMem> fboBlur;
};
}  // namespace ara
