//
// Created by sven on 29-04-26.
//

#include "Resizable.h"

#include "UISharedRes.h"
#include "UIWindow.h"

using namespace ara;
using namespace glm;
using namespace std;

using Corner = ResizableHandle::Corner;

namespace ara {

Resizable::Resizable() {
    setName(ara::getTypeName<Resizable>());
    setTypeName<Resizable>();
    setFocusAllowed(true);
    setScissorChildren(false);
}

void Resizable::init() {
    initHandles();

    getWindow()->addGlobalMouseMoveCb(this, [this](const hidData& data) {
        if (!getWindow()->isCursorVisible() || !m_visible || !m_dragging) {
            return;
        }
        resizeFromCorner(m_dragCorner, data.movedPix);
        getSharedRes()->reqRedraw();
    });

    getWindow()->addGlobalMouseUpLeftCb(this, [this](const hidData& data) {
        m_dragging = false;
    });
}

void Resizable::initHandles() {
    array positions {
        ivec2{-m_handleSize/2, -m_handleSize/2}, ivec2{m_handleSize/2, -m_handleSize/2},
        ivec2{m_handleSize/2, m_handleSize/2}, ivec2{-m_handleSize/2, m_handleSize/2},
        ivec2{0, -m_handleSize * 0.35f}, ivec2{m_handleSize * 0.35f, 0},
        ivec2{0, m_handleSize * 0.35f}, ivec2{-m_handleSize * 0.35f, 0},
        ivec2{0, 0}
    };

    array sizes {
        ivec2{m_handleSize, m_handleSize}, ivec2{m_handleSize, m_handleSize},
        ivec2{m_handleSize, m_handleSize}, ivec2{m_handleSize, m_handleSize},
        ivec2{m_handleSize*2, m_handleSize * 0.7f}, ivec2{m_handleSize * 0.7f, m_handleSize*2},
        ivec2{m_handleSize*2, m_handleSize * 0.7f}, ivec2{m_handleSize * 0.7f, m_handleSize*2},
        ivec2{m_handleSize*2, m_handleSize*2}
    };

    array aligns {  align::left, align::right, align::right, align::left,
                    align::center, align::right, align::center, align::left, align::center };
    array vAligns { valign::top, valign::top, valign::bottom, valign::bottom,
                    valign::top, valign::center, valign::bottom, valign::center, valign::center };

    for (int32_t i=0; i<m_handles.size(); ++i) {
        m_handles[i] = &push<ResizableHandle>(UINodePars{
            .pos = positions[i],
            .size = sizes[i],
            .bgColor = vec4{0.4f, 0.4f, 0.4f, 0.9f},
            .align = aligns[i],
            .valign = vAligns[i],
            .borderWidth = 1.f,
            .borderColor = vec4{1.f, 1.f, 1.f, 1.f}
        });
        m_handles[i]->setCorner(static_cast<Corner>(i));
        m_handles[i]->setResizeImage(this);
        if (i == static_cast<int32_t>(Corner::center)) {
            m_handles[i]->setAlpha(0.f);
        }
        m_handles[i]->setResizeImage(this);
    }
}

void Resizable::setResizeStart(const Corner corner) {
    m_dragStartPos = getPos();
    m_dragStartSize = getSize();
    m_dragCorner = corner;
    m_dragging = true;
}

void Resizable::setResizeEnd() {
    m_dragging = false;
}

void Resizable::resizeFromCorner(const Corner corner, const vec2& movedPix) {
    if (corner == Corner::bottomRight) {
        setSize(max(m_minSize, ivec2(m_dragStartSize + movedPix)));
    } else if (corner == Corner::topLeft) {
        const auto newSize = ivec2(m_dragStartSize - movedPix);
        setPos(min(ivec2(m_dragStartPos + movedPix), ivec2(m_dragStartPos  + m_dragStartSize - vec2(m_minSize))));
        setSize(max(m_minSize, newSize));
    } else if (corner == Corner::bottomLeft) {
        const auto newSize = ivec2{ m_dragStartSize.x - movedPix.x, m_dragStartSize.y + movedPix.y };
        setPos(getLeftDragX(movedPix.x), static_cast<int32_t>(m_dragStartPos.y));
        setSize(max(m_minSize, newSize));
    } else if (corner == Corner::topRight) {
        const auto newSize = ivec2{ m_dragStartSize.x + movedPix.x, m_dragStartSize.y - movedPix.y };
        setPos(static_cast<int32_t>(m_dragStartPos.x), getTopDragY(movedPix.y));
        setSize(max(m_minSize, newSize));
    } else if (corner == Corner::top) {
        setSize(max(m_minSize, ivec2(m_dragStartSize.x, m_dragStartSize.y - movedPix.y)));
        setPos(static_cast<int32_t>(m_dragStartPos.x), getTopDragY(movedPix.y));
    } else if (corner == Corner::bottom) {
        setSize(max(m_minSize, ivec2(m_dragStartSize.x, m_dragStartSize.y + movedPix.y)));
    }  else if (corner == Corner::right) {
        setSize(max(m_minSize, ivec2(m_dragStartSize.x + movedPix.x, m_dragStartSize.y)));
    } else if (corner == Corner::left) {
        setPos(getLeftDragX(movedPix.x), static_cast<int32_t>(m_dragStartPos.y));
        setSize(max(m_minSize, ivec2(m_dragStartSize.x - movedPix.x, m_dragStartSize.y)));
    } else if (corner == Corner::center) {
        setPos(ivec2(m_dragStartPos + movedPix));
    }

    if (getSharedRes()) {
        getSharedRes()->reqRedraw();
    }
}

int32_t Resizable::getTopDragY(const int32_t moved) const {
    return std::min(static_cast<int32_t>(m_dragStartPos.y + moved),
                      static_cast<int32_t>(m_dragStartPos.y + m_dragStartSize.y) - m_minSize.y);
}

int32_t Resizable::getLeftDragX(const int32_t moved) const {
    return std::min(static_cast<int32_t>(m_dragStartPos.x + moved),
                            static_cast<int32_t>(m_dragStartPos.x + m_dragStartSize.x) - m_minSize.x);
}

}