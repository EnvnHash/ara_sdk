//
// Created by sven on 24-03-26.
//

#pragma once
#include "UIElements/DataBinding/JsonEditor.h"

namespace ara {

class Label;

class NodeEdit : public JsonEditor {
public:
    NodeEdit();
    void rebuild();

    void setNode(Node& node) {
        m_node = &node;
        addGlCb(std::to_string(reinterpret_cast<uint64_t>(this)), [&] {
            rebuild();
            return true;
        });
    }

private:
    Label*      m_label = nullptr;
    Node*       m_node = nullptr;
    int32_t     m_lineHeight = 20;
};

}
