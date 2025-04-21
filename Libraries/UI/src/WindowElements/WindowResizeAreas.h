//
// Created by user on 02.12.2020.
//

#pragma once

#include "WindowResizeArea.h"
#include "WindowManagement/GLFWWindow.h"

#include "Button/ImageButton.h"

namespace ara {

class WindowResizeAreas : public UINode {
public:
    WindowResizeAreas();
    virtual ~WindowResizeAreas() = default;

    void init();
    bool draw(uint32_t* objId) { return false; }

private:
    std::vector<WindowResizeArea*> m_winResizeAreas;
};

}  // namespace ara
