//
// Created by sven on 21-05-25.
//

#pragma once

#include <iostream>

#ifdef __linux__
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#elif _WIN32
#include <crtdbg.h>
#include <windows.h>
#elif __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#endif

namespace ara {

#ifdef __linux__
    Display* getStdDisplay() {
        Display* display = XOpenDisplay(nullptr);
        if (!display) {
            fprintf(stderr, "Failed to open display\n");
            return nullptr;
        }
        return display;
    }

    void SimulateKeyPress(unsigned char key) {
        auto display = getStdDisplay();
        if (display) {
            KeyCode keycode = XKeysymToKeycode(display, XK_B);
            XTestFakeKeyEvent(display, keycode, True, 0);
            XTestFakeKeyEvent(display, keycode, False, 0);
            XSync(display, false);
            XCloseDisplay(display);
        }
    }

    void SimulateMouseButtonClick(int x, int y, int button) {
        auto display = getStdDisplay();
        if (display) {
            int screen_number = DefaultScreen(display);
            XTestFakeMotionEvent(display, screen_number, x, y, 0);
            XSync(display, false);

            XTestFakeButtonEvent(display, button, True, 0); // Mouse button press
            XTestFakeButtonEvent(display, button, False, 0); // Mouse button release
            XSync(display, false);
        }
    }

    void SimulateMouseMovement(int x, int y, int x2, int y2) {
        auto display = getStdDisplay();
        if (display) {
            int screen_number = DefaultScreen(display);
            XTestFakeMotionEvent(display, screen_number, x, y, 0);
            XSync(display, false);

            XTestFakeMotionEvent(display, screen_number, x2, y2, 0);
            XSync(display, false);
        }
    }

    void SimulateWheel(void* ctx, int delta) {
        Window window = glfwGetX11Window(reinterpret_cast<GLFWwindow *>(ctx));
        auto display = glfwGetX11Display();

        XButtonEvent event;
        event.type = ButtonPress;
        event.display = display;
        event.window = window;
        event.root = DefaultRootWindow(display);
        event.subwindow = None;
        event.time = CurrentTime;
        event.x_root = 0; // Mouse position (root coordinates)
        event.y_root = 0; // Mouse position (root coordinates)
        event.x = 0;      // Mouse position (window coordinates)
        event.y = 0;      // Mouse position (window coordinates)
        event.state = 0;
        event.same_screen = True;

        if (delta > 0) {
            // Scroll up
            event.button = Button4; // Wheel scroll up button
        } else if (delta < 0) {
            // Scroll down
            event.button = Button5; // Wheel scroll down button
        } else {
            return; // No scroll action
        }

        XSendEvent(display, window, True, ButtonPressMask | ButtonReleaseMask, (XEvent*)&event);
        XFlush(display);

        event.type = ButtonRelease;
        XSendEvent(display, window, True, ButtonPressMask | ButtonReleaseMask, (XEvent*)&event);
        XFlush(display);
    }

    void CloseWindow(void* ctx) {
        Window window = glfwGetX11Window(reinterpret_cast<GLFWwindow*>(ctx));
        auto display = glfwGetX11Display();

        XEvent event;
        event.xclient.type = ClientMessage;
        event.xclient.window = window; // window is the ID of the window to close
        event.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", True);
        event.xclient.format = 32;
        event.xclient.data.l[0] = XInternAtom(display, "WM_DELETE_WINDOW", False);
        event.xclient.data.l[1] = CurrentTime;
        XSendEvent(display, window, False, NoEventMask, &event);
        XSync(display, False); // Ensure the event is sent immediately
    }

#elif _WIN32

    void SimulateKeyPress(unsigned char virtualKey) {
        keybd_event(virtualKey, MapVirtualKey(virtualKey, 0), 0, 0);
        keybd_event(virtualKey, MapVirtualKey(virtualKey, 0), KEYEVENTF_KEYUP, 0);
    }

#elif __APPLE__

    void SimulateKeyPress(CGKeyCode keyCode) {
        CGEventRef keyDown = CGEventCreateKeyboardEvent(NULL, keyCode, true);
        CGEventPost(kCGHIDEventTap, keyDown);
        CFRelease(keyDown);

        CGEventRef keyUp = CGEventCreateKeyboardEvent(NULL, keyCode, false);
        CGEventPost(kCGHIDEventTap, keyUp);
        CFRelease(keyUp);
    }
#endif

}
