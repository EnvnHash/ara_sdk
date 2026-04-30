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
}

void ResizableHandle::mouseIn(hidData& data) {
    if (m_corner == Corner::topLeft || m_corner == Corner::bottomRight) {
        getSharedRes()->winHandle->setMouseCursor(WinMouseIcon::lbtrResize);
    } else if (m_corner == Corner::topRight || m_corner == Corner::bottomLeft) {
        getSharedRes()->winHandle->setMouseCursor(WinMouseIcon::ltbrResize);
    } else if (m_corner == Corner::top || m_corner == Corner::bottom) {
        getSharedRes()->winHandle->setMouseCursor(WinMouseIcon::vresize);
    } else if (m_corner == Corner::left || m_corner == Corner::right) {
        getSharedRes()->winHandle->setMouseCursor(WinMouseIcon::hresize);
    }
}

void ResizableHandle::mouseOut(hidData& data) {
    getSharedRes()->winHandle->setMouseCursor(WinMouseIcon::arrow);
}

void ResizableHandle::mouseDown(hidData& data) {
    if (m_resizeImage) {
        m_resizeImage->setResizeStart(m_corner);
    }
}

void ResizableHandle::mouseUp(hidData& data) {
    if (m_resizeImage) {
        m_resizeImage->setResizeEnd();
    }
}

}