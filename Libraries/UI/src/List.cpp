//
// Created by user on 08.08.2021.
//

#include "List.h"

#include <Asset/ResNode.h>

using namespace std;

namespace ara {

ListBase::ListBase() : ScrollView() {
    setName(getTypeName<ListBase>());
    setFocusAllowed(false);
}

ListBase::ListBase(std::string&& styleClass) : ScrollView(std::move(styleClass)) {
    setName(getTypeName<ListBase>());
    setFocusAllowed(false);
}

void ListBase::init() {
    ScrollView::init();

    // table must be always on top of ScrollViews children, in order to have the
    // children's bounding box being calculated correctly before ScrollBar
    // onChange callbacks are evaluated
    m_table = ScrollView::addChild<UITable>();
    m_table->setDynamicHeight(true);
}

void ListBase::loadStyleDefaults() {
    UINode::loadStyleDefaults();
    m_setStyleFunc[state::none][styleInit::rowHeight] = [this]() { m_rowHeight = 30.f; };
}

void ListBase::updateStyleIt(ResNode* node, state st, std::string& styleClass) {
    UINode::updateStyleIt(node, st, styleClass);

    auto rh = node->findNumericNode("rowHeight");
    if (get<ResNode*>(rh)) {
        if (get<unitType>(rh) == unitType::Percent) {
            float val                                = stof(get<string>(rh)) * 0.01f;
            m_setStyleFunc[st][styleInit::rowHeight] = [this, val, st]() {
                m_rowHeight = val;
                addGlCb("rbList", [this] {
                    rebuild();
                    return true;
                });
            };
        } else {
            int val                                  = stoi(get<string>(rh));
            m_setStyleFunc[st][styleInit::rowHeight] = [this, val, st]() {
                m_rowHeight = (float)val;
                addGlCb("rbList", [this] {
                    rebuild();
                    return true;
                });
            };
        }
    }
}

}  // namespace ara