//
// Created by user on 05.10.2020.
//

#include "WindowResizeArea.h"

#include "UIWindow.h"

using namespace std;
using namespace glm;

namespace ara {

WindowResizeArea::WindowResizeArea() : m_type(AreaType::top) {
    m_canReceiveDrag = true;
    setFocusAllowed(false);
    setName(getTypeName<WindowResizeArea>());
    memset(&m_bgColor[0], 0, 16);
}

void WindowResizeArea::init() {
    if (m_drawImmediate && m_shCol) m_shdr = m_shCol->getUIObjMapOnly();
}

void WindowResizeArea::updateDrawData() {
    if (m_drawImmediate) {
        if (!m_uniBlock.isInited()) {
            m_uniBlock.addVarName("pvm", getMVPMatPtr(), GL_FLOAT_MAT4);
            m_uniBlock.addVarName("size", &m_size[0], GL_FLOAT_VEC2);
            m_uniBlock.addVarName(m_objIdName, &m_objIdMin, GL_UNSIGNED_INT);
        } else {
            m_uniBlock.update();
        }
    } else {
        if (m_indDrawBlock.vaoData.empty()) {
            return;
        }

        auto dIt = m_indDrawBlock.vaoData.begin();

        m_zPos = 0.f;

        for (const auto& it : stdQuadVertices) {
            dIt->pos    = *getMvp() * vec4(it * m_size, 0.f, 1.0);
            dIt->aux2.z = m_excludeFromObjMap ? 0.f : static_cast<float>(m_objIdMin);
            dIt->aux2.w = m_zPos;
            ++dIt;
        }
    }
}

void WindowResizeArea::mouseUp(hidData* data) {
#ifdef ARA_USE_GLFW
    m_sharedRes->winHandle->setBlockMouseIconSwitch(false);
#endif
    data->consumed = true;
}

void WindowResizeArea::mouseIn(hidData* data) {
#ifdef ARA_USE_GLFW
    auto m_win = m_sharedRes->winHandle;

    switch (m_type) {
        case AreaType::top: m_win->setMouseCursor(WinMouseIcon::vresize); break;
        case AreaType::left: m_win->setMouseCursor(WinMouseIcon::hresize); break;
        case AreaType::bottom: m_win->setMouseCursor(WinMouseIcon::vresize); break;
        case AreaType::right: m_win->setMouseCursor(WinMouseIcon::hresize); break;
        case AreaType::topLeft: m_win->setMouseCursor(WinMouseIcon::lbtrResize); break;
        case AreaType::topRight: m_win->setMouseCursor(WinMouseIcon::ltbrResize); break;
        case AreaType::bottomLeft: m_win->setMouseCursor(WinMouseIcon::ltbrResize); break;
        case AreaType::bottomRight: m_win->setMouseCursor(WinMouseIcon::lbtrResize); break;
    }

        // m_sharedRes->winHandle->setBlockMouseIconSwitch(false);
#endif
}

void WindowResizeArea::mouseOut(hidData* data) {
#ifdef ARA_USE_GLFW
    m_sharedRes->winHandle->setMouseCursor(WinMouseIcon::arrow);
    // m_sharedRes->winHandle->setBlockMouseIconSwitch(false);
#endif
}

void WindowResizeArea::mouseDrag(hidData* data) {
#ifdef ARA_USE_GLFW
    ivec2 pixOffs{0, 0};
    ivec2 newPos{0, 0};
    ivec2 newSize{0, 0};

    auto absMousePos = m_sharedRes->winHandle->getAbsMousePos();

    if (data->dragStart) {
        m_dragStartWinPos  = m_sharedRes->winHandle->getPosition();
        m_dragStartWinSize = m_sharedRes->winHandle->getSize();
        m_sharedRes->winHandle->setBlockMouseIconSwitch(true);
        m_mouseDownPixelPos = absMousePos;

    } else {
        pixOffs = absMousePos - m_mouseDownPixelPos;
        switch (m_type) {
            case AreaType::top:
                newPos  = glm::ivec2(m_dragStartWinPos.x, m_dragStartWinPos.y + pixOffs.y);
                newSize = glm::ivec2(m_dragStartWinSize.x, m_dragStartWinSize.y - pixOffs.y);
                break;
            case AreaType::bottom:
                newPos  = glm::ivec2(m_dragStartWinPos.x, m_dragStartWinPos.y);
                newSize = glm::ivec2(m_dragStartWinSize.x, m_dragStartWinSize.y + pixOffs.y);
                break;
            case AreaType::left:
                newPos  = glm::ivec2(m_dragStartWinPos.x + pixOffs.x, m_dragStartWinPos.y);
                newSize = glm::ivec2(m_dragStartWinSize.x - pixOffs.x, m_dragStartWinSize.y);
                break;
            case AreaType::right:
                newPos  = glm::ivec2(m_dragStartWinPos.x, m_dragStartWinPos.y);
                newSize = glm::ivec2(m_dragStartWinSize.x + pixOffs.x, m_dragStartWinSize.y);
                break;
            case AreaType::topLeft:
                newPos  = glm::ivec2(m_dragStartWinPos.x + pixOffs.x, m_dragStartWinPos.y + pixOffs.y);
                newSize = glm::ivec2(m_dragStartWinSize.x - pixOffs.x, m_dragStartWinSize.y - pixOffs.y);
                break;
            case AreaType::topRight:
                newPos  = glm::ivec2(m_dragStartWinPos.x, m_dragStartWinPos.y + pixOffs.y);
                newSize = glm::ivec2(m_dragStartWinSize.x + pixOffs.x, m_dragStartWinSize.y - pixOffs.y);
                break;
            case AreaType::bottomRight:
                newPos  = glm::ivec2(m_dragStartWinPos.x, m_dragStartWinPos.y);
                newSize = glm::ivec2(m_dragStartWinSize.x + pixOffs.x, m_dragStartWinSize.y + pixOffs.y);
                break;
            case AreaType::bottomLeft:
                newPos  = glm::ivec2(m_dragStartWinPos.x + pixOffs.x, m_dragStartWinPos.y);
                newSize = glm::ivec2(m_dragStartWinSize.x - pixOffs.x, m_dragStartWinSize.y + pixOffs.y);
                break;
        }

        // limit newSize
        if (m_sharedRes->minWinSize) {
            newSize.x = std::max<int>(newSize.x, m_sharedRes->minWinSize->x);
            newSize.y = std::max<int>(newSize.y, m_sharedRes->minWinSize->y);
        }

        static_cast<UIWindow *>(m_sharedRes->win)->addWinCb([this, newPos, newSize]() {
            // the effect of winHandle->setSize is immediate, so there will be a visual artefact, since the draw buffer
            // will not be updated immediately ... most applications ignore this artefact...

            // winHandle->setSize() this will call GLFWWindow::onWindowResize which will call UIWindow::window_size_callback
            m_sharedRes->winHandle->setSize(newSize.x, newSize.y);
            m_sharedRes->winHandle->setPosition(newPos.x, newPos.y);

            return true;
        });
    }

#endif
    data->consumed = true;
}

}  // namespace ara