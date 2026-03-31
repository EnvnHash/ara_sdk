//
// Created by sven on 24-03-26.
//

#include "NodeEdit.h"

using namespace std;
using namespace nlohmann;
using namespace glm;

namespace ara {

NodeEdit::NodeEdit() {
    m_enableExpandButton = false;
    m_xIdent = 0;

    Node n;
    json js;
    n.serialize(js);
    m_nodeClassKeys = &n.getClassKeys().at(n.typeName()).first;
}

void NodeEdit::rebuild() {
    if (m_node) {
        clearChildren();
        setNodeValueType(nodeValueType::root);
        auto j = m_node->asJson();
        for (auto& key : *m_nodeClassKeys) {
            j.erase(key);
        }

        m_boundClassKeys.clear();
        for (auto &[key, value] : j.items()) {
            m_boundClassKeys.emplace_back(key);
        }

        loadFromJson(j);
        setExpanded(true);
        int32_t yOffsCntr = 0;
        rebuildCollapseState(this, yOffsCntr, m_lineHeight);
    }
}

void NodeEdit::onEnter(const std::string &str) {
    JsonEditor::onEnter(str);
    auto p = dynamic_cast<NodeEdit*>(parent());
    if (p) {
        auto j = asJson();
        json extractJson;
        for (const auto &key : p->getBoundClassKeys()) {
            extractJson[key] = j[key];
        }

        auto n = p->getBoundNode();
        if (n) {
            n->deserialize(extractJson);
        }
    }
}
}
