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
    setSnapToHwPix(true);
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
        getSharedRes()->winHandle->setMouseCursor(WinMouseIcon::arrow);
        m_dragging = false;
    });
}

void Resizable::initHandles() {
    const auto handleSize = static_cast<float>(m_handleSize);
    array positions {
        ivec2{-m_handleSize/2, -m_handleSize/2}, ivec2{m_handleSize/2, -m_handleSize/2},
        ivec2{m_handleSize/2, m_handleSize/2}, ivec2{-m_handleSize/2, m_handleSize/2},
        ivec2{0, -handleSize * 0.35f}, ivec2{handleSize * 0.35f, 0},
        ivec2{0, handleSize * 0.35f}, ivec2{-handleSize * 0.35f, 0},
        ivec2{0, 0}
    };

    array sizes {
        ivec2{m_handleSize, m_handleSize}, ivec2{m_handleSize, m_handleSize},
        ivec2{m_handleSize, m_handleSize}, ivec2{m_handleSize, m_handleSize},
        ivec2{m_handleSize*2, handleSize * 0.7f}, ivec2{handleSize * 0.7f, m_handleSize*2},
        ivec2{m_handleSize*2, handleSize * 0.7f}, ivec2{handleSize * 0.7f, m_handleSize*2},
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
            .bgColor = vec4{0.4f, 0.4f, 0.4f, 1.f},
            .align = aligns[i],
            .valign = vAligns[i],
            .borderWidth = 1.f,
            .borderColor = vec4{1.f, 1.f, 1.f, 1.f}
        });
        m_handles[i]->setCorner(static_cast<Corner>(i));
        m_handles[i]->setResizeImage(this);
        m_handles[i]->excludeFromParentPaddingAndBorder(true);
        m_handles[i]->setSnapToHwPix(true);
        m_handles[i]->setVisibility(m_handleVisibility);
    }

    setFixedAspect(m_fixedAspect);
}

void Resizable::setResizeStart(const Corner corner) {
    m_dragStartPos = getAlignedPos();
    m_dragStartSize = getSize();
    m_dragStartAspect = m_dragStartSize.x / m_dragStartSize.y;
    m_dragCorner = corner;
    m_dragging = true;
}

void Resizable::setResizeEnd() {
    m_dragging = false;
}

void Resizable::setFixedAspect(const bool& val) {
    m_fixedAspect = val;
    if (m_inited) {
        for (auto& it : { Corner::left, Corner::right, Corner::top, Corner::bottom }) {
            m_handles[static_cast<int32_t>(it)]->setVisibility(!val);
        }
    }
}

void Resizable::setHandleVisibility(const bool val) {
    for (const auto& it: m_handles) {
        if (it) {
            it->setVisibility(val);
        }
    }
    m_handleVisibility = val;
}

void Resizable::resizeFromCorner(const Corner corner, const vec2& movedPix) {
    ivec2 newSize = getNewSize(corner, movedPix);
    if (m_fixedAspect) {
        newSize.x = newSize.y * m_dragStartAspect;
    }

    setSize(newSize);
    setPos(getNewPos(corner, movedPix, newSize));

    if (getSharedRes()) {
        getSharedRes()->reqRedraw();
    }
}

ivec2 Resizable::getNewSize(const Corner corner, const vec2& movedPix) const {
    static unordered_map<Corner, function<ivec2(const dragPar&)>> funcMap{
        {Corner::bottomRight, [](const dragPar& dp) {
            return max(dp.minSize, ivec2((dp.dragStartSize + dp.movedPix)));
        }}, {Corner::topLeft, [](const dragPar& dp) {
            return max(dp.minSize, ivec2(dp.dragStartSize - dp.movedPix));
        }}, {Corner::bottomLeft, [](const dragPar& dp) {
            return max(dp.minSize, ivec2{dp.dragStartSize.x - dp.movedPix.x, dp.dragStartSize.y + dp.movedPix.y});
        }}, {Corner::topRight, [](const dragPar& dp) {
            return max(dp.minSize, ivec2{dp.dragStartSize.x + dp.movedPix.x, dp.dragStartSize.y - dp.movedPix.y});
        }}, {Corner::top, [](const dragPar& dp) {
            return max(dp.minSize, ivec2(dp.dragStartSize.x, dp.dragStartSize.y - dp.movedPix.y));
        }}, {Corner::bottom, [](const dragPar& dp) {
            return max(dp.minSize, ivec2(dp.dragStartSize.x, dp.dragStartSize.y + dp.movedPix.y));
        }}, {Corner::right, [](const dragPar& dp) {
            return max(dp.minSize, ivec2(dp.dragStartSize.x + dp.movedPix.x, dp.dragStartSize.y));
        }}, {Corner::left, [](const dragPar& dp) {
            return max(dp.minSize, ivec2(dp.dragStartSize.x - dp.movedPix.x, dp.dragStartSize.y));
        }}, {Corner::center, [](const dragPar& dp) {
            return dp.dragStartSize;
        }}
    };

    const dragPar dp{movedPix, m_minSize, m_dragStartPos, m_dragStartSize, {}};
    return funcMap[corner](dp);
}

