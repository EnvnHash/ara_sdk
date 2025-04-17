//
// Created by user on 02.12.2020.
//

#pragma once

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>

namespace ara::mouse {

static int getAbsMousePos(int &root_x, int &root_y) {
    CGEventRef event  = CGEventCreate(NULL);
    CGPoint    cursor = CGEventGetLocation(event);
    root_x            = (int)cursor.x;
    root_y            = (int)cursor.y;
    CFRelease(event);
    return 1;
}
}  // namespace ara::mouse
#endif
