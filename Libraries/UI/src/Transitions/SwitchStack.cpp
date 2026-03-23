//
// Created by sven on 23-03-26.
//

#include <UIElements/UINodeBase/UINode.h>
#include "SwitchStack.h"

namespace ara {

void SwitchStack::show(const std::string& name) {
    if (!m_nodes.contains(name)) {
        return;
    }

    if (m_currentNode) {
        m_currentNode->setVisibility(false);
    }

    m_currentNode = m_nodes[name];
    m_currentNode->setVisibility(true);
}

}  // namespace ara