ivec2 Resizable::getNewPos(const Corner corner, const vec2& movedPix, const vec2& newSize) const {
    static unordered_map<Corner, function<ivec2(const dragPar&)>> funcMap{
        {Corner::bottomRight, [](const dragPar& dp) {
            return ivec2{ getNewXPosRightHandle(dp), getNewYPosBottomHandle(dp) };
        }}, {Corner::topLeft, [](const dragPar& dp) {
            return ivec2{getNewXPosLeftHandle(dp), getNewYPosTopHandle(dp) };
        }}, {Corner::bottomLeft, [](const dragPar& dp) {
            return ivec2{ getNewXPosLeftHandle(dp), getNewYPosBottomHandle(dp) };
        }}, {Corner::topRight, [](const dragPar& dp) {
            return ivec2{ getNewXPosRightHandle(dp), getNewYPosTopHandle(dp) };
        }}, {Corner::top, [](const dragPar& dp) {
            return ivec2{ getNewXPosCenterHandle(dp), getNewYPosTopHandle(dp) };
        }}, {Corner::bottom, [](const dragPar& dp) {
            return ivec2{ getNewXPosCenterHandle(dp), getNewYPosBottomHandle(dp) };
        }}, {Corner::right, [](const dragPar& dp) {
            return ivec2{ getNewXPosRightHandle(dp), getNewYPosCenterHandle(dp) };
        }}, {Corner::left, [](const dragPar& dp) {
            return ivec2{ getNewXPosLeftHandle(dp), getNewYPosCenterHandle(dp) };
        }}, {Corner::center, [](const dragPar& dp) {
            return dp.dragStartPos + dp.movedPix;
        }}
    };

    const dragPar dp{movedPix, m_minSize, m_dragStartPos, m_dragStartSize, newSize, m_alignX, m_alignY };
    return funcMap[corner](dp);
}

int32_t Resizable::getNewXPosCenterHandle(const dragPar& dp) {
    return dp.dragStartPos.x;
}

int32_t Resizable::getNewXPosLeftHandle(const dragPar& dp) {
    const float pivotFac = dp.alignX == align::right ? 1.f : dp.alignX == align::center ? 0.5f : 0.f;
    return static_cast<int32_t>(std::round(dp.dragStartPos.x + (1.f - pivotFac) * (dp.dragStartSize.x - dp.newSize.x)));
}

int32_t Resizable::getNewXPosRightHandle(const dragPar& dp) {
    const float pivotFac = dp.alignX == align::right ? 1.f : dp.alignX == align::center ? 0.5f : 0.f;
    return static_cast<int32_t>(std::round(dp.dragStartPos.x + pivotFac * (dp.newSize.x - dp.dragStartSize.x)));
}

int32_t Resizable::getNewYPosCenterHandle(const dragPar& dp) {
    return dp.dragStartPos.y;
}

int32_t Resizable::getNewYPosTopHandle(const dragPar& dp) {
    const float pivotFac = dp.alignY == valign::bottom ? 1.f : dp.alignY == valign::center ? 0.5f : 0.f;
    return static_cast<int32_t>(std::round(dp.dragStartPos.y + (1.f - pivotFac) * (dp.dragStartSize.y - dp.newSize.y)));
}

int32_t Resizable::getNewYPosBottomHandle(const dragPar& dp) {
    const float pivotFac = dp.alignY == valign::bottom ? 1.f : dp.alignY == valign::center ? 0.5f : 0.f;
    return static_cast<int32_t>(std::round(dp.dragStartPos.y + pivotFac * (dp.newSize.y - dp.dragStartSize.y)));
}

int32_t Resizable::getTopDragY(const dragPar& dp) {
    return std::min(static_cast<int32_t>(dp.dragStartPos.y + dp.movedPix.y),
                      static_cast<int32_t>(dp.dragStartPos.y + dp.dragStartSize.y) - dp.minSize.y);
}

int32_t Resizable::getLeftDragX(const dragPar& dp) {
    return std::min(static_cast<int32_t>(dp.dragStartPos.x + dp.movedPix.x),
                            static_cast<int32_t>(dp.dragStartPos.x + dp.dragStartSize.x) - dp.minSize.x);
}

Resizable::~Resizable() {
    getWindow()->removeGlobalMouseMoveCb(this);
    getWindow()->removeGlobalMouseUpLeftCb(this);
}

}