//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <Utils/FBO.h>
#include <Utils/PingPongFbo.h>
#include <UIElements/Button/Button.h>
#include <UIElements/ScrollView.h>
#include <UISharedRes.h>
#include <UIElements/Image.h>

#include "ComboBox.h"

using namespace glm;
using namespace std;

namespace ara {

void ComboBox::init() {
    DropDownMenu::init();

    setFocusAllowed(false);
    setBorderRadius(4);
    setHeight(getSharedRes()->gridSize.y);
    setBackgroundColor(getSharedRes()->colors->at(uiColors::sepLine));

    auto downIcon = addChild<Image>();
    downIcon->excludeFromObjMap(true);
    downIcon->setImg("Icons/icon-arrow-down.png", 1);
    downIcon->setSize(12, 1.f);
    downIcon->setX(-11);
    downIcon->setAlign(align::right, valign::center);

    if (!getStyleClass().empty()) {
        downIcon->addStyleClass(getStyleClass() + ".downIcon");
    }

    m_menuEntryButt->setFontSize(19);
    m_menuEntryButt->setFontType("bold");
    m_menuEntryButt->setColor(1.f, 1.f, 1.f, 1.f);
    m_menuEntryButt->setX(18);
}

void ComboBox::open() {
    if (m_open) {
        close();
        return;
    }

    // create on top of all nodes
    m_entryList = getRoot()->addChild<ScrollView>(getStyleClass() + ".list");
    m_entryList->setPos(static_cast<int>(getWinPos().x), static_cast<int>(getWinPos().y + getSharedRes()->gridSize.y));
    m_entryList->setWidth(static_cast<int>(getSize().x));
    m_entryList->setBorderRadius(5);
    m_entryList->setHeight(std::min<int>(static_cast<int>(m_entries.size()), m_maxListEntries) * m_listEntryHeight);
    m_entryList->setBackgroundColor(getSharedRes()->colors->at(uiColors::background));

    rebuildEntryList();

    m_open = true;
}

void ComboBox::rebuildEntryList() {
    if (m_entryList) {
        dynamic_cast<ScrollView*>(m_entryList)->clearContentChildren();
    } else {
        return;
    }

    DropDownMenu::rebuildEntryList();
}

}  // namespace ara