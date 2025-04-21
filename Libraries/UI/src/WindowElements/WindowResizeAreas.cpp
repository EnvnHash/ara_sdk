
#include "WindowResizeAreas.h"
//
// Created by user on 05.10.2020.
//

#include "MenuBar.h"
#include "UIWindow.h"

using namespace std;
using namespace glm;

namespace ara {

WindowResizeAreas::WindowResizeAreas() : UINode() {
    excludeFromObjMap(true);
    setName(getTypeName<WindowResizeAreas>());
    setFocusAllowed(false);
}

void WindowResizeAreas::init() {
#ifdef ARA_USE_GLFW
    auto m_win = m_sharedRes->winHandle;

    // since we are using undecorated windows in order to have our own MenuBars
    // there are no grabbers to resize the window ... so we also have to build
    // those by ourselves
    int dragAreaSize = 10;
    for (int i = 0; i < 8; i++) m_winResizeAreas.push_back(addChild<WindowResizeArea>());

    // top
    m_winResizeAreas[0]->setAreaType(WindowResizeArea::AreaType::top);  // top
    m_winResizeAreas[0]->setSize(1.f, dragAreaSize);                    // top
    m_winResizeAreas[0]->setAlign(align::center, valign::top);

    // right
    m_winResizeAreas[1]->setAreaType(WindowResizeArea::AreaType::right);
    m_winResizeAreas[1]->setSize(dragAreaSize, 1.f);  // right
    m_winResizeAreas[1]->setAlign(align::right, valign::center);
    // bottom
    m_winResizeAreas[2]->setAreaType(WindowResizeArea::AreaType::bottom);
    m_winResizeAreas[2]->setSize(1.f, dragAreaSize);  // bottom
    m_winResizeAreas[2]->setAlign(align::center, valign::bottom);

    // left
    m_winResizeAreas[3]->setAreaType(WindowResizeArea::AreaType::left);
    m_winResizeAreas[3]->setSize(dragAreaSize, 1.f);
    m_winResizeAreas[3]->setAlign(align::left, valign::center);

    // top-left
    m_winResizeAreas[4]->setAreaType(WindowResizeArea::AreaType::topLeft);
    m_winResizeAreas[4]->setSize(dragAreaSize, dragAreaSize);
    m_winResizeAreas[4]->setAlign(align::left, valign::top);

    // top-right
    m_winResizeAreas[5]->setAreaType(WindowResizeArea::AreaType::topRight);
    m_winResizeAreas[5]->setSize(dragAreaSize, dragAreaSize);
    m_winResizeAreas[5]->setAlign(align::right, valign::top);

    // bottom-left
    m_winResizeAreas[6]->setAreaType(WindowResizeArea::AreaType::bottomLeft);
    m_winResizeAreas[6]->setSize(dragAreaSize, dragAreaSize);
    m_winResizeAreas[6]->setAlign(align::left, valign::bottom);

    // bottom-right
    m_winResizeAreas[7]->setAreaType(WindowResizeArea::AreaType::bottomRight);
    m_winResizeAreas[7]->setSize(dragAreaSize, dragAreaSize);
    m_winResizeAreas[7]->setAlign(align::right, valign::bottom);

    for (auto &it : m_winResizeAreas)
        it->addMouseOutCb([m_win](hidData *data) {
            // don't change the mouse cursor to 0 on mouse out if the new node
            // is also a WindowResizeArea
            if (data->newNode && ((UINode *)data->newNode)->getName() == getTypeName<WindowResizeAreas>()) return;

            m_win->setMouseCursor(WinMouseIcon::arrow);
        });

#endif
}

}  // namespace ara