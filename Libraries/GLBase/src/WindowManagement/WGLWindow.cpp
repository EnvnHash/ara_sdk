//
// Created by user on 23.09.2020.
//

#include <string_utils.h>
#ifdef WGLWINDOW
#ifdef _WIN32

#include "WGLWindow.h"

namespace ara {

/*	This Code Creates Our OpenGL Window.  Parameters Are:
 ** title			- Title To Appear At The Top Of The Window
 ** width			- Width Of The GL Window Or Fullscreen Mode
 ** height			- Height Of The GL Window Or Fullscreen Mode
 ** bits			- Number Of Bits To Use For Color (8/16/24/32)
 ** fullscreenflag	- Use Fullscreen Mode (true) Or Windowed Mode (false)
 */

bool WGLWindow::create(LPCWSTR title, uint32_t width, uint32_t height, uint32_t posX, uint32_t posY, uint32_t bits,
                       bool fullscreenflag, bool hidden, bool decorated, bool resizable, bool floating,
                       bool transparent, HGLRC shareCtx) {
    m_msgLoop = std::thread([this, title, width, height, posX, posY, bits, fullscreenflag, hidden, decorated, resizable,
                             floating, transparent, shareCtx]() {
        // createKeyTables();
        // loadLibraries();

        WNDCLASS wc;  // Windows Class Structure

        RECT WindowRect;  // Grabs Rectangle Upper Left / Lower Right Values
        WindowRect.left   = (long)posX;
        WindowRect.right  = (long)posX + width;
        WindowRect.top    = (long)posY;
        WindowRect.bottom = (long)posY + height;

        m_decorated   = decorated;
        m_resizable   = resizable;
        m_transparent = transparent;
        m_sharedCtx   = shareCtx;
        m_fullscreen  = fullscreenflag;  // Set The Global Fullscreen Flag

        m_hInstance      = GetModuleHandle(nullptr);               // Grab An Instance For Our Window
        wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;  // Redraw On Size, And Own DC For Window.
        wc.lpfnWndProc   = (WNDPROC)WndProc;                    // WndProc Handles Messages
        wc.cbClsExtra    = 0;                                   // No Extra Window Data
        wc.cbWndExtra    = 0;                                   // No Extra Window Data
        wc.hInstance     = m_hInstance;                         // Set The Instance
        wc.hIcon         = LoadIcon(nullptr, IDI_WINLOGO);         // Load The Default Icon
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);         // Load The Arrow Pointer
        wc.hbrBackground = nullptr;                                // No Background Required For GL
        wc.lpszMenuName  = nullptr;                                // We Don't Want A Menu
        m_thisClassName  = "OpenGL_" + std::to_string((uint64_t)this);
        wc.lpszClassName = StringToLPCWSTR(m_thisClassName);  // Set The Class Name

        if (!RegisterClass(&wc))  // Attempt To Register The Window Class
        {
            std::cerr << "Failed To Register The Window Class." << std::endl;
            return;  // Return false
        }

        if (m_fullscreen)  // Attempt Fullscreen Mode?
        {
            DEVMODE dmScreenSettings;  // Device Mode
            memset(&dmScreenSettings, 0,
                   sizeof(dmScreenSettings));                          // Makes Sure Memory's Cleared
            dmScreenSettings.dmSize       = sizeof(dmScreenSettings);  // Size Of The Devmode Structure
            dmScreenSettings.dmPelsWidth  = width;                     // Selected Screen Width
            dmScreenSettings.dmPelsHeight = height;                    // Selected Screen Height
            dmScreenSettings.dmBitsPerPel = bits;                      // Selected Bits Per Pixel
            dmScreenSettings.dmFields     = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

            // Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN
            // Gets Rid Of Start Bar.
            if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL) {
                // If The Mode Fails, Offer Two Options.  Quit Or Use Windowed
                // Mode.
                if (MessageBox(NULL,
                               LPCWSTR("The Requested Fullscreen Mode Is Not Supported "
                                   "By\nYour Video Card. Use Windowed Mode Instead?"),
                               LPCWSTR(" GL"), MB_YESNO | MB_ICONEXCLAMATION) == IDYES) {
                    m_fullscreen = false;  // Windowed Mode Selected.  Fullscreen = false
                } else {
                    // Pop Up A Message Box Letting User Know The Program Is Closing.
                    std::cerr << "WGLWindow: Program Will Now Close." << std::endl;
                    return;  // Return false
                }
            }
        }

        m_dwExStyle = WS_EX_APPWINDOW;
        if (m_fullscreen) {
            ShowCursor(false);
        }

        // else dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        // // Window Extended Style

        if (floating) {
            m_dwExStyle |= WS_EX_TOPMOST;
        }

        m_dwStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        // if (window->monitor) dwStyle |= WS_POPUP; else {
        m_dwStyle |= WS_SYSMENU | WS_MINIMIZEBOX;
        if (decorated) {
            m_dwStyle |= WS_CAPTION;
            if (resizable) m_dwStyle |= WS_MAXIMIZEBOX | WS_THICKFRAME;
        } else
            m_dwStyle |= WS_POPUP;
        // }

        AdjustWindowRectEx(&WindowRect, m_dwStyle, false,
                           m_dwExStyle);  // Adjust Window To true Requested Size

        // Create The Window
        if (!(m_hWnd = CreateWindowEx(m_dwExStyle,                                    // Extended Style For The Window
                                      reinterpret_cast<LPCWSTR>(m_thisClassName.c_str()),                        // Class Name
                                      title,                                          // Window Title
                                      m_dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,  // Required Window Style
                                      posX, posY,                                     // Window Position
                                      width, height,                                  // Window Size
                                      NULL,                                           // No Parent Window
                                      NULL,                                           // No Menu
                                      m_hInstance,                                    // Instance
                                      NULL)))                                         // Dont Pass Anything To WM_CREATE
        {
            destroy();  // Reset The Display
            std::cerr << "WGLWindow Error: Window Creation Error." << std::endl;
            return;  // Return false
        }

        static PIXELFORMATDESCRIPTOR pfd =  // pfd Tells Windows How We Want Things To Be
            {
                sizeof(PIXELFORMATDESCRIPTOR),  // Size Of This Pixel Format
                                                // Descriptor
                1,                              // Version Number
                PFD_DRAW_TO_WINDOW |            // Format Must Support Window
                    PFD_SUPPORT_OPENGL |        // Format Must Support OpenGL
                    PFD_DOUBLEBUFFER,           // Must Support Double Buffering
                PFD_TYPE_RGBA,                  // Request An RGBA Format
                (BYTE)bits,                     // Select Our Color Depth
                0, 0, 0, 0, 0,
                0,  // Color Bits Ignored
                0,  // No Alpha Buffer
                0,  // Shift Bit Ignored
                0,  // No Accumulation Buffer
                0, 0, 0,
                0,               // Accumulation Bits Ignored
                24,              // 16Bit Z-Buffer (Depth Buffer)
                8,               // No Stencil Buffer
                0,               // No Auxiliary Buffer
                PFD_MAIN_PLANE,  // Main Drawing Layer
                0,               // Reserved
                0, 0,
                0  // Layer Masks Ignored
            };

        if (!(m_hDC = GetDC(m_hWnd)))  // Did We Get A Device Context?
        {
            destroy();  // Reset The Display
            std::cerr << "WGLWindow Error: Can't Create A GL Device Context." << std::endl;
            return;  // Return false
        }

        if (!(m_pixelFormat = ChoosePixelFormat(m_hDC, &pfd)))  // Did Windows Find A Matching Pixel Format?
        {
            destroy();  // Reset The Display
            std::cerr << "WGLWindow Error: Can't Find A Suitable PixelFormat." << std::endl;
            return;  // Return false
        }

        if (!SetPixelFormat(m_hDC, m_pixelFormat,
                            &pfd))  // Are We Able To Set The Pixel Format?
        {
            destroy();  // Reset The Display
            std::cerr << "WGLWindow Error: Can't Set The PixelFormat." << std::endl;
            return;  // Return false
        }

        if (!(m_hRC = wglCreateContext(m_hDC)))  // Are We Able To Get A Rendering Context?
        {
            destroy();  // Reset The Display
            std::cerr << "WGLWindow Error: Can't Create A GL Rendering Context." << std::endl;
            return;  // Return false
        }

        if (!wglMakeCurrent(m_hDC,
                            m_hRC))  // Try To Activate The Rendering Context
        {
            destroy();  // Reset The Display
            std::cerr << "WGLWindow Error: Can't Activate The GL Rendering Context." << std::endl;
            return;  // Return false
        }

        ShowWindow(m_hWnd, hidden ? SW_HIDE : SW_SHOW);  // Show The Window
        m_isOpen = !hidden;

        SetForegroundWindow(m_hWnd);  // Slightly Higher Priority
        SetFocus(m_hWnd);             // Sets Keyboard Focus To The Window
        resize(width, height);        // Set Up Our Perspective GL Screen

        if (!initGLEW()) return;

        if (!init())  // Initialize Our Newly Created GL Window
        {
            destroy();  // Reset The Display
            std::cerr << "Initialization Failed." << std::endl;
            return;  // Return false
        }
        resize(width, height);  // Set Up Our Perspective GL Screen
    });
    m_msgLoop.detach();

    return true;  // Success
}

