//
// Created by sven on 01-04-26.
//

#include "UIElements/Div.h"
#include "UIElements/Text/Label.h"
#include "UIElements/DataBinding/NodeMemberVariableEdit.h"

using namespace std;
using namespace glm;

namespace ara {

NodeMemberVariableEdit::NodeMemberVariableEdit() {
    setTypeName<NodeMemberVariableEdit>();
    setName(getTypeName<NodeMemberVariableEdit>());
}

void NodeMemberVariableEdit::init() {
    setHeight(m_lineHeight);

    m_label = &push<Label>(LabelPars{
        .pos = ivec2{ 0, 0 },
        .size = ivec2{ m_labelWidth, m_lineHeight },
        .style = getStyleClass()+".label",
        .align = align::left,
        .valign = valign::top,
        .color = vec4{ 1.f, 1.f, 1.f, 1.f },
        .text = m_text + ":",
        .textAlignX = align::left,
        .textAlignY = valign::center,
        .fontType = "regular",
        .fontHeight = 22
    });

    if (m_memVar) {
        setEditValFromMemberVar();
    }
}

void NodeMemberVariableEdit::setEditValFromMemberVar() {
    if (!m_memVar) {
        return;
    }

    if (m_edit) {
        remove(m_edit);
    }

    if (!m_arrayEdit.empty()) {
        for (const auto it : m_arrayEdit) {
            remove(it);
        }
    }

    auto stdWidth = -m_labelWidth -m_xSpacing;
    if (m_memVar->typeIndex == tpi::tp_float) {
        createSingleEdit<float>(std::any_cast<float>(m_memVar->get()), stdWidth);
    } else if (m_memVar->typeIndex == tpi::tp_int32) {
        createSingleEdit<int32_t>(std::any_cast<int32_t>(m_memVar->get()), stdWidth);
    } else if (m_memVar->typeIndex == tpi::tp_string) {
        createSingleEdit<std::string>(std::any_cast<std::string>(m_memVar->get()), stdWidth);
    } else if (m_memVar->typeIndex == tpi::tp_vector_float) {
        createArrayEdit<float>();
    } else if (m_memVar->typeIndex == tpi::tp_vector_int32) {
        createArrayEdit<int32_t>();
    } else if (m_memVar->typeIndex == tpi::tp_vector_string) {
        createArrayEdit<std::string>();
    }
}

/*
void NodeMemberVariableEdit::updateStyleIt(ResNode* node, const state st, const std::string& styleClass) {
    UINode::updateStyleIt(node, st, styleClass);

    if (m_nodeValueType == nodeValueType::root) {
        m_lineHeight = getLineHeight(node);
        int32_t yOffsCntr = 0;
        setLineOffset(this, yOffsCntr, m_lineHeight);
    } else {
        // override standard UINode settings
        m_setStyleFunc[state::none][styleInit::x] = [this] { setX(m_custPos.x, state::none); };
        m_setStyleFunc[state::none][styleInit::y] = [this] { setY(m_custPos.y, state::none); };
        m_setStyleFunc[state::none][styleInit::height] = [this] { setHeight(m_lineHeight, state::none); };
    }
}

void NodeMemberVariableEdit::setLineOffset(NodeMemberVariableEdit* nd, int32_t& yOffsCntr, int32_t& lineHeight) {
    if (!nd) {
        return;
    }

    if (const auto parent = dynamic_cast<NodeMemberVariableEdit*>(nd->parent())) {
        nd->setYOffs(parent, yOffsCntr, lineHeight);
    }

    for (auto& c : nd->children()) {
        setLineOffset(dynamic_cast<NodeMemberVariableEdit*>(c.get()), yOffsCntr, lineHeight);
    }
}

void NodeMemberVariableEdit::setYOffs(const NodeMemberVariableEdit* parent, int32_t& yOffsCntr, const int32_t& lineHeight) {
    if (getNodeValueType() != nodeValueType::root) {
        setLineHeight(lineHeight);
    }

    m_yOffs = yOffsCntr++;

    m_custPos.y = (m_ySpacing + lineHeight) * (m_yOffs - parent->getYOffs());
    setPos(m_custPos);
}

int32_t NodeMemberVariableEdit::getLineHeight(ResNode* node) const {
    int32_t lineHeight = m_lineHeight;
    if (const auto editNode = node->findNode("edit")) {
        if (const auto numNode = editNode->findNumericNode("height"); get<ResNode*>(numNode)) {
            if (get<unitType>(numNode) == unitType::Percent) {
                lineHeight = std::stof(get<std::string>(numNode)) * 0.01f;
            } else {
                lineHeight = std::stoi(get<std::string>(numNode));
            }
        }
    }
    return lineHeight;
}
*/

void NodeMemberVariableEdit::setLabelText(const std::string& text) {
    m_text = text;
    if (m_label) {
        m_label->setText(text);
    }
}



}