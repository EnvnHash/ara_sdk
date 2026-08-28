//
// Created by sven on 30-04-26.
//

#include <UIElements/Resizable.h>
#include <UIElements/ResizableHandle.h>

#include "UISharedRes.h"

using namespace glm;
using namespace std;

namespace ara {

ResizableHandle::ResizableHandle() {
    setName(ara::getTypeName<ResizableHandle>());
    setTypeName<ResizableHandle>();
    excludeSizeFromParentContentTrans(true);
}

void ResizableHandle::mouseIn(hidData& data) {
#ifdef ARA_USE_GLFW
    if (m_corner == Corner::topLeft || m_corner == Corner::bottomRight) {
        getSharedRes()->winHandle->setMouseCursor(WinMouseIcon::lbtrResize);
    } else if (m_corner == Corner::topRight || m_corner == Corner::bottomLeft) {
        getSharedRes()->winHandle->setMouseCursor(WinMouseIcon::ltbrResize);
    } else if (m_corner == Corner::top || m_corner == Corner::bottom) {
        getSharedRes()->winHandle->setMouseCursor(WinMouseIcon::vresize);
    } else if (m_corner == Corner::left || m_corner == Corner::right) {
        getSharedRes()->winHandle->setMouseCursor(WinMouseIcon::hresize);
    } else if (m_corner == Corner::center) {
        getSharedRes()->winHandle->setMouseCursor(WinMouseIcon::move);
        getSharedRes()->reqRedraw();
    }
#endif
    data.consumed = true;
}

void ResizableHandle::mouseOut(hidData& data) {
#ifdef ARA_USE_GLFW
    getSharedRes()->winHandle->setMouseCursor(WinMouseIcon::arrow);
#endif
    data.consumed = true;
}

void ResizableHandle::mouseDrag(hidData& data) {
    data.consumed = true;
}

void ResizableHandle::mouseDown(hidData& data) {
    if (m_resizeImage) {
        m_resizeImage->setResizeStart(m_corner);
    }
    data.consumed = true;
}

void ResizableHandle::mouseUp(hidData& data) {
    if (m_resizeImage) {
        m_resizeImage->setResizeEnd();
    }
    data.consumed = true;
}

}