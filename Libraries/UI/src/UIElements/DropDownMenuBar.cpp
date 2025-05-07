//
// Created by user on 05.10.2020.
//

#include "DropDownMenuBar.h"
#include "UIWindow.h"

using namespace std;
using namespace glm;

namespace ara {

DropDownMenuBar::DropDownMenuBar() : Div() {
    m_canReceiveDrag = true;
    setName(getTypeName<DropDownMenuBar>());
    setFocusAllowed(false);
}

DropDownMenu* DropDownMenuBar::addDropDownMenu(const std::string& name) {
    m_menuButtSize = m_sharedRes->gridSize.x * 2;

    auto ddm = addChild<DropDownMenu>();
    ddm->setMenuName(name);
    ddm->setPos(m_menuButtSize * static_cast<int>(m_menuEntries.size()), 0);
    ddm->setSize(m_menuButtSize, 1.f);

    m_menuEntries.emplace_back(ddm);
    return ddm;
}

void DropDownMenuBar::globalMouseDown(hidData* data) const {
    for (const auto& it : m_menuEntries) {
        it->globalMouseDown(data);
    }
}

}  // namespace ara