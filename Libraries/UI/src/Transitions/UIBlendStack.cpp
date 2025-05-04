//
// Created by sven on 04-04-25.
//

#include "UIBlendStack.h"

namespace ara {

void UIBlendStack::show(const std::string& name) {
    if (!m_nodes.contains(name)) {
        return;
    }

    if (m_blender.stopped()) {
        m_blender.swap();
        m_blender.set(UINodeBlender::type::back, m_nodes[name]);
        m_blender.setEndFunc(m_onShowFunctions[name]);
        m_blender.transition(UINodeBlender::transType::frontToBack, m_transTime);
    }
}

void UIBlendStack::show(const std::string& name, float delay, double transTime) {
    setTransitionDelay(delay);
    setTransitionTime(transTime);
    show(name);
}

void UIBlendStack::setRootNode(UINode* node) {
    m_rootNode = node;
    m_blender.setRoot(node);
}

}