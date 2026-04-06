//
// Created by sven on 24-03-26.
//

#pragma once
#include "UIElements/DataBinding/JsonEditor.h"

namespace ara {

class Label;

class NodeEdit : public Div {
public:
    NodeEdit();
    void rebuild();

    auto& getBoundNode() { return m_node; }

    void setNode(Node& node) {
        m_node = &node;
        m_node->createProperties();
        addGlCb(std::to_string(reinterpret_cast<uint64_t>(this)), [&] {
            rebuild();
            return true;
        });
    }

    void setSpacing(const glm::ivec2& s) { m_spacing = s; }
    void setLabelWidth(const int32_t& w) { m_labelWidth = w; }
    void setLineHeight(const int32_t& h) { m_lineHeight = h; }

private:
    Label*      m_label = nullptr;
    Node*       m_node = nullptr;
    glm::ivec2  m_spacing {5, 5};
    int32_t     m_lineHeight = 20;
    int32_t     m_labelWidth = 180;
};

}
