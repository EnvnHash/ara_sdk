#include "ScrollBar.h"
#include "ScrollView.h"
#include "UIApplication.h"



using namespace glm;
using namespace std;

namespace ara {

ScrollBar::ScrollBar() : Div() {
    setName(getTypeName<ScrollBar>());
    setFocusAllowed(false);
    m_canReceiveDrag = true;
}

void ScrollBar::init() {
    m_dragArea = addChild<UIScrollBarDragArea>();
    m_dragArea->setBackgroundColor(m_color);
    m_dragArea->setSize(1.f, 1.f);
    m_dragArea->setName(getTypeName<UIScrollBarDragArea>());
}

void ScrollBar::updateMatrix() {
    UINode::updateMatrix();

    // calculate DragArea

    // get the scroll view
    auto sv = dynamic_cast<ScrollView *>(m_parent);
    if (!sv || !m_dragArea) {
        return;
    }

    // calculate the size of the drag area
    m_bbSize = sv->getBBSize();  // is in absolute coordinates top/left x,y
                                 // right/bottom x,y => vec4(x,y,z,w)

    // set size of drag area in percentage (parentBound / childrenBound)
    m_dragAreaSizeRel   = sv->getSize() / m_bbSize;
    m_dragAreaSizePix.x = sv->m_HSB->getSize().x * m_dragAreaSizeRel.x;
    m_dragAreaSizePix.y = sv->m_VSB->getSize().y * m_dragAreaSizeRel.y;

    if (s_Type == horizontal) {
        m_dragArea->setSize(m_dragAreaSizeRel.x, 1.f);
    } else {
        m_dragArea->setSize(1.f, m_dragAreaSizeRel.y);
    }

    // calculate the position of the scrollbars in relation to the content offset

    // maximum distance in pixels, the area the dragArea can be dragged
    m_maxScrollBarDragWay.x = sv->m_HSB->getSize().x - m_dragAreaSizePix.x;
    m_maxScrollBarDragWay.y = sv->m_VSB->getSize().y - m_dragAreaSizePix.y;

    // the maximum offset the content can have inside the scrollview
    m_maxScrollOffset = m_bbSize - sv->getSize();
    m_scrollOffset    = sv->getContentTransTransl();

    if (s_Type == horizontal) {
        m_dragArea->setX(static_cast<int>(-m_scrollOffset.x / m_maxScrollOffset.x * m_maxScrollBarDragWay.x));
    } else {
        m_dragArea->setY(static_cast<int>(-m_scrollOffset.y / m_maxScrollOffset.y * m_maxScrollBarDragWay.y));
    }
}

void ScrollBar::mouseDown(hidData& data) {
    if (!data.hit) {
        return;
    }

    if (m_dragArea) {
        // offset the scrollbar by its size
        auto sv = dynamic_cast<ScrollView *>(getParent());

        // was the click on the negative or positive side
        float isPositiveSide;
        if (s_Type == horizontal) {
            isPositiveSide = data.mousePosNodeRel.x > m_dragArea->getPos().x + m_dragArea->getSize().x ? -1.f : 1.f;
            sv->setScrollOffset(sv->getContentTransTransl().x + isPositiveSide * m_dragArea->getSize().x,
                                sv->getContentTransTransl().y);
        } else {
            isPositiveSide = data.mousePosNodeRel.y > m_dragArea->getPos().y + m_dragArea->getSize().y ? -1.f : 1.f;
            sv->setScrollOffset(sv->getContentTransTransl().x,
                                sv->getContentTransTransl().y + isPositiveSide * m_dragArea->getSize().y);
        }
        data.consumed = true;
    }
}

void UIScrollBarDragArea::mouseDrag(hidData& data) {
    // get the ScrollView, which is the parent of this parent
    const auto sv = dynamic_cast<ScrollView*>(getParent()->getParent());
    const auto sb = dynamic_cast<ScrollBar*>(getParent());

    if (data.dragStart) {
        m_startDragScrollViewTrans = sv->getContentTransTransl();
    }

    // relative pos
    m_relOffs = glm::max(glm::min((vec2)data.movedPix / sb->getMaxDragWay(), vec2{1.f}), vec2{-1.f});

    // add offset to the initial offset on dragStart
    if (dynamic_cast<ScrollBar *>(getParent())->s_Type == ScrollBar::e_stype::horizontal) {
        sv->setScrollOffset(-m_relOffs.x * sb->getMaxScrollOffset().x + m_startDragScrollViewTrans.x,
                            sv->getContentTransTransl().y);
    } else {
        sv->setScrollOffset(sv->getContentTransTransl().x,
                            -m_relOffs.y * sb->getMaxScrollOffset().y + m_startDragScrollViewTrans.y);
    }

    data.consumed = true;
}

}