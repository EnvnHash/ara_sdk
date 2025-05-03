//
//  ShadowMap.h
//
//
//  Created by Sven Hahne on 18.08.14.
//

#pragma once

#include <glsg_common/glsg_common.h>

namespace ara {
class CameraSet;
class FBO;
class Shaders;

class ShadowMap {
public:
    explicit ShadowMap(GLBase*);
    virtual ~ShadowMap() = default;

    virtual void begin() = 0;
    virtual void end()   = 0;
    virtual void clear();

    virtual void bindShadowMap(GLuint texUnit);

    virtual Shaders* getShader();
    virtual GLuint   getDepthImg();
    virtual GLuint   getColorImg();
    virtual FBO*     getFbo();
    virtual int      getWidth();
    virtual int      getHeight();

    virtual void setScreenSize(uint width, uint height);

    GLBase*              s_glbase       = nullptr;
    Shaders*             s_shadowShader = nullptr;
    CameraSet*           s_cs           = nullptr;
    std::unique_ptr<FBO> s_fbo;

    int   s_scrWidth        = 0;
    int   s_scrHeight       = 0;
    float s_shadow_map_coef = 0.f;
};

}  // namespace ara
