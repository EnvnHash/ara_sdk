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
        reset();
        int32_t y=0;
        for (auto &[key, memVar]: m_node->getMemberVariables()) {
            if (std::ranges::find(m_excludeKeys, key) != m_excludeKeys.end()) {
                continue;
            }

            m_variableEdits[key] = &push<NodeMemberVariableEdit>(NodeMemberVariableEdit::EditPar{
                .style = getStyleClass()+".variable",
                .memVar = &memVar,
                .labelText = key,
                .spacing = m_spacing,
                .lineHeight = m_lineHeight,
                .labelWidth = m_labelWidth,
                .editAlign = m_editAlign,
                .yOffs = y * (m_lineHeight + m_spacing.y),
                .options = m_optPerKey.contains(key) ? std::make_optional(&m_optPerKey[key]) : std::nullopt,
            });

            y += m_variableEdits[key]->getUnitHeight();
        }
    }
}

void NodeEdit::reset() {
    clearChildren();
    m_variableEdits.clear();
}

void NodeEdit::exclude(const std::vector<std::string>& excludeKeys) {
    m_excludeKeys = excludeKeys;
}

}
