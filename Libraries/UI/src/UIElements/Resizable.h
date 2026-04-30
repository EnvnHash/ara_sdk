//
// Created by sven on 29-04-26.
//

#pragma once

#include <UIElements/Image.h>
#include <UIElements/ResizableHandle.h>

namespace ara {

class Resizable : public Div {
public:
    Resizable();

    void init() override;
    void initHandles();
    void setResizeStart(ResizableHandle::Corner corner);
    void setResizeEnd();
    void resizeFromCorner(ResizableHandle::Corner corner, const glm::vec2& movedPix);

    void setMinSize(const glm::vec2& size) { m_minSize = size; }

private:
    int32_t getTopDragY(int32_t moved) const;
    int32_t getLeftDragX(int32_t moved) const;

    ResizableHandle::Corner m_dragCorner = ResizableHandle::Corner::bottomRight;
    glm::vec2               m_dragStartMouseParentRel{};
    glm::vec2               m_dragStartPos{};
    glm::vec2               m_dragStartSize{};
    glm::ivec2              m_minSize = {40, 40 };
    int32_t                 m_handleSize = 12;
    bool                    m_dragging = false;

    std::array<ResizableHandle*, static_cast<int>(ResizableHandle::Corner::size)> m_handles{};

};

} // namespace hfc