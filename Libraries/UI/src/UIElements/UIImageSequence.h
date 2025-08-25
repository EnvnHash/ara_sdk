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
    void setLoopSwitch(int32_t nrOfLoopsBeforeSwitch, int32_t newStartFrame) { m_imgSeq.setLoopSwitch(nrOfLoopsBeforeSwitch, newStartFrame); }

protected:
    ImageSequence       m_imgSeq;
};

}
