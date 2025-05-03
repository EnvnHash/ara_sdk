//
//  ShadowMapStd.cpp
//
//  Created by Sven Hahne on 12.07.14.
//
//  Standard directional light
//

#include "Shaders/ShadowMap/ShadowMap.h"
#include <Shaders/Shaders.h>
#include <Utils/FBO.h>

#include "SceneNodes/SceneNode.h"

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
    return s_fbo ? s_fbo->getDepthImg() : 0;
}

GLuint ShadowMap::getColorImg() {
    return s_fbo ? s_fbo->getColorImg() : 0;
}

FBO* ShadowMap::getFbo() {
    return s_fbo ? s_fbo.get() : nullptr;
}

int ShadowMap::getWidth() {
    return s_scrWidth;
}

int ShadowMap::getHeight() {
    return s_scrHeight;
}

Shaders* ShadowMap::getShader() {
    return s_shadowShader;
}

void ShadowMap::setScreenSize(uint width, uint height) {
    s_scrWidth  = width;
    s_scrHeight = height;
    if (s_fbo) {
        s_fbo->resize(width, height);
    }
}

}  // namespace ara
