//
// Created by sven on 23-03-26.
//

#pragma once

#include <Transitions/UIStack.h>

namespace ara {

class SwitchStack : public UIStack {
public:
    void show(const std::string& name) override;

private:
    UINode* m_currentNode = nullptr;
};

}