bool WGLWindow::init() {
    GLenum err = glGetError();

    RECT rWnd;
    GetWindowRect(m_hWnd, &rWnd);
    DWORD       dwStyle   = GetWindowLong(m_hWnd, GWL_STYLE);
    DWORD       dwExStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE);
    LPCWSTR     szVer = reinterpret_cast<LPCWSTR>(glGetString(GL_VERSION));
    HWND        newHwnd;
    if (!(newHwnd = CreateWindowEx(dwExStyle,                // Extended Style For The Window
                                   reinterpret_cast<LPCWSTR>(m_thisClassName.c_str()),  // Class Name
                                   szVer,                    // Window Title
                                   dwStyle |                 // Defined Window Style
                                       WS_CLIPSIBLINGS |     // Required Window Style
                                       WS_CLIPCHILDREN,      // Required Window Style
                                   rWnd.left, rWnd.top,      // Window Position
                                   rWnd.right - rWnd.left,   // Calculate Window Width
                                   rWnd.bottom - rWnd.top,   // Calculate Window Height
                                   NULL,                     // No Parent Window
                                   NULL,                     // No Menu
                                   m_hInstance,              // Instance
                                   NULL)))                   // Dont Pass Anything To WM_CREATE
    {
        destroy();  // Reset The Display
        std::cerr << "Window Creation Error." << std::endl;
        return false;  // Return false
    }

    m_widthVirt  = rWnd.right - rWnd.left;
    m_heightVirt = rWnd.bottom - rWnd.top;
    m_offsX      = rWnd.left;
    m_offsY      = rWnd.top;

    SetWindowLongPtrW(m_hWnd, 0, reinterpret_cast<LONG_PTR>(this));

    HDC       newHDC         = GetDC(newHwnd);
    const int pixelAttribs[] = {WGL_DRAW_TO_WINDOW_ARB,
                                GL_TRUE,
                                WGL_SUPPORT_OPENGL_ARB,
                                GL_TRUE,
                                WGL_DOUBLE_BUFFER_ARB,
                                GL_TRUE,
                                WGL_PIXEL_TYPE_ARB,
                                WGL_TYPE_RGBA_ARB,
                                WGL_TRANSPARENT_ARB,
                                m_transparent ? GL_TRUE : GL_FALSE,
                                WGL_ACCELERATION_ARB,
                                WGL_FULL_ACCELERATION_ARB,
                                WGL_COLOR_BITS_ARB,
                                32,
                                WGL_ALPHA_BITS_ARB,
                                8,
                                WGL_DEPTH_BITS_ARB,
                                24,
                                WGL_STENCIL_BITS_ARB,
                                8,
                                WGL_SAMPLE_BUFFERS_ARB,
                                GL_TRUE,
                                WGL_SAMPLES_ARB,
                                2,
                                0};

    int  pixelFormatID;
    UINT numFormats;
    BOOL status = wglChoosePixelFormatARB(newHDC, pixelAttribs, NULL, 1, &pixelFormatID, &numFormats);

    if (status == false || numFormats == 0) {
        destroy();  // Reset The Display
        std::cerr << "WGLWindow Error: Can't Find A Suitable PixelFormat." << std::endl;
        return false;  // Return false
    }

    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(newHDC, pixelFormatID, sizeof(pfd), &pfd);

    if (!SetPixelFormat(newHDC, pixelFormatID, &pfd)) {  // Are We Able To Set The Pixel Format?
        destroy();  // Reset The Display
        std::cerr << "WGLWindow Error: Can't Set The PixelFormat." << std::endl;
        return false;  // Return false
    }

    HGLRC thRC;
    // choose opengl version automatically
    if (!(thRC = wglCreateContextAttribsARB(newHDC, m_sharedCtx, nullptr))) {
        destroy();  // Reset The Display
        std::cerr << "WGLWindow Error: Can't Create A GL Rendering Context." << std::endl;
        return false;  // Return false
    }

    // transparent window
    if (m_transparent) {
        HRGN           region = CreateRectRgn(0, 0, -1, -1);
        DWM_BLURBEHIND bb     = {0};
        bb.dwFlags            = DWM_BB_ENABLE | DWM_BB_BLURREGION;
        bb.hRgnBlur           = region;
        // bb.dwFlags = DWM_BB_ENABLE;
        // bb.hRgnBlur = NULL;

        bb.fEnable = TRUE;
        DwmEnableBlurBehindWindow(newHwnd, &bb);
        MARGINS margins = {-1};
        DwmExtendFrameIntoClientArea(newHwnd, &margins);
    }

    // destroy and recreate????
    wglMakeCurrent(m_hDC, nullptr);
    wglDeleteContext(m_hRC);
    ReleaseDC(m_hWnd, m_hDC);
    DestroyWindow(m_hWnd);
    m_hWnd = newHwnd;
    m_hDC  = newHDC;
    m_hRC  = thRC;

    if (!wglMakeCurrent(m_hDC, m_hRC))  // Try To Activate The Rendering Context
    {
        destroy();  // Reset The Display
        std::cerr << "WGLWindow Error: Can't Activate The GL Rendering Context." << std::endl;
        return false;  // Return false
    }

    return true;  // Initialization Went OK
}

void WGLWindow::destroy()  // Properly Kill The Window
{
    m_active = false;

    if (m_fullscreen) {
        ChangeDisplaySettings(nullptr, 0);  // If So Switch Back To The Desktop
    }

    if (m_hRC) {
        if (!wglMakeCurrent(nullptr, nullptr)) {
            std::cout << "Release Of DC And RC Failed." << std::endl;
        }

        if (!wglDeleteContext(m_hRC)) {
            std::cout << "Release Rendering Context Failed." << std::endl;
        }
        m_hRC = nullptr;
    }

    if (m_hDC && !ReleaseDC(m_hWnd, m_hDC)) {  // Are We Able To Release The DC
        std::cout << "Release Device Context Failed." << std::endl;
        m_hDC = nullptr;
    }

    if (m_hWnd && !DestroyWindow(m_hWnd)) { // Are We Able To Destroy The Window?
        std::cout << "Could Not Release hWnd.." << std::endl;
        m_hWnd = nullptr;
    }

    if (!UnregisterClass(reinterpret_cast<LPCWSTR>(m_thisClassName.c_str()), m_hInstance)) {
        m_hInstance = nullptr;  // Set m_hInstance To NULL
    }
}

void WGLWindow::resize(GLsizei width, GLsizei height) {
    if (height == 0) {
        height = 1;
    }
    glViewport(0, 0, width, height);  // Reset The Current Viewport
}

