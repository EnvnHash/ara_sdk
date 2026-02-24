//
// Created by user on 02.12.2020.
//

#pragma once

#include "WindowResizeArea.h"

namespace ara {

class WindowResizeAreas {
public:
    static void addResizeAreas(UINode& root, GLFWWindow* win);
};

}  // namespace ara
