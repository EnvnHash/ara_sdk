//
// Created by sven on 01-04-25.
//

#pragma once

#include <Transitions/UIStack.h>
#include <Transitions/UINodeBlender.h>

namespace ara {

class UIBlendStack : public UIStack {
public:
    void show(const std::string& name) override;
    void show(const std::string& name, float delay, double transTime);
    void setRootNode(UINode* node) override;

    void setTransitionDelay(float delay) { m_blender.setDelay(delay); }
    UINodeBlender::transType getTransType() { return m_blender.getTransType(); }

private:
    UINodeBlender                               m_blender;
};

}
