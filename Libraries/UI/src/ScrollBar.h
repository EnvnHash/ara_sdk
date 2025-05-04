#pragma once

#include "Div.h"

namespace ara {

class UIScrollBarDragArea : public Div {
public:
    UIScrollBarDragArea() : Div() {
        setName(getTypeName<UIScrollBarDragArea>());
        m_canReceiveDrag = true;
    }
    ~UIScrollBarDragArea() override = default;
    void mouseDrag(hidData* data) override;

private:
    glm::vec2 m_startDragScrollViewTrans{0.f};
    glm::vec2 m_relOffs{0.f};
};

class ScrollBar : public Div {
public:
    enum e_stype { vertical = 0, horizontal = 1 };
    enum e_marea { none = 0, pgup, thumb, pgdown };

    ScrollBar();

    void init() override;
    void updateMatrix() override;
    void mouseDown(hidData* data) override;

    void setType(ScrollBar::e_stype type) { s_Type = type; }
    void setScrollOffset(float offsX, float offsY) {
        m_scrollOffset.x = offsX;
        m_scrollOffset.y = offsY;
    }

    [[nodiscard]] UIScrollBarDragArea* getDragArea() const { return m_dragArea; }
    [[nodiscard]] glm::vec2            getMaxDragWay() const { return m_maxScrollBarDragWay; }
    [[nodiscard]] glm::vec2            getMaxScrollOffset() const { return m_maxScrollOffset; }

    e_stype s_Type = vertical;

protected:
    UIScrollBarDragArea* m_dragArea = nullptr;

    glm::vec2 m_scrollOffset{0.f};     // in pixel, means the x,y offset of the content
    glm::vec2 m_maxScrollOffset{0.f};  // in pixel, means the maximum x,y offset of the content
    glm::vec2 m_maxScrollBarDragWay{0.f};
    glm::vec2 m_bbSize{0.f};
    glm::vec2 m_dragAreaSizeRel{0.f};
    glm::vec2 m_dragAreaSizePix{0.f};
};

// just a couple of small classes to help set up easier:

class UIVScrollBar : public ScrollBar {
public:
    UIVScrollBar() {
        setName(getTypeName<UIVScrollBar>());
        s_Type = vertical;
    }
};

class UIHScrollBar : public ScrollBar {
public:
    UIHScrollBar() {
        setName(getTypeName<UIHScrollBar>());
        s_Type = horizontal;
    }
};

}  // namespace ara
