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

DropDownMenu* DropDownMenuBar::addDropDownMenu(std::string name) {
    m_menuButtSize = m_sharedRes->gridSize.x * 2;

    auto ddm = addChild<DropDownMenu>();
    ddm->setMenuName(name);
    ddm->setPos(m_menuButtSize * (int)m_menuEntries.size(), 0);
    ddm->setSize(m_menuButtSize, 1.f);

    m_menuEntries.push_back(ddm);
    return ddm;
}

void DropDownMenuBar::globalMouseDown(hidData* data) {
    for (auto& it : m_menuEntries) it->globalMouseDown(data);
}

}  // namespace ara