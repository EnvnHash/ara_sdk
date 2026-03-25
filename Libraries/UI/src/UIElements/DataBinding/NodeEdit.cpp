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

    m_valChangedCb = [this](const std::string& str) {
        if (m_jsonEntryType == JsonEntryType::float_number) {
            LOG << "m_edit->changed float " << m_edit->getValue<float>();
        } else if (m_jsonEntryType == JsonEntryType::int_number) {
            LOG << "m_edit->changed int " << m_edit->getValue<int32_t>();
        } else if (m_jsonEntryType == JsonEntryType::string) {
            LOG << "m_edit->changed " << str;
        }
    };
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
        rebuildCollapseState(this, yOffsCntr, m_lineHeight);
    }
}

}
