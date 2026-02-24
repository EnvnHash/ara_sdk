
#include "WindowResizeAreas.h"
//
// Created by user on 05.10.2020.
//

#include "MenuBar.h"
#include "UIWindow.h"

using namespace std;
using namespace glm;

namespace ara {

void WindowResizeAreas::addResizeAreas(UINode& root, GLWindow* win) {
#ifdef ARA_USE_GLFW
    //auto m_win = m_sharedRes->winHandle;

    // since we are using undecorated windows in order to have our own MenuBars
    // there are no grabbers to resize the window ... so we also have to build
    // those by ourselves
    int dragAreaSize = 10;
    std::vector<WindowResizeArea*> wra;
    for (int i = 0; i < 8; i++) {
        wra.emplace_back(&root.push<WindowResizeArea>());
    }

    // top
    wra[0]->setAreaType(WindowResizeArea::AreaType::top);  // top
    wra[0]->setSize(1.f, dragAreaSize);                    // top
    wra[0]->setAlign(align::center, valign::top);

    // right
    wra[1]->setAreaType(WindowResizeArea::AreaType::right);
    wra[1]->setSize(dragAreaSize, 1.f);  // right
    wra[1]->setAlign(align::right, valign::center);
    // bottom
    wra[2]->setAreaType(WindowResizeArea::AreaType::bottom);
    wra[2]->setSize(1.f, dragAreaSize);  // bottom
    wra[2]->setAlign(align::center, valign::bottom);

    // left
    wra[3]->setAreaType(WindowResizeArea::AreaType::left);
    wra[3]->setSize(dragAreaSize, 1.f);
    wra[3]->setAlign(align::left, valign::center);

    // top-left
    wra[4]->setAreaType(WindowResizeArea::AreaType::topLeft);
    wra[4]->setSize(dragAreaSize, dragAreaSize);
    wra[4]->setAlign(align::left, valign::top);

    // top-right
    wra[5]->setAreaType(WindowResizeArea::AreaType::topRight);
    wra[5]->setSize(dragAreaSize, dragAreaSize);
    wra[5]->setAlign(align::right, valign::top);

    // bottom-left
    wra[6]->setAreaType(WindowResizeArea::AreaType::bottomLeft);
    wra[6]->setSize(dragAreaSize, dragAreaSize);
    wra[6]->setAlign(align::left, valign::bottom);

    // bottom-right
    wra[7]->setAreaType(WindowResizeArea::AreaType::bottomRight);
    wra[7]->setSize(dragAreaSize, dragAreaSize);
    wra[7]->setAlign(align::right, valign::bottom);

    for (auto &it : wra) {
        it->addMouseOutCb([win](const hidData& data) {
            // don't change the mouse cursor to 0 on mouse out if the new node is also a WindowResizeArea
            if (data.newNode && static_cast<UINode *>(data.newNode)->name() == getTypeName<WindowResizeAreas>()) {
                return;
            }

            win->setMouseCursor(WinMouseIcon::arrow);
        });
    }
#endif
}

}  // namespace ara