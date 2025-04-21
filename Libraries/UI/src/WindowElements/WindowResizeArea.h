//
// Created by user on 05.10.2020.
//

#pragma once

#include "Div.h"

namespace ara {

class WindowResizeArea : public Div {
public:
    enum class AreaType : int { top = 0, left, bottom, right, topLeft, topRight, bottomLeft, bottomRight };
    WindowResizeArea();
    virtual ~WindowResizeArea() = default;

    void init() override;
    void updateDrawData() override;

    void mouseUp(hidData* data) override;
    void mouseDrag(hidData* data) override;
    void mouseIn(hidData* data) override;
    void mouseOut(hidData* data) override;

    void setAreaType(AreaType t) { m_type = t; }

private:
    AreaType   m_type = AreaType::top;
    glm::ivec2 m_dragStartWinPos;
    glm::ivec2 m_dragStartWinSize;
    glm::ivec2 m_mouseDownPixelPos;
};

}  // namespace ara
