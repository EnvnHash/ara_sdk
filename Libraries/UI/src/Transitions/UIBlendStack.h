//
// Created by sven on 01-04-25.
//

#pragma once

#include <Transitions/UIStack.h>
#include <Transitions/UINodeBlender.h>

namespace ara {

class UIBlendStack : public UIStack {
public:
    virtual ~UIBlendStack() = default;
    void show(const std::string& name) override;
    void show(const std::string& name, float delay, double transTime);
    void setRootNode(UINode* node) override;

    void setTransitionDelay(float delay) { m_blender.setDelay(delay); }
    UINodeBlender::transType getTransType() { return m_blender.getTransType(); }
    std::string& getCurrStackItemName() { return m_currStackItemName; }
    void setCurrStackItemName(const std::string& item) { m_currStackItemName = item; }
    void setSwitchCb(const std::function<void(std::string)>& cb) { m_switchCb = cb; }

private:
    UINodeBlender   m_blender;
    std::string     m_currStackItemName;
    std::function<void(std::string)>  m_switchCb = nullptr;
};

}
