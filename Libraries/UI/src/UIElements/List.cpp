//
// Created by user on 08.08.2021.
//

#include "List.h"
#include <Asset/ResNode.h>

using namespace std;

namespace ara {

ListBase::ListBase() {
    setName(getTypeName<ListBase>());
    setFocusAllowed(false);
}

void ListBase::init() {
    ScrollView::init();

    // table must be always on top of ScrollViews children, in order to have the
    // children's bounding box being calculated correctly before ScrollBar
    // onChange callbacks are evaluated
    m_table = addChild<UITable>();
    m_table->setDynamicHeight(true);
}

void ListBase::loadStyleDefaults() {
    UINode::loadStyleDefaults();
    m_setStyleFunc[state::none][styleInit::rowHeight] = [this] { m_rowHeight = 30.f; };
}

void ListBase::updateStyleIt(ResNode* node, const state st, const std::string& styleClass) {
    UINode::updateStyleIt(node, st, styleClass);

    if (const auto rh = node->findNumericNode("rowHeight"); get<ResNode*>(rh)) {
        if (get<unitType>(rh) == unitType::Percent) {
            float val                                = stof(get<string>(rh)) * 0.01f;
            m_setStyleFunc[st][styleInit::rowHeight] = [this, val] {
                m_rowHeight = val;
                addGlCb("rbList", [this] {
                    rebuild();
                    return true;
                });
            };
        } else {
            int val                                  = stoi(get<string>(rh));
            m_setStyleFunc[st][styleInit::rowHeight] = [this, val] {
                m_rowHeight = static_cast<float>(val);
                addGlCb("rbList", [this] {
                    rebuild();
                    return true;
                });
            };
        }
    }
}

}  // namespace ara