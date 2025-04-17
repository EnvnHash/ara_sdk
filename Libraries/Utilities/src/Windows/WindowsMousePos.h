//
// Created by user on 02.12.2020.
//

#pragma once

#ifdef _WIN32
#include <windows.h>

namespace ara::mouse {

static int getAbsMousePos(int &root_x, int &root_y) {
    POINT pos;
    if (GetCursorPos(&pos)) {
        root_x = (int)pos.x;
        root_y = (int)pos.y;
        return 0;
    }
    return 1;
}
}  // namespace ara::mouse
#endif
