//
//  ShadowMap.h
//
//
//  Created by Sven Hahne on 18.08.14.
//

#pragma once

#include <Shaders/Shaders.h>
#include <Utils/Camera.h>
#include <Utils/FBO.h>

#include "SceneNodes/SceneNode.h"

namespace ara {
class CameraSet;

class ShadowMap {
public:
    ShadowMap(GLBase*);
    virtual ~ShadowMap();

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

    virtual void setScreenSize(uint _width, uint _height);

    GLBase*              s_glbase       = nullptr;
    Shaders*             s_shadowShader = nullptr;
    CameraSet*           s_cs           = nullptr;
    std::unique_ptr<FBO> s_fbo;

    int   s_scrWidth        = 0;
    int   s_scrHeight       = 0;
    float s_shadow_map_coef = 0.f;
};

}  // namespace ara
