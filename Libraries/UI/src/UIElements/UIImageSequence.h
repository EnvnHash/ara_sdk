//
// Created by sven on 25-08-25.
//

#pragma once

#include "UIElements/Image.h"
#include "Utils/ImageSequence.h"

namespace ara {

class UIImageSequence : public Image {
public:
    UIImageSequence();
    void init() override;
    void start() { m_imgSeq.start(); }
    void update();
    void setLoopPoint(int32_t val) { m_imgSeq.setLoopPoint(val); }
    void setFps(int32_t val) { m_imgSeq.setFps(val); }
    void setEndCb(const std::function<void(ImageSequence*, int32_t)>& f) { m_imgSeq.setEndCb(f); }

protected:
    ImageSequence       m_imgSeq;
};

}