LRESULT CALLBACK WGLWindow::WndProc(HWND   hWnd,    // Handle For This Window
                                    UINT   uMsg,    // Message For This Window
                                    WPARAM wParam,  // Additional Message Information
                                    LPARAM lParam)  // Additional Message Information
{
    auto thisWin = reinterpret_cast<WGLWindow*>(GetWindowLongPtrW(hWnd, 0));

    switch (uMsg) {
        case WM_ACTIVATE: {
            // LoWord Can Be WA_INACTIVE, WA_ACTIVE, WA_CLICKACTIVE,
            // The High-Order Word Specifies The Minimized State Of The Window
            // Being Activated Or Deactivated. A NonZero Value Indicates The
            // Window Is Minimized.
            if ((LOWORD(wParam) != WA_INACTIVE) && !((BOOL)HIWORD(wParam)))
                thisWin->m_active = true;  // Program Is Active
            else
                thisWin->m_active = false;  // Program Is No Longer Active

            return 0;  // Return To The Message Loop
        }

        case WM_SYSCOMMAND: {
            switch (wParam) {
                case SC_SCREENSAVE:    // Screensaver Trying To Start?
                case SC_MONITORPOWER:  // Monitor Trying To Enter Powersave?
                    return 0;          // Prevent From Happening
            }
            break;  // Exit
        }

        case WM_CLOSE: {
            if (thisWin->m_callbacks.close) {
                thisWin->m_callbacks.close();
            }
            PostQuitMessage(0);  // Send A Quit Message
            return 0;            // Jump Back
        }

        case WM_MOUSEACTIVATE: {
            if (HIWORD(lParam) == WM_LBUTTONDOWN) {
                if (LOWORD(lParam) != HTCLIENT) {
                    thisWin->m_frameAction = true;
                }
            }

            break;
        }

        case WM_CAPTURECHANGED: {
            if (lParam == 0 && thisWin->m_frameAction) {
                if (thisWin->m_cursorMode == GLSG_CURSOR_DISABLED) {
                    thisWin->disableCursor();
                }
                thisWin->m_frameAction = false;
            }

            break;
        }

        case WM_SETFOCUS: {
            thisWin->inputWindowFocus(true);
            if (thisWin->m_frameAction) {
                break;
            }
            if (thisWin->m_cursorMode == GLSG_CURSOR_DISABLED) {
                thisWin->disableCursor();
            }
            return 0;
        }

        case WM_KILLFOCUS: {
            if (thisWin->m_cursorMode == GLSG_CURSOR_DISABLED) {
                thisWin->enableCursor();
            }
            thisWin->inputWindowFocus(false);
            return 0;
        }

        case WM_KEYDOWN: {
            printf(" WM_KEYDOWN \n");
            if (VK_ESCAPE == wParam) {
                PostQuitMessage(0);  // Send A Quit Message
                thisWin->m_exitSema.notify();
                return 0;  // Jump Back
            }
        }

        case WM_INPUTLANGCHANGE: {
            thisWin->updateKeyNamesWin32();
            break;
        }

        case WM_UNICHAR: {
            const bool plain = (uMsg != WM_SYSCHAR);
            if (uMsg == WM_UNICHAR && wParam == UNICODE_NOCHAR) {
                return TRUE;
            }
            thisWin->inputChar((unsigned int)wParam, thisWin->getKeyMods(), plain);
            return 0;
        }

        case WM_SYSKEYUP: {
            int       key, scancode;
            const int action = (HIWORD(lParam) & KF_UP) ? GLSG_RELEASE : GLSG_PRESS;
            const int mods   = thisWin->getKeyMods();

            scancode = (HIWORD(lParam) & (KF_EXTENDED | 0xff));
            if (!scancode) {
                scancode = MapVirtualKeyW((UINT)wParam, MAPVK_VK_TO_VSC);
            }

            key = thisWin->m_keycodes[scancode];
            if (wParam == VK_CONTROL) {
                if (HIWORD(lParam) & KF_EXTENDED) {
                    key = GLSG_KEY_RIGHT_CONTROL;
                } else {
                    MSG         next;
                    const DWORD time = GetMessageTime();

                    if (PeekMessageW(&next, NULL, 0, 0, PM_NOREMOVE)) {
                        if (next.message == WM_KEYDOWN || next.message == WM_SYSKEYDOWN || next.message == WM_KEYUP ||
                            next.message == WM_SYSKEYUP) {
                            if (next.wParam == VK_MENU && (HIWORD(next.lParam) & KF_EXTENDED) && next.time == time) {
                                // Next message is Right Alt down so discard
                                // this
                                break;
                            }
                        }
                    }

                    // This is a regular Left Ctrl message
                    key = GLSG_KEY_LEFT_CONTROL;
                }
            } else if (wParam == VK_PROCESSKEY) {
                break;
            }

            if (action == GLSG_RELEASE && wParam == VK_SHIFT) {
                thisWin->inputKey(GLSG_KEY_LEFT_SHIFT, scancode, action, mods);
                thisWin->inputKey(GLSG_KEY_RIGHT_SHIFT, scancode, action, mods);
            } else if (wParam == VK_SNAPSHOT) {
                thisWin->inputKey(key, scancode, GLSG_PRESS, mods);
                thisWin->inputKey(key, scancode, GLSG_RELEASE, mods);
            } else
                thisWin->inputKey(key, scancode, action, mods);

            break;
        }

        case WM_XBUTTONUP: {
            int i, button, action;

            if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP)
                button = GLSG_MOUSE_BUTTON_LEFT;
            else if (uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP)
                button = GLSG_MOUSE_BUTTON_RIGHT;
            else if (uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP)
                button = GLSG_MOUSE_BUTTON_MIDDLE;
            else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
                button = GLSG_MOUSE_BUTTON_4;
            else
                button = GLSG_MOUSE_BUTTON_5;

            if (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg == WM_MBUTTONDOWN || uMsg == WM_XBUTTONDOWN) {
                action = GLSG_PRESS;
            } else
                action = GLSG_RELEASE;

            for (i = 0; i <= GLSG_MOUSE_BUTTON_LAST; i++) {
                if (thisWin->m_mouseButtons[i] == GLSG_PRESS) {
                    break;
                }
            }

            if (i > GLSG_MOUSE_BUTTON_LAST) {
                SetCapture(hWnd);
            }

            thisWin->inputMouseClick(button, action, thisWin->getKeyMods());

            for (i = 0; i <= GLSG_MOUSE_BUTTON_LAST; i++) {
                if (thisWin->m_mouseButtons[i] == GLSG_PRESS) {
                    break;
                }
            }

            if (i > GLSG_MOUSE_BUTTON_LAST) {
                ReleaseCapture();
            }

            if (uMsg == WM_XBUTTONDOWN || uMsg == WM_XBUTTONUP) {
                return TRUE;
            }

            return 0;
        }

        case WM_MOUSEMOVE: {
            const int x = GET_X_LPARAM(lParam);
            const int y = GET_Y_LPARAM(lParam);

            if (!thisWin->m_cursorTracked) {
                TRACKMOUSEEVENT tme;
                ZeroMemory(&tme, sizeof(tme));
                tme.cbSize    = sizeof(tme);
                tme.dwFlags   = TME_LEAVE;
                tme.hwndTrack = thisWin->m_hWnd;
                TrackMouseEvent(&tme);

                thisWin->m_cursorTracked = true;
                thisWin->inputCursorEnter(true);
            }

            if (thisWin->m_cursorMode == GLSG_CURSOR_DISABLED) {
                const int m_dx = x - thisWin->m_lastCursorPosX;
                const int dy   = y - thisWin->m_lastCursorPosY;

                // if (_glfw.win32.disabledCursorWindow != window)
                //     break;
                if (thisWin->m_rawMouseMotion) break;

                thisWin->inputCursorPos(thisWin->m_virtualCursorPosX + m_dx, thisWin->m_virtualCursorPosY + dy);
            } else
                thisWin->inputCursorPos(x, y);

            thisWin->m_lastCursorPosX = x;
            thisWin->m_lastCursorPosY = y;

            return 0;
        }

        case WM_INPUT: {
            UINT      size = 0;
            HRAWINPUT ri   = (HRAWINPUT)lParam;
            RAWINPUT* data = NULL;
            int       m_dx, dy;

            // if (thisWin->m_disabledCursorWindow != window)break;

            if (!thisWin->m_rawMouseMotion) break;

            GetRawInputData(ri, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER));
            if (size > (UINT)thisWin->m_rawInputSize) {
                free(thisWin->m_rawInput);
                thisWin->m_rawInput     = (RAWINPUT*)calloc(size, 1);
                thisWin->m_rawInputSize = size;
            }

            size = thisWin->m_rawInputSize;
            if (GetRawInputData(ri, RID_INPUT, thisWin->m_rawInput, &size, sizeof(RAWINPUTHEADER)) == (UINT)-1) {
                std::cerr << "Win32: Failed to retrieve raw input data" << std::endl;
                break;
            }

            data = thisWin->m_rawInput;
            if (data->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
                m_dx = data->data.mouse.lLastX - thisWin->m_lastCursorPosX;
                dy   = data->data.mouse.lLastY - thisWin->m_lastCursorPosY;
            } else {
                m_dx = data->data.mouse.lLastX;
                dy   = data->data.mouse.lLastY;
            }

            thisWin->inputCursorPos(thisWin->m_virtualCursorPosX + m_dx, thisWin->m_virtualCursorPosY + dy);
            thisWin->m_lastCursorPosX += m_dx;
            thisWin->m_lastCursorPosY += dy;
            break;
        }

        case WM_MOUSELEAVE: {
            thisWin->m_cursorTracked = false;
            thisWin->inputCursorEnter(false);
            return 0;
        }

        case WM_MOUSEWHEEL: {
            thisWin->inputScroll(0.0, (SHORT)HIWORD(wParam) / (double)WHEEL_DELTA);
            return 0;
        }

        case WM_MOUSEHWHEEL: {
            // This message is only sent on Windows Vista and later
            // NOTE: The X-axis is inverted for consistency with macOS and X11
            thisWin->inputScroll(-((SHORT)HIWORD(wParam) / (double)WHEEL_DELTA), 0.0);
            return 0;
        }

        case WM_ENTERSIZEMOVE:
        case WM_ENTERMENULOOP: {
            if (thisWin->m_frameAction) break;

            // HACK: Enable the cursor while the user is moving or
            //       resizing the window or using the window menu
            if (thisWin->m_cursorMode == GLSG_CURSOR_DISABLED) thisWin->enableCursor();

            break;
        }

        case WM_EXITSIZEMOVE:
        case WM_EXITMENULOOP: {
            if (thisWin->m_frameAction) break;

            // HACK: Disable the cursor once the user is done moving or resizing
            // the window or using the menu
            if (thisWin->m_cursorMode == GLSG_CURSOR_DISABLED) thisWin->disableCursor();

            break;
        }

        case WM_SIZE: {
            thisWin->resize(LOWORD(lParam),
                            HIWORD(lParam));  // LoWord=Width, HiWord=Height

            const bool iconified = wParam == SIZE_MINIMIZED;
            const bool maximized = wParam == SIZE_MAXIMIZED || (thisWin->m_maximized && wParam != SIZE_RESTORED);

            // if (thisWin->m_disabledCursorWindow == window)
            //     updateClipRect(window);

            if (thisWin->m_iconified != iconified) thisWin->inputWindowIconify(iconified);

            if (thisWin->m_maximized != maximized) thisWin->inputWindowMaximize(maximized);

            thisWin->inputFramebufferSize(LOWORD(lParam), HIWORD(lParam));
            thisWin->inputWindowSize(LOWORD(lParam), HIWORD(lParam));

            /*
            if (thisWin->m_monitor && thisWin->m_iconified != iconified)
            {
                if (iconified)
                    releaseMonitor(window);
                else
                {
                    acquireMonitor(window);
                    fitToMonitor(window);
                }
            }*/

            thisWin->m_iconified = iconified;
            thisWin->m_maximized = maximized;
            return 0;
        }

        case WM_MOVE: {
            // if (thisWin->m_disabledCursorWindow == window)
            //     updateClipRect(window);

            // NOTE: This cannot use LOWORD/HIWORD recommended by MSDN, as
            // those macros do not handle negative window positions correctly
            thisWin->inputWindowPos(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }

        case WM_SIZING: {
            if (thisWin->m_numer == GLSG_DONT_CARE || thisWin->m_denom == GLSG_DONT_CARE) break;

            thisWin->applyAspectRatio((int)wParam, (RECT*)lParam);
            return TRUE;
        }

        case WM_GETMINMAXINFO: {
            //                int xoff, yoff;
            UINT        dpi = USER_DEFAULT_SCREEN_DPI;
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;

            // if (thisWin->m_monitor)
            //     break;
            /*
                            if
               (_glfwIsWindows10AnniversaryUpdateOrGreaterWin32()) dpi =
               GetDpiForWindow(thisWin->m_handle);

                            getFullWindowSize(getWindowStyle(window),
               getWindowExStyle(window), 0, 0, &xoff, &yoff, dpi);

                            if (thisWin->m_minwidth != GLSG_DONT_CARE &&
                                thisWin->m_minheight != GLSG_DONT_CARE)
                            {
                                mmi->ptMinTrackSize.x = thisWin->m_minwidth +
               xoff; mmi->ptMinTrackSize.y = thisWin->m_minheight + yoff;
                            }

                            if (thisWin->m_maxwidth != GLSG_DONT_CARE &&
                                thisWin->m_maxheight != GLSG_DONT_CARE)
                            {
                                mmi->ptMaxTrackSize.x = thisWin->m_maxwidth +
               xoff; mmi->ptMaxTrackSize.y = thisWin->m_maxheight + yoff;
                            }

                            if (!thisWin->m_decorated)
                            {
                                MONITORINFO mi;
                                const HMONITOR mh =
               MonitorFromWindow(thisWin->m_handle, MONITOR_DEFAULTTONEAREST);

                                ZeroMemory(&mi, sizeof(mi));
                                mi.cbSize = sizeof(mi);
                                GetMonitorInfo(mh, &mi);

                                mmi->ptMaxPosition.x = mi.rcWork.left -
               mi.rcMonitor.left; mmi->ptMaxPosition.y = mi.rcWork.top -
               mi.rcMonitor.top; mmi->ptMaxSize.x = mi.rcWork.right -
               mi.rcWork.left; mmi->ptMaxSize.y = mi.rcWork.bottom -
               mi.rcWork.top;
                            }
            */
            return 0;
        }

        case WM_PAINT: {
            thisWin->inputWindowDamage();
            break;
        }

        case WM_ERASEBKGND: {
            return TRUE;
        }

        case WM_NCACTIVATE:
        case WM_NCPAINT: {
            // Prevent title bar from being drawn after restoring a minimized
            // undecorated window
            if (!thisWin->m_decorated) return TRUE;

            break;
        }

        case WM_DWMCOMPOSITIONCHANGED: {
            if (thisWin->m_transparent) thisWin->updateFramebufferTransparency();
            return 0;
        }

        case WM_GETDPISCALEDSIZE: {
            if (thisWin->m_scaleToMonitor) break;

            // Adjust the window size to keep the content area size constant
            if (thisWin->isWindows10BuildOrGreaterWin32(15063)) {
                RECT  source = {0}, target = {0};
                SIZE* size = (SIZE*)lParam;

                AdjustWindowRectExForDpi(&source, thisWin->getWindowStyle(), FALSE, thisWin->getWindowExStyle(),
                                         GetDpiForWindow(thisWin->m_hWnd));
                AdjustWindowRectExForDpi(&target, thisWin->getWindowStyle(), FALSE, thisWin->getWindowExStyle(),
                                         LOWORD(wParam));

                size->cx += (target.right - target.left) - (source.right - source.left);
                size->cy += (target.bottom - target.top) - (source.bottom - source.top);
                return TRUE;
            }

            break;
        }

        case WM_DPICHANGED: {
            const float xscale = HIWORD(wParam) / (float)USER_DEFAULT_SCREEN_DPI;
            const float yscale = LOWORD(wParam) / (float)USER_DEFAULT_SCREEN_DPI;

            // Only apply the suggested size if the OS is new enough to have
            // sent a WM_GETDPISCALEDSIZE before this
            if (thisWin->isWindows10BuildOrGreaterWin32(15063)) {
                RECT* suggested = (RECT*)lParam;
                SetWindowPos(thisWin->m_hWnd, HWND_TOP, suggested->left, suggested->top,
                             suggested->right - suggested->left, suggested->bottom - suggested->top,
                             SWP_NOACTIVATE | SWP_NOZORDER);
            }

            thisWin->inputWindowContentScale(xscale, yscale);
            break;
        }

        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT) {
                thisWin->updateCursorImage();
                return TRUE;
            }

            break;
        }

        case WM_DROPFILES: {
        }
    }

    // Pass All Unhandled Messages To DefWindowProc
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// Enforce the content area aspect ratio based on which edge is being dragged
void WGLWindow::applyAspectRatio(int edge, RECT* area) {
    int         xoff, yoff;
    UINT        dpi   = USER_DEFAULT_SCREEN_DPI;
    const float ratio = (float)m_numer / (float)m_denom;

    if (isWindows10BuildOrGreaterWin32(14393)) dpi = GetDpiForWindow(m_hWnd);

    getFullWindowSize(getWindowStyle(), getWindowExStyle(), 0, 0, &xoff, &yoff, dpi);

    if (edge == WMSZ_LEFT || edge == WMSZ_BOTTOMLEFT || edge == WMSZ_RIGHT || edge == WMSZ_BOTTOMRIGHT) {
        area->bottom = area->top + yoff + (int)((area->right - area->left - xoff) / ratio);
    } else if (edge == WMSZ_TOPLEFT || edge == WMSZ_TOPRIGHT) {
        area->top = area->bottom - yoff - (int)((area->right - area->left - xoff) / ratio);
    } else if (edge == WMSZ_TOP || edge == WMSZ_BOTTOM) {
        area->right = area->left + xoff + (int)((area->bottom - area->top - yoff) * ratio);
    }
}

