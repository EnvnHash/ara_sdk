//
// Created by sven on 01-04-25.
//

#include "Transitions/UIStack.h"

namespace ara {

void UIStack::setRootNode(UINode* node) {
    m_rootNode = node;
}

UINode* UIStack::get(const std::string& name) {
    return m_nodes.contains(name) ? m_nodes[name] : nullptr;
}


}