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
            auto& nmve = push<NodeMemberVariableEdit>();
            nmve.setMemberVar(memVar);
            nmve.setLabelText(key);
            nmve.setSpacing(m_xSpacing);
            nmve.setLineHeight(m_lineHeight);
            nmve.setLabelWidth(m_labelWidth);
            nmve.setY(y * m_lineHeight);
        }
    }
}

}
