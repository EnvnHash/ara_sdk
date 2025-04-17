//
//  ShadowMapStd.cpp
//
//  Created by Sven Hahne on 12.07.14.
//
//  Standard directional light
//

#include "Shaders/ShadowMap/ShadowMap.h"

using namespace glm;
using namespace std;

namespace ara {

ShadowMap::ShadowMap(GLBase* glbase) : s_glbase(glbase) {}

void ShadowMap::clear() {
    if (s_fbo) {
        s_fbo->bind();
        s_fbo->clear();
        s_fbo->unbind();
    }
}

void ShadowMap::bindShadowMap(GLuint texUnit) {
    glActiveTexture(GL_TEXTURE0 + texUnit);
    glBindTexture(GL_TEXTURE_2D, s_fbo->getDepthImg());
}

GLuint ShadowMap::getDepthImg() {
    if (s_fbo)
        return s_fbo->getDepthImg();
    else
        return 0;
}

GLuint ShadowMap::getColorImg() {
    if (s_fbo)
        return s_fbo->getColorImg();
    else
        return 0;
}

FBO* ShadowMap::getFbo() {
    if (s_fbo)
        return s_fbo.get();
    else
        return nullptr;
}

int ShadowMap::getWidth() { return s_scrWidth; }

int ShadowMap::getHeight() { return s_scrHeight; }

Shaders* ShadowMap::getShader() { return s_shadowShader; }

void ShadowMap::setScreenSize(uint _width, uint _height) {
    s_scrWidth  = _width;
    s_scrHeight = _height;
    if (s_fbo) s_fbo->resize(_width, _height);
}

ShadowMap::~ShadowMap() {
    if (s_shadowShader) delete s_shadowShader;
}
}  // namespace ara
