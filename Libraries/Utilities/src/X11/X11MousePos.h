//
// Created by sven on 11/28/20.
//

#ifdef __linux__

#pragma once

#ifndef __ANDROID__
#include <X11/Xlib.h>
#endif

#include <cassert>
#include <malloc.h>
#include <cstdio>
#include <unistd.h>

namespace ara::mouse {

#ifndef __ANDROID__
static int _XlibErrorHandler(Display *display, XErrorEvent *event) {
    fprintf(stderr, "An error occured detecting the mouse position\n");
    return True;
}

static Display *display = nullptr;
static int      numOfScreens;
static Window  *root_windows;
#endif

static int getAbsMousePos(int &root_x, int &root_y) {
#ifndef __ANDROID__
    int    i;
    Bool   result;
    Window window_returned;

    int          win_x, win_y;
    unsigned int mask_return;

    if (!display) {
        display = XOpenDisplay(NULL);
        assert(display);
        XSetErrorHandler(_XlibErrorHandler);
        numOfScreens = XScreenCount(display);
        root_windows = (Window *)malloc(sizeof(Window) * numOfScreens);
        for (i = 0; i < numOfScreens; i++) {
            root_windows[i] = XRootWindow(display, i);
        }
    }

    for (i = 0; i < numOfScreens; i++) {
        result = XQueryPointer(display, root_windows[i], &window_returned, &window_returned, &root_x, &root_y, &win_x,
                               &win_y, &mask_return);
        if (result == True) {
            break;
        }
    }
    if (result != True) {
        fprintf(stderr, "No mouse found.\n");
        return -1;
    }

    // free(root_windows);
    // XCloseDisplay(display);
#endif
    return 0;
}

}  // namespace ara::mouse

#endif