// Enables WM_INPUT messages for the mouse for the specified window
void WGLWindow::enableRawMouseMotion() {
    const RAWINPUTDEVICE rid = {0x01, 0x02, 0, m_hWnd};
    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
        std::cerr << "Win32: Failed to register raw input device" << std::endl;
}

// Disables WM_INPUT messages for the mouse
void WGLWindow::disableRawMouseMotion() {
    const RAWINPUTDEVICE rid = {0x01, 0x02, RIDEV_REMOVE, NULL};
    if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
        std::cerr << "Win32: Failed to remove raw input device" << std::endl;
}

// Exit disabled cursor mode for the specified window
void WGLWindow::enableCursor() {
    if (m_rawMouseMotion) disableRawMouseMotion();

    // m_disabledCursorWindow = NULL;
    ClipCursor(NULL);
    setCursorPos(m_restoreCursorPosX, m_restoreCursorPosY);
    updateCursorImage();
}

// Apply disabled cursor mode to a focused window
void WGLWindow::disableCursor() {
    getCursorPos(&m_restoreCursorPosX, &m_restoreCursorPosY);
    updateCursorImage();
    centerCursorInContentArea();
    updateClipRect();

    if (m_rawMouseMotion) enableRawMouseMotion();
}

void WGLWindow::getCursorPos(double* xpos, double* ypos) {
    POINT pos;

    if (GetCursorPos(&pos)) {
        ScreenToClient(m_hWnd, &pos);

        if (xpos) *xpos = pos.x;
        if (ypos) *ypos = pos.y;
    }
}

