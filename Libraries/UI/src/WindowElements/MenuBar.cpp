//
// Created by user on 05.10.2020.
//

#include "MenuBar.h"

#include "GLBase.h"

#include "UIWindow.h"

using namespace std;
using namespace glm;

namespace ara {

MenuBar::MenuBar() : Image() {
    m_canReceiveDrag = true;
    setName(getTypeName<MenuBar>());
    setFocusAllowed(false);
}

void MenuBar::init() {
    Image::init();

    int iconsRightSizeWidth = 150;
    int containersWidth     = iconsRightSizeWidth / 3;

    // container for right side icons
    m_iconsRightSide = addChild<Div>();
    m_iconsRightSide->setName("IconContainer");
    m_iconsRightSide->setSize(iconsRightSizeWidth, 1.f);
    m_iconsRightSide->setAlignX(align::right);
    m_iconsRightSide->setX(8);

    if (m_showButtons) {
        m_menButtons[butType::close] = m_iconsRightSide->addChild<ImageButton>("menuBar.close");
        m_menButtons[butType::close]->setAltText("Close");
        m_menButtons[butType::close]->setClickedCb(m_closeFunc);
        m_menButtons[butType::close]->setName("close_button");

        if (m_enableMinMaxButtons) {
            m_menButtons[butType::maximize] = m_iconsRightSide->addChild<ImageButton>("menuBar.maximize");
            m_menButtons[butType::maximize]->setAltText("Maximize");
            m_menButtons[butType::maximize]->setIsToggle(true);
            m_menButtons[butType::maximize]->setToggleCb([this](bool state) {
                if (!m_enableMinMaxButtons) return;
                if (state && m_maximizeFunc) m_maximizeFunc();
                if (!state && m_restoreFunc) m_restoreFunc();
            });
            m_menButtons[butType::maximize]->setName("maximize_button");

            m_menButtons[butType::minimize] = m_iconsRightSide->addChild<ImageButton>("menuBar.minimize");
            m_menButtons[butType::minimize]->setAltText("Minimize");
            m_menButtons[butType::minimize]->setClickedCb([this] {
                if (m_enableMinMaxButtons && m_minimizeFunc) m_minimizeFunc();
            });
            m_menButtons[butType::minimize]->setName("minimize_button");
        }

        m_brightAdjCol = vec4{m_brightAdj >= 0.f ? m_brightAdj : m_sharedRes->colors->at(uiColors::font).r};

        int i = 0;
        for (const auto& but : m_menButtons)
            if (but.second) {
                but.second->setSize(containersWidth, 1.f);
                but.second->setPadding(8.f);
                but.second->setAlign(align::right, valign::center);
                but.second->setColor(m_brightAdjCol);
                but.second->setX(i * -containersWidth);
                i++;
            }
    }
}

void MenuBar::mouseUp(hidData* data) {
#ifdef ARA_USE_GLFW
    if (m_win) {
        m_win->setBlockResizing(false);  // somehow window receives size changes on windows when
                                         // moved fast between displays
        m_win->setBlockMouseIconSwitch(false);
    }
#endif
}

void MenuBar::mouseDown(hidData* data) {
#ifdef ARA_USE_GLFW
    if (data->hit && getWindow() && m_enableMinMaxButtons && data->isDoubleClick) {
        if (!getWindow()->getWinHandle()->isMaximized() && m_maximizeFunc) {
            m_maximizeFunc();
            if (m_menButtons[butType::maximize]) m_menButtons[butType::maximize]->setSelected(true);
        }

        if (getWindow()->getWinHandle()->isMaximized() && m_restoreFunc) {
            m_restoreFunc();
            if (m_menButtons[butType::maximize]) m_menButtons[butType::maximize]->setSelected(false);
        }
    }
#endif
}

void MenuBar::mouseDrag(hidData* data) {
#ifdef ARA_USE_GLFW
    ivec2 pixOffs{0};
    if (!m_win) return;

    auto absMousePos = m_win->getAbsMousePos();

    if (data->dragStart) {
        m_dragStartWinPos = m_win->getPosition();
        m_win->setBlockResizing(true);  // somehow window receives size changes on windows when
                                        // moved fast between displays
        m_win->setBlockMouseIconSwitch(true);
        m_mouseDownPixelPos = absMousePos;

    } else {
        pixOffs = absMousePos - m_mouseDownPixelPos;
        ((UIWindow*)m_sharedRes->win)->addWinCb([this, pixOffs]() {
            m_win->setPosition(m_dragStartWinPos.x + pixOffs.x, m_dragStartWinPos.y + pixOffs.y);
        });
    }
#endif

    data->consumed = true;
}

void MenuBar::setEnableMinMaxButtons(bool val) {
    m_enableMinMaxButtons = val;

    if (m_menButtons[butType::maximize]) m_menButtons[butType::maximize]->setVisibility(val);

    if (m_menButtons[butType::minimize]) m_menButtons[butType::minimize]->setVisibility(val);
}

}  // namespace ara
