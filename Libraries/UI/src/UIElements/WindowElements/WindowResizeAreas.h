//
// Created by user on 02.12.2020.
//

#pragma once

#include "WindowResizeArea.h"

namespace ara {

class WindowResizeAreas {
public:
#ifdef ARA_USE_GLFW
    static void addResizeAreas(UINode& root, GLFWWindow* win);
#endif
};

}  // namespace ara