void WGLWindow::setCursorPos(double xpos, double ypos) {
    POINT pos = {(int)xpos, (int)ypos};

    // Store the new position so it can be recognized later
    m_lastCursorPosX = pos.x;
    m_lastCursorPosY = pos.y;

    ClientToScreen(m_hWnd, &pos);
    SetCursorPos(pos.x, pos.y);
}

// Center the cursor in the content area of the specified window
void WGLWindow::centerCursorInContentArea() {
    int width, height;
    getWindowSize(&width, &height);
    setCursorPos(width / 2.0, height / 2.0);
}

// Returns whether the cursor is in the content area of the specified window
bool WGLWindow::cursorInContentArea() {
    if (!m_hWnd) return false;

    RECT  area;
    POINT pos;

    if (!GetCursorPos(&pos)) return false;

    if (WindowFromPoint(pos) != m_hWnd) return false;

    GetClientRect(m_hWnd, &area);
    ClientToScreen(m_hWnd, (POINT*)&area.left);
    ClientToScreen(m_hWnd, (POINT*)&area.right);

    return PtInRect(&area, pos);
}

// Updates the cursor image according to its cursor mode
void WGLWindow::updateCursorImage() {
    if (m_cursorMode == GLSG_CURSOR_NORMAL) {
        if (m_cursorHandle)
            SetCursor(m_cursorHandle);
        else
            SetCursor(LoadCursor(NULL, IDC_ARROW));
    } else
        SetCursor(NULL);
}

// Updates the cursor clip rect
void WGLWindow::updateClipRect() {
    if (m_hWnd) {
        RECT clipRect;
        GetClientRect(m_hWnd, &clipRect);
        ClientToScreen(m_hWnd, (POINT*)&clipRect.left);
        ClientToScreen(m_hWnd, (POINT*)&clipRect.right);
        ClipCursor(&clipRect);
    } else
        ClipCursor(NULL);
}

// Updates key names according to the current keyboard layout
void WGLWindow::updateKeyNamesWin32() {
    BYTE state[256] = {0};

    memset(m_keynames, 0, sizeof(m_keynames));

    for (int key = GLSG_KEY_SPACE; key <= GLSG_KEY_LAST; key++) {
        UINT  vk;
        int   scancode, length;
        WCHAR chars[16];

        scancode = m_scancodes[key];
        if (scancode == -1) continue;

        if (key >= GLSG_KEY_KP_0 && key <= GLSG_KEY_KP_ADD) {
            const UINT vks[] = {VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2,  VK_NUMPAD3,  VK_NUMPAD4,
                                VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7,  VK_NUMPAD8,  VK_NUMPAD9,
                                VK_DECIMAL, VK_DIVIDE,  VK_MULTIPLY, VK_SUBTRACT, VK_ADD};

            vk = vks[key - GLSG_KEY_KP_0];
        } else
            vk = MapVirtualKey(scancode, MAPVK_VSC_TO_VK);

        length = ToUnicode(vk, scancode, state, chars, sizeof(chars) / sizeof(WCHAR), 0);

        if (length == -1) {
            length = ToUnicode(vk, scancode, state, chars, sizeof(chars) / sizeof(WCHAR), 0);
        }

        if (length < 1) continue;

        WideCharToMultiByte(CP_UTF8, 0, chars, 1, m_keynames[key], sizeof(m_keynames[key]), NULL, NULL);
    }
}

// Update window framebuffer transparency
void WGLWindow::updateFramebufferTransparency() {
    BOOL enabled;

    if (!isWindowsVersionOrGreaterWin32(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA), 0)) return;

    if (SUCCEEDED(DwmIsCompositionEnabled(&enabled)) && enabled) {
        HRGN           region = CreateRectRgn(0, 0, -1, -1);
        DWM_BLURBEHIND bb     = {0};
        bb.dwFlags            = DWM_BB_ENABLE | DWM_BB_BLURREGION;
        bb.hRgnBlur           = region;
        bb.fEnable            = TRUE;

        if (SUCCEEDED(DwmEnableBlurBehindWindow(m_hWnd, &bb))) {
            // Decorated windows don't repaint the transparent background
            // leaving a trail behind animations
            // HACK: Making the window layered with a transparency color key
            //       seems to fix this.  Normally, when specifying
            //       a transparency color key to be used when composing the
            //       layered window, all pixels painted by the window in this
            //       color will be transparent.  That doesn't seem to be the
            //       case anymore, at least when used with blur behind window
            //       plus negative region.
            LONG exStyle = GetWindowLongW(m_hWnd, GWL_EXSTYLE);
            exStyle |= WS_EX_LAYERED;
            SetWindowLongW(m_hWnd, GWL_EXSTYLE, exStyle);

            // Using a color key not equal to black to fix the trailing
            // issue.  When set to black, something is making the hit test
            // not resize with the window m_frame.
            SetLayeredWindowAttributes(m_hWnd, RGB(255, 0, 255), 255, LWA_COLORKEY);
        }

        DeleteObject(region);
    } else {
        LONG exStyle = GetWindowLongW(m_hWnd, GWL_EXSTYLE);
        exStyle &= ~WS_EX_LAYERED;
        SetWindowLongW(m_hWnd, GWL_EXSTYLE, exStyle);
        RedrawWindow(m_hWnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_FRAME);
    }
}

