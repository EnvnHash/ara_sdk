//
// Created by sven on 24-03-26.
//

#include "NodeEdit.h"

using namespace std;
using namespace glm;

namespace ara {

NodeEdit::NodeEdit() {
    m_enableExpandButton = false;
    m_xIdent = 0;
}

void NodeEdit::init() {
    JsonEditor::init();
}

void NodeEdit::rebuild() {
    if (m_node) {
        clearChildren();
        setJsonObjectType(JsonEntryType::rootObject);
        auto j = m_node->asJson();
        j.erase("name");
        j.erase("typeName");
        j.erase("uuid");
        LOG << j;
        loadFromJson(j);
        setExpanded(true);
        int32_t yOffsCntr = 0;
        rebuildCollapseState(this, yOffsCntr);
    }
}

}
