//
// Created by sven on 24-03-26.
//

#include "NodeEdit.h"
#include "NodeMemberVariableEdit.h"

using namespace std;
using namespace nlohmann;
using namespace glm;

namespace ara {

NodeEdit::NodeEdit() {
    setTypeName<NodeEdit>();
    setName(getTypeName<NodeEdit>());
}

void NodeEdit::rebuild() {
    if (m_node) {
        clearChildren();

        int32_t y=0;
        for (auto &[key, memVar]: m_node->getMemberVariables()) {
            if (std::ranges::find(m_excludeKeys, key) != m_excludeKeys.end()) {
                continue;
            }
            auto& nmve = push<NodeMemberVariableEdit>({ .style = getStyleClass() });
            nmve.setMemberVar(memVar);
            nmve.setLabelText(key);
            nmve.setSpacing(m_spacing.x);
            nmve.setLineHeight(m_lineHeight);
            nmve.setLabelWidth(m_labelWidth);
            nmve.setY(y++ * (m_lineHeight + m_spacing.y));
        }
    }
}

void NodeEdit::exclude(const std::vector<std::string>& excludeKeys) {
    m_excludeKeys = excludeKeys;
}

}