// Notifies shared code of a cursor motion event
// The position is specified in content area relative screen coordinates
void WGLWindow::inputCursorPos(double xpos, double ypos) {
    if (m_virtualCursorPosX == xpos && m_virtualCursorPosY == ypos) return;

    m_virtualCursorPosX = xpos;
    m_virtualCursorPosY = ypos;

    if (m_callbacks.cursorPos) m_callbacks.cursorPos(xpos, ypos);
}

// Notifies shared code that a window has lost or received input focus
void WGLWindow::inputWindowFocus(bool focused) {
    if (m_callbacks.focus) m_callbacks.focus(focused);

    if (!focused) {
        for (int key = 0; key <= GLSG_KEY_LAST; key++) {
            if (m_keys[key] == GLSG_PRESS) {
                const int scancode = getKeyScancode(key);
                inputKey(key, scancode, GLSG_RELEASE, 0);
            }
        }

        for (int button = 0; button <= GLSG_MOUSE_BUTTON_LAST; button++) {
            if (m_mouseButtons[button] == GLSG_PRESS) inputMouseClick(button, GLSG_RELEASE, 0);
        }
    }
}

// Notifies shared code of a mouse button click event
void WGLWindow::inputMouseClick(int button, int action, int mods) {
    if (button < 0 || button > GLSG_MOUSE_BUTTON_LAST) return;

    if (!m_lockKeyMods) mods &= ~(GLSG_MOD_CAPS_LOCK | GLSG_MOD_NUM_LOCK);

    if (action == GLSG_RELEASE && m_stickyMouseButtons)
        m_mouseButtons[button] = _GLSG_STICK;
    else
        m_mouseButtons[button] = (char)action;

    if (m_callbacks.mouseButton) m_callbacks.mouseButton(button, action, mods);
}

// Notifies shared code of a Unicode codepoint input event
// The 'plain' parameter determines whether to emit a regular character event
void WGLWindow::inputChar(unsigned int codepoint, int mods, bool plain) {
    if (codepoint < 32 || (codepoint > 126 && codepoint < 160)) return;

    if (!m_lockKeyMods) mods &= ~(GLSG_MOD_CAPS_LOCK | GLSG_MOD_NUM_LOCK);

    if (m_callbacks.charmods) m_callbacks.charmods(codepoint, mods);

    if (plain) {
        if (m_callbacks.character) m_callbacks.character(codepoint);
    }
}

void WGLWindow::inputKey(int key, int scancode, int action, int mods) {
    if (key >= 0 && key <= GLSG_KEY_LAST) {
        bool repeated = false;

        if (action == GLSG_RELEASE && m_keys[key] == GLSG_RELEASE) return;

        if (action == GLSG_PRESS && m_keys[key] == GLSG_PRESS) repeated = true;

        if (action == GLSG_RELEASE && m_stickyKeys)
            m_keys[key] = _GLSG_STICK;
        else
            m_keys[key] = (char)action;

        if (repeated) action = GLSG_REPEAT;
    }

    if (!m_lockKeyMods) mods &= ~(GLSG_MOD_CAPS_LOCK | GLSG_MOD_NUM_LOCK);

    if (m_callbacks.key) m_callbacks.key(key, scancode, action, mods);
}

void WGLWindow::setInputMode(int mode, int value) {
    assert(m_hWnd != nullptr);

    //_GLSG_REQUIRE_INIT();

    if (mode == GLSG_CURSOR) {
        if (value != GLSG_CURSOR_NORMAL && value != GLSG_CURSOR_HIDDEN && value != GLSG_CURSOR_DISABLED) {
            std::cerr << "Invalid cursor mode 0x%08X " << value << std::endl;
            return;
        }

        if (m_cursorMode == value) return;

        m_cursorMode = value;

        getCursorPos(&m_virtualCursorPosX, &m_virtualCursorPosY);
        setCursorMode(value);
    } else if (mode == GLSG_STICKY_KEYS) {
        value = value ? true : false;
        if (m_stickyKeys == (bool)value) return;

        if (!value) {
            int i;

            // Release all sticky keys
            for (i = 0; i <= GLSG_KEY_LAST; i++) {
                if (m_keys[i] == _GLSG_STICK) m_keys[i] = GLSG_RELEASE;
            }
        }

        m_stickyKeys = value;
    } else if (mode == GLSG_STICKY_MOUSE_BUTTONS) {
        value = value ? true : false;
        if (m_stickyMouseButtons == (bool)value) return;

        if (!value) {
            int i;

            // Release all sticky mouse buttons
            for (i = 0; i <= GLSG_MOUSE_BUTTON_LAST; i++) {
                if (m_mouseButtons[i] == _GLSG_STICK) m_mouseButtons[i] = GLSG_RELEASE;
            }
        }

        m_stickyMouseButtons = value;
    } else if (mode == GLSG_LOCK_KEY_MODS) {
        m_lockKeyMods = value ? true : false;
    } else if (mode == GLSG_RAW_MOUSE_MOTION) {
        if (!rawMouseMotionSupported()) {
            std::cerr << "Raw mouse motion is not supported on this system" << std::endl;
            return;
        }

        value = value ? true : false;
        if (m_rawMouseMotion == (bool)value) return;

        m_rawMouseMotion = value;
        setRawMouseMotion(value);
    } else
        std::cerr << "Invalid input mode 0x%08X " << mode << std::endl;
}

void WGLWindow::setCursorMode(int mode) {
    if (mode == GLSG_CURSOR_DISABLED)
        if (windowFocused()) disableCursor();

        // else if (_glfw.win32.disabledCursorWindow == window)
        //     enableCursor();
        else if (cursorInContentArea())
            updateCursorImage();
}

void WGLWindow::setRawMouseMotion(bool enabled) {
    if (enabled)
        enableRawMouseMotion();
    else
        disableRawMouseMotion();
}

// Checks whether we are on at least the specified build of Windows 10
BOOL WGLWindow::isWindows10BuildOrGreaterWin32(WORD build) {
    OSVERSIONINFOEXW osvi = {sizeof(osvi), 10, 0, build};
    DWORD            mask = VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER;
    ULONGLONG        cond = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
    cond                  = VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
    cond                  = VerSetConditionMask(cond, VER_BUILDNUMBER, VER_GREATER_EQUAL);
    // HACK: Use RtlVerifyVersionInfo instead of VerifyVersionInfoW as the
    //       latter lies unless the user knew to embed a non-default manifest
    //       announcing support for Windows 10 via supportedOS GUID
    return RtlVerifyVersionInfo(&osvi, mask, cond) == 0;
}

