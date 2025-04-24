//
//  PingPongFbo.cpp
//  Tav_App
//
//  Created by Sven Hahne on 4/5/15.
//  Copyright (c) 2015 Sven Hahne. All rights reserved.
//

#include "Utils/PingPongFbo.h"
#include "Utils/FBO.h"

using namespace std;

namespace ara {

PingPongFbo::PingPongFbo(const FboInitParams& ip)
    : flag(0) {
    for (int i = 0; i < 2; i++) {
        fbos[i] = make_unique<FBO>(ip);
        fbos[i]->bind();
        fbos[i]->clearAlpha(1.f);
        fbos[i]->unbind();
    }

    src    = fbos[0].get();
    dst    = fbos[1].get();
    inited = true;
}

void PingPongFbo::swap() {
    flag = (flag + 1) % 2;
    src  = fbos[flag % 2].get();
    dst  = fbos[(flag + 1) % 2].get();
}

void PingPongFbo::clear(float _alpha) {
    for (int i = 0; i < 2; i++) {
        fbos[i]->bind();
        fbos[i]->clearAlpha(_alpha);
        fbos[i]->unbind();
    }
}

void PingPongFbo::clearWhite() {
    for (int i = 0; i < 2; i++) {
        fbos[i]->bind();
        fbos[i]->clearWhite();
        fbos[i]->unbind();
    }
}

void PingPongFbo::clearAlpha(float _alpha, float _col) {
    for (int i = 0; i < 2; i++) {
        fbos[i]->clearToColor(_col, _col, _col, _alpha);
    }
}

void PingPongFbo::setMinFilter(GLenum _type) {
    for (int i = 0; i < 2; i++) {
        fbos[i]->setMinFilter(_type);
    }
}

void PingPongFbo::setMagFilter(GLenum _type) {
    for (int i = 0; i < 2; i++) {
        fbos[i]->setMagFilter(_type);
    }
}

GLuint PingPongFbo::getSrcTexId() const {
    return src ? src->getColorImg() : 0;
}

GLuint PingPongFbo::getDstTexId() const {
    return dst ? dst->getColorImg() : 0;
}

GLuint PingPongFbo::getSrcTexId(int _index) const {
    return src ? src->getColorImg(_index) : 0;
}

GLuint PingPongFbo::getDstTexId(int _index) const {
    return dst ? dst->getColorImg(_index) : 0;
}

GLuint PingPongFbo::getWidth() const {
    return src ? src->getWidth() : 0;
}

GLuint PingPongFbo::getHeight() const {
    return src ? src->getHeight() : 0;
}

GLuint PingPongFbo::getDepth() const {
    return src ? src->getDepth() : 0;
}

GLuint PingPongFbo::getBitCount() const {
    return src ? ara::getBitCount(src->getType()) : 0;
}

GLenum PingPongFbo::getTarget() const {
    return src ? src->getTarget() : 0;
}

}  // namespace ara
