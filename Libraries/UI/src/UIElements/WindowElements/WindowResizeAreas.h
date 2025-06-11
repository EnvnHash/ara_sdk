//
// Created by user on 02.12.2020.
//

#pragma once

#include "WindowResizeArea.h"

namespace ara {

class WindowResizeAreas : public UINode {
public:
    WindowResizeAreas();
    ~WindowResizeAreas() override = default;

    void init() override;
    bool draw(uint32_t& objId) override { return false; }

private:
    std::vector<WindowResizeArea*> m_winResizeAreas;
};

}  // namespace ara
