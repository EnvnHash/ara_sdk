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
    setParseAsGenericJson(true);
}

void JsonEditor::init() {
    if (const auto p = dynamic_cast<JsonEditor*>(parent());
        m_jsonEntryType == JsonEntryType::rootObject || !p || !p->isExpanded()) {
        return;
    }

    setHeight( m_lineHeight);
    if (m_enableExpandButton) {
        addExpandButt();
    }

    m_label = &push<Label>(LabelPars{
        .pos = ivec2{ 0, 0 },
        .size = ivec2{ m_labelWidth, m_lineHeight },
        .align = align::left,
        .valign = valign::top,
        .text_color = vec4{ 1.f, 1.f, 1.f, 1.f },
        .text = m_key + ":",
        .text_align_x = align::left,
        .text_align_y = valign::center,
        .font_type = "regular",
        .font_height = m_lineHeight - 5
    });

    if (m_jsonEntryType != JsonEntryType::object && m_jsonEntryType != JsonEntryType::array) {
        m_edit = &push<UIEdit>(UINodePars{
            .pos = ivec2{ m_labelWidth + m_xSpacing, 0 },
            .size = { ivec2{ -m_labelWidth -m_xSpacing, m_lineHeight } },
            .bgColor = vec4{.15f, .15f, .15f, 1.f},
            .borderWidth = 2,
            .borderRadius = 4,
            .borderColor = vec4{.3f, .3f, .3f, 1.f},
            .padding = vec4{2.f, 2.f, 2.f, 2.f},
        });
        m_edit->setFontSize(m_lineHeight -9);

        if (m_jsonEntryType == JsonEntryType::float_number) {
            m_edit->setValue(value<float>());
        } else if (m_jsonEntryType == JsonEntryType::int_number) {
            m_edit->setValue(value<int32_t>());
        } else if (m_jsonEntryType == JsonEntryType::string) {
            m_edit->setText(value<std::string>());
        }
    }

    //m_edit->setProp();
}

void JsonEditor::addExpandButt() {
    if (!children().empty()) {
        auto& arrowButt = push<ImageButton>(UINodePars{
            .pos = ivec2{ 0.f,  3 },
            .size = ivec2{ 12, m_lineHeight - 5 },
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
            rebuildCollapseState(root, yOffsCntr);

            return true;
        });
    }
}

void JsonEditor::loadFile(const filesystem::path& p) {
    addGlCb("JsonEditorRebuild", [p, this] {
        clearChildren();
        setJsonObjectType(JsonEntryType::rootObject);
        load(p);
        setExpanded(true);

        int32_t yOffsCntr = 0;
        rebuildCollapseState(this, yOffsCntr);

        return true;
    });
}

void JsonEditor::parseNonClassEntries(const json& j, std::list<std::function<void()>*>* postLoadCbsArg) {
    for (auto& [key, value] : j.items()) {
        if (value.is_array() || value.is_object()) {
            auto& newChild = push<JsonEditor>();
            newChild.setLineHeight(m_lineHeight);
            newChild.setSpacing(m_xSpacing);
            newChild.setLabelWidth(m_labelWidth);
            newChild.setKey(key);
            newChild.setJsonObjectType(value.is_array() ? JsonEntryType::array : JsonEntryType::object);
            newChild.deserialize(value, postLoadCbsArg);
            newChild.setVisibility(false);
            // TODO: missing check if node to add already exists
        } else {
            auto childIt = std::ranges::find_if(m_children, [&](auto& el){ return el.get()->key() == key; });
            if (childIt == m_children.end()) {
                auto& je = push<JsonEditor>();
                childIt = --m_children.end();
                je.setKey(key);
                je.setLineHeight(m_lineHeight);
                je.setSpacing(m_xSpacing);
                je.setLabelWidth(m_labelWidth);
            }

            const auto child = dynamic_cast<JsonEditor*>(childIt->get());
            child->setVisibility(false);
            if (value.is_boolean()) {
                child->setValue(value.get<bool>());
                child->setJsonObjectType(JsonEntryType::boolean);
            } else if (value.is_number_float()) {
                child->setValue(value.get<float>());
                child->setJsonObjectType(JsonEntryType::float_number);
            } else if (value.is_number()) {
                child->setValue(value.get<int32_t>());
                child->setJsonObjectType(JsonEntryType::int_number);
            } else if (value.is_string()) {
                child->setValue(value.get<std::string>());
                child->setJsonObjectType(JsonEntryType::string);
            }
        }
    }
}

void JsonEditor::rebuildCollapseState(JsonEditor* nd, int32_t& yOffsCntr) {
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

        if (parent->isExpanded()) {
            nd->setYOffs(yOffsCntr);
            ++yOffsCntr;
        }

        if (!parent->isExpanded() && !uiChildren.empty()) {
            nd->setVisibility(false);
            for (const auto it : uiChildren) {
                nd->remove(it);
            }
            return;
        }
    }

    for (auto& c : nd->children()) {
        rebuildCollapseState(dynamic_cast<JsonEditor *>(c.get()), yOffsCntr);
    }
}

void JsonEditor::setYOffs(const int32_t& val) {
    m_yOffs = val;
    if (const auto p = dynamic_cast<JsonEditor*>(parent())) {
        m_yOffs -= p->getYOffs();
    }
   // LOG << getTreeDepth() ;
    setPos(getTreeDepth() * m_xIdent, (m_ySpacing + m_lineHeight) * m_yOffs);
}

void JsonEditor::setSpacing(const int32_t& s) {
    m_xSpacing = s;

    if (m_edit) {
        m_edit->setX(m_labelWidth +s);
        m_edit->setWidth(-m_labelWidth -s);
    }
    getSharedRes()->reqRedraw();
}

void JsonEditor::setLabelWidth(const int32_t& w) {
    m_labelWidth = w;

    if (m_label) {
        m_label->setWidth(w);
    }
    if (m_edit) {
        m_edit->setX(m_labelWidth +m_xSpacing);
        m_edit->setWidth(-m_labelWidth -m_xSpacing);

    }
    getSharedRes()->reqRedraw();
}

void JsonEditor::setLineHeight(const int32_t& h) {
    m_lineHeight = h;

    if (m_label) {
        m_label->setHeight(h);
    }

    if (m_edit) {
        m_edit->setHeight(h);
    }

    getSharedRes()->reqRedraw();
}

}