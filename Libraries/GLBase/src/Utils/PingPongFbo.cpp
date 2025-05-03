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

PingPongFbo::PingPongFbo(const FboInitParams& ip) {
    for (int i = 0; i < 2; i++) {
        m_fbos[i] = make_unique<FBO>(ip);
        m_fbos[i]->bind();
        m_fbos[i]->clearAlpha(1.f);
        m_fbos[i]->unbind();
    }

    m_src    = m_fbos[0].get();
    m_dst    = m_fbos[1].get();
    m_inited = true;
}

void PingPongFbo::swap() {
    m_flag = (m_flag + 1) % 2;
    m_src  = m_fbos[m_flag % 2].get();
    m_dst  = m_fbos[(m_flag + 1) % 2].get();
}

void PingPongFbo::clear(float _alpha) const {
    for (int i = 0; i < 2; i++) {
        m_fbos[i]->bind();
        m_fbos[i]->clearAlpha(_alpha);
        m_fbos[i]->unbind();
    }
}

void PingPongFbo::clearWhite() const {
    for (int i = 0; i < 2; i++) {
        m_fbos[i]->bind();
        m_fbos[i]->clearWhite();
        m_fbos[i]->unbind();
    }
}

void PingPongFbo::clearAlpha(float alpha, float col) const {
    for (int i = 0; i < 2; i++) {
        m_fbos[i]->clearToColor(col, col, col, alpha);
    }
}

void PingPongFbo::setMinFilter(GLenum type) const {
    for (int i = 0; i < 2; i++) {
        m_fbos[i]->setMinFilter(type);
    }
}

void PingPongFbo::setMagFilter(GLenum type) const {
    for (int i = 0; i < 2; i++) {
        m_fbos[i]->setMagFilter(type);
    }
}

GLuint PingPongFbo::getSrcTexId() const {
    return m_src ? m_src->getColorImg() : 0;
}

GLuint PingPongFbo::getDstTexId() const {
    return m_dst ? m_dst->getColorImg() : 0;
}

GLuint PingPongFbo::getSrcTexId(int _index) const {
    return m_src ? m_src->getColorImg(_index) : 0;
}

GLuint PingPongFbo::getDstTexId(int _index) const {
    return m_dst ? m_dst->getColorImg(_index) : 0;
}

GLuint PingPongFbo::getWidth() const {
    return m_src ? m_src->getWidth() : 0;
}

GLuint PingPongFbo::getHeight() const {
    return m_src ? m_src->getHeight() : 0;
}

GLuint PingPongFbo::getDepth() const {
    return m_src ? m_src->getDepth() : 0;
}

GLuint PingPongFbo::getBitCount() const {
    return m_src ? ara::getBitCount(m_src->getType()) : 0;
}

GLenum PingPongFbo::getTarget() const {
    return m_src ? m_src->getTarget() : 0;
}

}  // namespace ara
