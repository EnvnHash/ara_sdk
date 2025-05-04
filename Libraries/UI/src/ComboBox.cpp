//
// Created by user on 27.01.2021.
//

#include "ComboBox.h"
#include "Image.h"
#include "Button/Button.h"
#include "ScrollView.h"
#include <Utils/PingPongFbo.h>

using namespace glm;
using namespace std;

namespace ara {

void ComboBox::init() {
    DropDownMenu::init();

    setFocusAllowed(false);
    setBorderRadius(4);
    setHeight(m_sharedRes->gridSize.y);
    setBackgroundColor(m_sharedRes->colors->at(uiColors::sepLine));

    m_downIcon = addChild<Image>();
    m_downIcon->excludeFromObjMap(true);
    m_downIcon->setImg("Icons/icon-arrow-down.png", 1);
    m_downIcon->setSize(12, 1.f);
    m_downIcon->setX(-11);
    m_downIcon->setAlign(align::right, valign::center);

    if (!getStyleClass().empty() && m_downIcon) {
        m_downIcon->addStyleClass(getStyleClass() + ".downIcon");
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
    m_entryList->setPos(static_cast<int>(getWinPos().x), static_cast<int>(getWinPos().y + m_sharedRes->gridSize.y));
    m_entryList->setWidth(static_cast<int>(getSize().x));
    m_entryList->setBorderRadius(5);
    m_entryList->setHeight(std::min<int>(static_cast<int>(m_entries.size()), m_maxListEntries) * m_listEntryHeight);
    m_entryList->setBackgroundColor(m_sharedRes->colors->at(uiColors::background));

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