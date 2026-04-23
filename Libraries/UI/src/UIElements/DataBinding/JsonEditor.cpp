//
// Created by sven on 25-02-26.
//

#include "UIElements/DataBinding/JsonEditor.h"
#include "UISharedRes.h"
#include "UIElements/Button/ImageButton.h"

using namespace std;
using namespace glm;
using namespace nlohmann;

namespace ara {
JsonEditor::JsonEditor() {
    setTypeName<JsonEditor>();
    setName(getTypeName<JsonEditor>());
}

void JsonEditor::init() {
    if (const auto p = dynamic_cast<JsonEditor*>(parent());
        m_nodeValueType == nodeValueType::root || !p || !p->isExpanded()) {
        return;
    }

    setHeight(m_lineHeight);
    if (m_enableExpandButton) {
        addExpandButt();
    }

    m_label = &push<Label>(LabelPars{
        .size = ivec2{ m_labelWidth, m_lineHeight },
        .style = getStyleClass()+".label",
        .color = vec4{ 1.f, 1.f, 1.f, 1.f },
        .text = m_key + ":",
        .textAlignX = align::left,
        .textAlignY = valign::center,
        .fontType = "regular",
        .fontHeight = 22
    });

    if (m_nodeValueType != nodeValueType::object && m_nodeValueType != nodeValueType::array) {
        m_edit = &push<UIEdit>(UINodePars{
            .pos = ivec2{ m_labelWidth + m_xSpacing, 0 },
            .size = { ivec2{ -m_labelWidth -m_xSpacing, m_lineHeight } },
            .bgColor = vec4{.15f, .15f, .15f, 1.f},
            .style = getStyleClass()+".edit",
            .borderWidth = 2,
            .borderRadius = 4,
            .borderColor = vec4{.3f, .3f, .3f, 1.f},
            .padding = vec4{2.f, 2.f, 2.f, 2.f},
        });
        m_edit->setFontSize(22);
        m_edit->setUseWheel(true);

        setupEdit();
    }
}

void JsonEditor::setupEdit() {
    if (m_nodeValueType == nodeValueType::floating) {
        m_edit->setValue(value<float>());
    } else if (m_nodeValueType == nodeValueType::integer) {
        m_edit->setValue(value<int32_t>());
    } else if (m_nodeValueType == nodeValueType::string) {
        m_edit->setText(value<std::string>());
    }

    m_edit->addEnterCb([&](const std::string &str) {
        onEnter(str);
    }, this);
}

void JsonEditor::onEnter(const std::string &str) {
    if (m_nodeValueType == nodeValueType::floating) {
        setValue(m_edit->getValue<float>());
    } else if (m_nodeValueType == nodeValueType::integer) {
        setValue(m_edit->getValue<int32_t>());
    } else if (m_nodeValueType == nodeValueType::string) {
        setValue(str);
    }
    if (const auto root = getJsonRoot()) {
        root->save(true);
    }
}

void JsonEditor::updateStyleIt(ResNode* node, const state st, const std::string& styleClass) {
    UINode::updateStyleIt(node, st, styleClass);

    if (m_nodeValueType == nodeValueType::root || m_nodeValueType == nodeValueType::undefined) {
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

void JsonEditor::addExpandButt() {
    if (!children().empty()) {
        auto& arrowButt = push<ImageButton>(UINodePars{
            .pos = ivec2{ 0.f,  3 },
            .size = ivec2{ 12, m_lineHeight - 5 },
            .style = getStyleClass()+".expand",
            .align = align::left,
            .valign = valign::top,
        });

        arrowButt.setIsToggle(true);
        arrowButt.setImg("Icons/icon-arrow-right.png", 1);
        arrowButt.setOnStateImg("Icons/icon-arrow-down.png", 1);
        arrowButt.setToggleState(m_expanded);

        arrowButt.setClickedCb([this] {
            m_expanded = !m_expanded;
            if (!m_expanded) {
                for (const auto jsonChildren = findChildrenByType<JsonEditor>();
                    const auto it : jsonChildren) {
                    it->setExpanded(false);
                }
            }
            const auto root = getJsonRoot();
            int32_t yOffsCntr = 0;
            rebuildCollapseState(root, yOffsCntr, m_lineHeight);

            return true;
        });
    }
}

void JsonEditor::loadFile(const filesystem::path& p) {
    addGlCb("JsonEditorRebuild", [p, this] {
        clearChildren();
        load(p, true);
        setExpanded(true);

        int32_t yOffsCntr = 0;
        rebuildCollapseState(this, yOffsCntr, m_lineHeight);

        return true;
    });
}

Node& JsonEditor::createNewElement() {
    auto& newChild = push<JsonEditor>();
    initChild(newChild);
    newChild.setVisibility(false);
    return newChild;
}

void JsonEditor::initChild(JsonEditor& je)  {
    je.addStyleClass(getStyleClass());
    if (m_label && m_edit) {
        je.setLineHeight(static_cast<int32_t>(m_label->getSize().y));
        je.setSpacing(static_cast<int32_t>(m_edit->getPos().x - m_label->getSize().x - m_label->getPos().x));
        je.setLabelWidth(static_cast<int32_t>(m_label->getSize().x));
    }
}

void JsonEditor::setLineOffset(JsonEditor* nd, int32_t& yOffsCntr, int32_t& lineHeight) {
    if (!nd) {
        return;
    }

    if (const auto parent = dynamic_cast<JsonEditor*>(nd->parent())) {
        nd->setYOffs(parent, yOffsCntr, lineHeight);
    }

    for (auto& c : nd->children()) {
        setLineOffset(dynamic_cast<JsonEditor *>(c.get()), yOffsCntr, lineHeight);
    }
}

void JsonEditor::rebuildCollapseState(JsonEditor* nd, int32_t& yOffsCntr, int32_t& lineHeight) {
    if (!nd) {
        return;
    }

    if (const auto parent = dynamic_cast<JsonEditor*>(nd->parent())) {
        const auto uiChildren = nd->findChildrenByTypes({ "UIEdit", "Label", "ImageButton"});
        if (parent->isExpanded() && uiChildren.empty()) {
            nd->setVisibility(true);
            if (nd->isInited()) { // force reinit if already inited
                nd->init();
            }
        }

        nd->setYOffs(parent, yOffsCntr, lineHeight);

        if (!parent->isExpanded() && !uiChildren.empty()) {
            nd->setVisibility(false);
            for (const auto it : uiChildren) {
                nd->remove(it);
            }
            return;
        }
    }

    for (auto& c : nd->children()) {
        rebuildCollapseState(dynamic_cast<JsonEditor *>(c.get()), yOffsCntr, lineHeight);
    }
}

void JsonEditor::setYOffs(const JsonEditor* parent, int32_t& yOffsCntr, const int32_t& lineHeight) {
    if (parent->isExpanded()) {
        if (getNodeValueType() != nodeValueType::root) {
            setLineHeight(lineHeight);
        }

        m_yOffs = yOffsCntr++;
        m_treeDepth = static_cast<int32_t>(getTreeDepth());
        m_custPos.x = m_treeDepth != -1 ? (m_treeDepth - parent->getDepth()) * m_xIdent : 0;
        m_custPos.y = (m_ySpacing + lineHeight) * (m_yOffs - parent->getYOffs());
        setPos(m_custPos);
    }
}

int32_t JsonEditor::getLineHeight(ResNode* node) const {
    int32_t lineHeight = m_lineHeight;
    if (const auto editNode = node->findNode("edit")) {
        if (const auto numNode = editNode->findNumericNode("height"); get<ResNode*>(numNode)) {
            if (get<unitType>(numNode) == unitType::Percent) {
                lineHeight = static_cast<int32_t>(std::stof(get<std::string>(numNode)) * 0.01f);
            } else {
                lineHeight = std::stoi(get<std::string>(numNode));
            }
        }
    }
    return lineHeight;
}

void JsonEditor::setSpacing(const int32_t& s) {
    m_xSpacing = s;
}

void JsonEditor::setLabelWidth(const int32_t& w) {
    m_labelWidth = w;
}

void JsonEditor::setLineHeight(const int32_t& h) {
    m_lineHeight = h;
}

}