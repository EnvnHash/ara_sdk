//
// Created by sven on 29-04-26.
//

#pragma once

#include <UIElements/Image.h>
#include <UIElements/ResizableHandle.h>

namespace ara {

class Resizable : public Div {
public:
    struct dragPar {
        const glm::vec2& movedPix{};
        const glm::ivec2& minSize{};
        const glm::vec2& dragStartPos{};
        const glm::vec2& dragStartSize{};
        const glm::vec2& newSize{};
    };

    Resizable();

    void init() override;
    void initHandles();
    void setResizeStart(ResizableHandle::Corner corner);
    void setResizeEnd();
    void setFixedAspect(const bool& val);
    void setMinSize(const glm::vec2& size) { m_minSize = size; }

private:
    void resizeFromCorner(ResizableHandle::Corner corner, const glm::vec2& movedPix);
    glm::ivec2 getNewSize(ResizableHandle::Corner corner, const glm::vec2& movedPix) const;
    glm::ivec2 getNewPos(ResizableHandle::Corner corner, const glm::vec2& movedPix, const glm::vec2& newSize) const;
    static int32_t getTopDragY(const dragPar& dp);
    static int32_t getLeftDragX(const dragPar& dp);

    ResizableHandle::Corner m_dragCorner = ResizableHandle::Corner::bottomRight;
    glm::vec2               m_dragStartMouseParentRel{};
    glm::vec2               m_dragStartPos{};
    glm::vec2               m_dragStartSize{};
    float                   m_dragStartAspect{};
    glm::ivec2              m_minSize = {40, 40 };
    int32_t                 m_handleSize = 12;
    bool                    m_dragging = false;
    bool                    m_fixedAspect = false;

    std::array<ResizableHandle*, static_cast<int>(ResizableHandle::Corner::size)> m_handles{};

};

} // namespace hfc