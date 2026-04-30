//
// Created by sven on 30-04-26.
//

#pragma once

#include <UIElements/Div.h>

namespace ara {

class Resizable;

class ResizableHandle : public Div {
public:
    enum class Corner {
        topLeft = 0,
        topRight,
        bottomRight,
        bottomLeft,
        top,
        right,
        bottom,
        left,
        center,
        size
    };

    ResizableHandle();

    void mouseIn(hidData& data) override;
    void mouseOut(hidData& data) override;
    void mouseDown(hidData& data) override;
    void mouseUp(hidData& data) override;

    void setResizeImage(Resizable* resizeImage) { m_resizeImage = resizeImage; }
    void setCorner(const Corner& corner) { m_corner = corner; }
    const Corner& getCorner() const { return m_corner; }

private:
    Corner      m_corner{};
    Resizable*  m_resizeImage{};
    glm::vec2   m_initRISize{};
};

}