// Replacement for IsWindowsVersionOrGreater as MinGW lacks versionhelpers.h
BOOL WGLWindow::isWindowsVersionOrGreaterWin32(WORD major, WORD minor, WORD sp) {
    OSVERSIONINFOEXW osvi = {sizeof(osvi), major, minor, 0, 0, {0}, sp};
    DWORD            mask = VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR;
    ULONGLONG        cond = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
    cond                  = VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
    cond                  = VerSetConditionMask(cond, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
    // HACK: Use RtlVerifyVersionInfo instead of VerifyVersionInfoW as the
    //       latter lies unless the user knew to embed a non-default manifest
    //       announcing support for Windows 10 via supportedOS GUID
    return RtlVerifyVersionInfo(&osvi, mask, cond) == 0;
}

// Returns the window style for the specified window
DWORD WGLWindow::getWindowStyle() {
    DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    /*if (window->monitor)
        style |= WS_POPUP;
    else
    {*/
    style |= WS_SYSMENU | WS_MINIMIZEBOX;

    if (m_decorated) {
        style |= WS_CAPTION;

        if (m_resizable) style |= WS_MAXIMIZEBOX | WS_THICKFRAME;
    } else
        style |= WS_POPUP;
    //}

    return style;
}

// Returns the extended window style for the specified window
DWORD WGLWindow::getWindowExStyle() {
    DWORD style = WS_EX_APPWINDOW;

    // if (m_monitor || m_floating)
    //     style |= WS_EX_TOPMOST;

    return style;
}

// Translate content area size to full window size according to styles and DPI
void WGLWindow::getFullWindowSize(DWORD style, DWORD exStyle, int contentWidth, int contentHeight, int* fullWidth,
                                  int* fullHeight, UINT dpi) {
    RECT rect = {0, 0, contentWidth, contentHeight};

    if (isWindows10BuildOrGreaterWin32(14393))
        AdjustWindowRectExForDpi(&rect, style, FALSE, exStyle, dpi);
    else
        AdjustWindowRectEx(&rect, style, FALSE, exStyle);

    *fullWidth  = rect.right - rect.left;
    *fullHeight = rect.bottom - rect.top;
}

// Retrieves and translates modifier keys
int WGLWindow::getKeyMods() {
    int mods = 0;

    if (GetKeyState(VK_SHIFT) & 0x8000) mods |= GLSG_MOD_SHIFT;
    if (GetKeyState(VK_CONTROL) & 0x8000) mods |= GLSG_MOD_CONTROL;
    if (GetKeyState(VK_MENU) & 0x8000) mods |= GLSG_MOD_ALT;
    if ((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) mods |= GLSG_MOD_SUPER;
    if (GetKeyState(VK_CAPITAL) & 1) mods |= GLSG_MOD_CAPS_LOCK;
    if (GetKeyState(VK_NUMLOCK) & 1) mods |= GLSG_MOD_NUM_LOCK;

    return mods;
}

void WGLWindow::getWindowSize(int* width, int* height) {
    RECT area;
    GetClientRect(m_hWnd, &area);

    if (width) *width = area.right;
    if (height) *height = area.bottom;
}

// Notifies shared code that a window has lost or received input focus
void WGLWindow::windowFocus(bool focused) {
    if (m_callbacks.focus) m_callbacks.focus(focused);

    if (!focused) {
        for (int key = 0; key <= GLSG_KEY_LAST; key++) {
            if (m_keys[key] == GLSG_PRESS) {
                const int scancode = getKeyScancode(key);
                inputKey(key, scancode, GLSG_RELEASE, 0);
            }
        }

        for (int button = 0; button <= GLSG_MOUSE_BUTTON_LAST; button++) {
            if (m_mouseButtons[button] == GLSG_PRESS) inputMouseClick(button, GLSG_RELEASE, 0);
        }
    }
}

// Load necessary libraries (DLLs)
bool WGLWindow::loadLibraries(void) {
    /*
    m_winmm.instance = LoadLibraryA("winmm.dll");
    if (!_glfw.win32.winmm.instance)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                             "Win32: Failed to load winmm.dll");
        return GLFW_FALSE;
    }

    _glfw.win32.winmm.GetTime = (PFN_timeGetTime)
            GetProcAddress(_glfw.win32.winmm.instance, "timeGetTime");

    _glfw.win32.user32.instance = LoadLibraryA("user32.dll");
    if (!_glfw.win32.user32.instance)
    {
        _glfwInputErrorWin32(GLFW_PLATFORM_ERROR,
                             "Win32: Failed to load user32.dll");
        return GLFW_FALSE;
    }

    _glfw.win32.user32.SetProcessDPIAware_ = (PFN_SetProcessDPIAware)
            GetProcAddress(_glfw.win32.user32.instance, "SetProcessDPIAware");
    _glfw.win32.user32.ChangeWindowMessageFilterEx_ =
    (PFN_ChangeWindowMessageFilterEx)
            GetProcAddress(_glfw.win32.user32.instance,
    "ChangeWindowMessageFilterEx");
    _glfw.win32.user32.EnableNonClientDpiScaling_ =
    (PFN_EnableNonClientDpiScaling) GetProcAddress(_glfw.win32.user32.instance,
    "EnableNonClientDpiScaling");
    _glfw.win32.user32.SetProcessDpiAwarenessContext_ =
    (PFN_SetProcessDpiAwarenessContext)
            GetProcAddress(_glfw.win32.user32.instance,
    "SetProcessDpiAwarenessContext"); _glfw.win32.user32.GetDpiForWindow_ =
    (PFN_GetDpiForWindow) GetProcAddress(_glfw.win32.user32.instance,
    "GetDpiForWindow"); _glfw.win32.user32.AdjustWindowRectExForDpi_ =
    (PFN_AdjustWindowRectExForDpi) GetProcAddress(_glfw.win32.user32.instance,
    "AdjustWindowRectExForDpi");

    _glfw.win32.dinput8.instance = LoadLibraryA("dinput8.dll");
    if (_glfw.win32.dinput8.instance)
    {
        _glfw.win32.dinput8.Create = (PFN_DirectInput8Create)
                GetProcAddress(_glfw.win32.dinput8.instance,
    "DirectInput8Create");
    }

    {
        int i;
        const char* names[] =
                {
                        "xinput1_4.dll",
                        "xinput1_3.dll",
                        "xinput9_1_0.dll",
                        "xinput1_2.dll",
                        "xinput1_1.dll",
                        NULL
                };

        for (i = 0;  names[i];  i++)
        {
            _glfw.win32.xinput.instance = LoadLibraryA(names[i]);
            if (_glfw.win32.xinput.instance)
            {
                _glfw.win32.xinput.GetCapabilities = (PFN_XInputGetCapabilities)
                        GetProcAddress(_glfw.win32.xinput.instance,
    "XInputGetCapabilities"); _glfw.win32.xinput.GetState = (PFN_XInputGetState)
                        GetProcAddress(_glfw.win32.xinput.instance,
    "XInputGetState");

                break;
            }
        }
    }

    _glfw.win32.dwmapi.instance = LoadLibraryA("dwmapi.dll");
    if (_glfw.win32.dwmapi.instance)
    {
        _glfw.win32.dwmapi.IsCompositionEnabled = (PFN_DwmIsCompositionEnabled)
                GetProcAddress(_glfw.win32.dwmapi.instance,
    "DwmIsCompositionEnabled"); _glfw.win32.dwmapi.Flush = (PFN_DwmFlush)
                GetProcAddress(_glfw.win32.dwmapi.instance, "DwmFlush");
        _glfw.win32.dwmapi.EnableBlurBehindWindow =
    (PFN_DwmEnableBlurBehindWindow) GetProcAddress(_glfw.win32.dwmapi.instance,
    "DwmEnableBlurBehindWindow");
    }

    _glfw.win32.shcore.instance = LoadLibraryA("shcore.dll");
    if (_glfw.win32.shcore.instance)
    {
        _glfw.win32.shcore.SetProcessDpiAwareness_ =
    (PFN_SetProcessDpiAwareness) GetProcAddress(_glfw.win32.shcore.instance,
    "SetProcessDpiAwareness"); _glfw.win32.shcore.GetDpiForMonitor_ =
    (PFN_GetDpiForMonitor) GetProcAddress(_glfw.win32.shcore.instance,
    "GetDpiForMonitor");
    }
*/
    ntdll.instance = LoadLibraryA("ntdll.dll");
    if (ntdll.instance) {
        ntdll.RtlVerifyVersionInfo_ = (PFN_RtlVerifyVersionInfo)GetProcAddress(ntdll.instance, "RtlVerifyVersionInfo");
    }

    return true;
}

// Unload used libraries (DLLs)
void WGLWindow::freeLibraries(void) {
    /*
    if (_glfw.win32.xinput.instance)
        FreeLibrary(_glfw.win32.xinput.instance);

    if (_glfw.win32.dinput8.instance)
        FreeLibrary(_glfw.win32.dinput8.instance);

    if (_glfw.win32.winmm.instance)
        FreeLibrary(_glfw.win32.winmm.instance);

    if (_glfw.win32.user32.instance)
        FreeLibrary(_glfw.win32.user32.instance);

    if (_glfw.win32.dwmapi.instance)
        FreeLibrary(_glfw.win32.dwmapi.instance);

    if (_glfw.win32.shcore.instance)
        FreeLibrary(_glfw.win32.shcore.instance);
*/
    if (ntdll.instance) FreeLibrary(ntdll.instance);
}

void WGLWindow::createKeyTables() {
    memset(m_keycodes, -1, sizeof(m_keycodes));
    memset(m_scancodes, -1, sizeof(m_scancodes));

    m_keycodes[0x00B] = GLSG_KEY_0;
    m_keycodes[0x002] = GLSG_KEY_1;
    m_keycodes[0x003] = GLSG_KEY_2;
    m_keycodes[0x004] = GLSG_KEY_3;
    m_keycodes[0x005] = GLSG_KEY_4;
    m_keycodes[0x006] = GLSG_KEY_5;
    m_keycodes[0x007] = GLSG_KEY_6;
    m_keycodes[0x008] = GLSG_KEY_7;
    m_keycodes[0x009] = GLSG_KEY_8;
    m_keycodes[0x00A] = GLSG_KEY_9;
    m_keycodes[0x01E] = GLSG_KEY_A;
    m_keycodes[0x030] = GLSG_KEY_B;
    m_keycodes[0x02E] = GLSG_KEY_C;
    m_keycodes[0x020] = GLSG_KEY_D;
    m_keycodes[0x012] = GLSG_KEY_E;
    m_keycodes[0x021] = GLSG_KEY_F;
    m_keycodes[0x022] = GLSG_KEY_G;
    m_keycodes[0x023] = GLSG_KEY_H;
    m_keycodes[0x017] = GLSG_KEY_I;
    m_keycodes[0x024] = GLSG_KEY_J;
    m_keycodes[0x025] = GLSG_KEY_K;
    m_keycodes[0x026] = GLSG_KEY_L;
    m_keycodes[0x032] = GLSG_KEY_M;
    m_keycodes[0x031] = GLSG_KEY_N;
    m_keycodes[0x018] = GLSG_KEY_O;
    m_keycodes[0x019] = GLSG_KEY_P;
    m_keycodes[0x010] = GLSG_KEY_Q;
    m_keycodes[0x013] = GLSG_KEY_R;
    m_keycodes[0x01F] = GLSG_KEY_S;
    m_keycodes[0x014] = GLSG_KEY_T;
    m_keycodes[0x016] = GLSG_KEY_U;
    m_keycodes[0x02F] = GLSG_KEY_V;
    m_keycodes[0x011] = GLSG_KEY_W;
    m_keycodes[0x02D] = GLSG_KEY_X;
    m_keycodes[0x015] = GLSG_KEY_Y;
    m_keycodes[0x02C] = GLSG_KEY_Z;

    m_keycodes[0x028] = GLSG_KEY_APOSTROPHE;
    m_keycodes[0x02B] = GLSG_KEY_BACKSLASH;
    m_keycodes[0x033] = GLSG_KEY_COMMA;
    m_keycodes[0x00D] = GLSG_KEY_EQUAL;
    m_keycodes[0x029] = GLSG_KEY_GRAVE_ACCENT;
    m_keycodes[0x01A] = GLSG_KEY_LEFT_BRACKET;
    m_keycodes[0x00C] = GLSG_KEY_MINUS;
    m_keycodes[0x034] = GLSG_KEY_PERIOD;
    m_keycodes[0x01B] = GLSG_KEY_RIGHT_BRACKET;
    m_keycodes[0x027] = GLSG_KEY_SEMICOLON;
    m_keycodes[0x035] = GLSG_KEY_SLASH;
    m_keycodes[0x056] = GLSG_KEY_WORLD_2;

    m_keycodes[0x00E] = GLSG_KEY_BACKSPACE;
    m_keycodes[0x153] = GLSG_KEY_DELETE;
    m_keycodes[0x14F] = GLSG_KEY_END;
    m_keycodes[0x01C] = GLSG_KEY_ENTER;
    m_keycodes[0x001] = GLSG_KEY_ESCAPE;
    m_keycodes[0x147] = GLSG_KEY_HOME;
    m_keycodes[0x152] = GLSG_KEY_INSERT;
    m_keycodes[0x15D] = GLSG_KEY_MENU;
    m_keycodes[0x151] = GLSG_KEY_PAGE_DOWN;
    m_keycodes[0x149] = GLSG_KEY_PAGE_UP;
    m_keycodes[0x045] = GLSG_KEY_PAUSE;
    m_keycodes[0x146] = GLSG_KEY_PAUSE;
    m_keycodes[0x039] = GLSG_KEY_SPACE;
    m_keycodes[0x00F] = GLSG_KEY_TAB;
    m_keycodes[0x03A] = GLSG_KEY_CAPS_LOCK;
    m_keycodes[0x145] = GLSG_KEY_NUM_LOCK;
    m_keycodes[0x046] = GLSG_KEY_SCROLL_LOCK;
    m_keycodes[0x03B] = GLSG_KEY_F1;
    m_keycodes[0x03C] = GLSG_KEY_F2;
    m_keycodes[0x03D] = GLSG_KEY_F3;
    m_keycodes[0x03E] = GLSG_KEY_F4;
    m_keycodes[0x03F] = GLSG_KEY_F5;
    m_keycodes[0x040] = GLSG_KEY_F6;
    m_keycodes[0x041] = GLSG_KEY_F7;
    m_keycodes[0x042] = GLSG_KEY_F8;
    m_keycodes[0x043] = GLSG_KEY_F9;
    m_keycodes[0x044] = GLSG_KEY_F10;
    m_keycodes[0x057] = GLSG_KEY_F11;
    m_keycodes[0x058] = GLSG_KEY_F12;
    m_keycodes[0x064] = GLSG_KEY_F13;
    m_keycodes[0x065] = GLSG_KEY_F14;
    m_keycodes[0x066] = GLSG_KEY_F15;
    m_keycodes[0x067] = GLSG_KEY_F16;
    m_keycodes[0x068] = GLSG_KEY_F17;
    m_keycodes[0x069] = GLSG_KEY_F18;
    m_keycodes[0x06A] = GLSG_KEY_F19;
    m_keycodes[0x06B] = GLSG_KEY_F20;
    m_keycodes[0x06C] = GLSG_KEY_F21;
    m_keycodes[0x06D] = GLSG_KEY_F22;
    m_keycodes[0x06E] = GLSG_KEY_F23;
    m_keycodes[0x076] = GLSG_KEY_F24;
    m_keycodes[0x038] = GLSG_KEY_LEFT_ALT;
    m_keycodes[0x01D] = GLSG_KEY_LEFT_CONTROL;
    m_keycodes[0x02A] = GLSG_KEY_LEFT_SHIFT;
    m_keycodes[0x15B] = GLSG_KEY_LEFT_SUPER;
    m_keycodes[0x137] = GLSG_KEY_PRINT_SCREEN;
    m_keycodes[0x138] = GLSG_KEY_RIGHT_ALT;
    m_keycodes[0x11D] = GLSG_KEY_RIGHT_CONTROL;
    m_keycodes[0x036] = GLSG_KEY_RIGHT_SHIFT;
    m_keycodes[0x15C] = GLSG_KEY_RIGHT_SUPER;
    m_keycodes[0x150] = GLSG_KEY_DOWN;
    m_keycodes[0x14B] = GLSG_KEY_LEFT;
    m_keycodes[0x14D] = GLSG_KEY_RIGHT;
    m_keycodes[0x148] = GLSG_KEY_UP;

    m_keycodes[0x052] = GLSG_KEY_KP_0;
    m_keycodes[0x04F] = GLSG_KEY_KP_1;
    m_keycodes[0x050] = GLSG_KEY_KP_2;
    m_keycodes[0x051] = GLSG_KEY_KP_3;
    m_keycodes[0x04B] = GLSG_KEY_KP_4;
    m_keycodes[0x04C] = GLSG_KEY_KP_5;
    m_keycodes[0x04D] = GLSG_KEY_KP_6;
    m_keycodes[0x047] = GLSG_KEY_KP_7;
    m_keycodes[0x048] = GLSG_KEY_KP_8;
    m_keycodes[0x049] = GLSG_KEY_KP_9;
    m_keycodes[0x04E] = GLSG_KEY_KP_ADD;
    m_keycodes[0x053] = GLSG_KEY_KP_DECIMAL;
    m_keycodes[0x135] = GLSG_KEY_KP_DIVIDE;
    m_keycodes[0x11C] = GLSG_KEY_KP_ENTER;
    m_keycodes[0x059] = GLSG_KEY_KP_EQUAL;
    m_keycodes[0x037] = GLSG_KEY_KP_MULTIPLY;
    m_keycodes[0x04A] = GLSG_KEY_KP_SUBTRACT;

    for (int scancode = 0; scancode < 512; scancode++) {
        if (m_keycodes[scancode] > 0) m_scancodes[m_keycodes[scancode]] = scancode;
    }
}
}  // namespace ara

#endif
#endif