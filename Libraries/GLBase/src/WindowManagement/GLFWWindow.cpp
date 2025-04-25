//
// Created by user on 26.11.2020.
//

#ifdef ARA_USE_GLFW

#include <Utils/Texture.h>
#include <WindowManagement/GLFWWindow.h>

#include <utility>

using namespace std;

namespace ara {

int GLFWWindow::init(glWinPar &gp) {
    m_widthVirt  = gp.width;
    m_heightVirt = gp.height;
    m_monWidth   = 0;
    m_monHeight  = 0;
#ifdef ARA_DEBUG
    gp.debug = true;
#endif

    glfwSetErrorCallback(error_callback);

    // Initialize the library
    if (gp.doInit && !glfwInit()) {
        LOGE << "GLFW init failed!!!!";
        return true;
    }

    // get all the information about the m_monitors
    m_monitors = glfwGetMonitors(&m_count);
    if (gp.doInit) {
        getVideoModes();
        getMonitorOffsets();
    }

    // get the bounding box around all displays
    m_workArea[0] = std::numeric_limits<int>::max();  // left
    m_workArea[1] = std::numeric_limits<int>::max();  // top
    m_workArea[2] = 0;                                // right
    m_workArea[3] = 0;                                // bottom

    {
        int w, h, x, y;
        for (int i = 0; i < m_count; i++) {
            glfwGetMonitorWorkarea(m_monitors[i], &x, &y, &w, &h);
            if (x < m_workArea[0]) {
                m_workArea[0] = x;
            }
            if (x + w > m_workArea[2]) {
                m_workArea[2] = x + w;
            }
            if (y < m_workArea[1]) {
                m_workArea[1] = y;
            }
            if (y + h > m_workArea[3]) {
                m_workArea[3] = y + h;
            }
        }
    }

    getMonitorScales();

    if (gp.fullScreen) {
        printf(" Window running in fullscreen mode \n");
        if (gp.debug) {
            for (int i = 0; i < m_count; i++) {
                printf("monitor %i: %s current video mode: width: %i height: %i "
                       "refreshRate: %i\n",
                       i, glfwGetMonitorName(m_monitors[i]), glfwGetVideoMode(m_monitors[i])->width,
                       glfwGetVideoMode(m_monitors[i])->height, glfwGetVideoMode(m_monitors[i])->refreshRate);
            }
        }

        if (m_count > gp.monitorNr) {
            useMonitor = gp.monitorNr;
        }

        // get Video Modes
        int                countVm;
        bool               found       = false;
        const GLFWvidmode *modes       = glfwGetVideoModes(m_monitors[useMonitor], &countVm);
        const GLFWvidmode *useThisMode = 0;

        if (gp.debug) {
            for (int j = 0; j < countVm; j++) {
                printf("%i: current video mode: width: %i height: %i refreshRate: %i\n",
                        j, modes[j].width, modes[j].height, modes[j].refreshRate);
            }
        }

        for (auto i = 0; i < countVm; i++) {
            if (modes[i].width == gp.width && modes[i].height == gp.height && modes[i].refreshRate == gp.refreshRate &&
                modes[i].redBits == 8) {
                found       = true;
                useThisMode = &modes[i];
            }
        }

        if (!found) {
            const GLFWvidmode *mode = glfwGetVideoMode(m_monitors[useMonitor]);
            if (gp.debug) {
                printf("current video mode: width: %i height: %i refreshRate: %i\n",
                    mode->width, mode->height, mode->refreshRate);
                printf("using unsupported videomode! \n");
            }
        } else {
            if (gp.debug) {
                printf("set video mode: width: %i height: %i refreshRate: %i\n", useThisMode->width,
                       useThisMode->height, useThisMode->refreshRate);
            }

            m_widthVirt  = useThisMode->width;
            m_heightVirt = useThisMode->height;
            glfwWindowHint(GLFW_RED_BITS, useThisMode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, useThisMode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, useThisMode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, useThisMode->refreshRate);
            monitorRefreshRate = useThisMode->refreshRate;
        }

        m_mon = m_monitors[useMonitor];

        // get the video mode of the current selected monitor
        const GLFWvidmode *mode = glfwGetVideoMode(m_mon);

        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        glfwWindowHint(GLFW_SAMPLES, gp.nrSamples);

        monitorRefreshRate = mode->refreshRate;
        m_widthVirt        = mode->width;
        m_heightVirt       = mode->height;
        m_monWidth         = mode->width;
        m_monHeight        = mode->height;

    } else {
        if (gp.debug) {
            for (int i = 0; i < m_count; i++) {
                printf( "Window monitor %i: %s current video mode: width: %i height: %i refreshRate: %i scaleY: %f scaleY: %f "
                        "decorated: %d floating: %d \n",
                        i, glfwGetMonitorName(m_monitors[i]), glfwGetVideoMode(m_monitors[i])->width,
                        glfwGetVideoMode(m_monitors[i])->height, glfwGetVideoMode(m_monitors[i])->refreshRate,
                        m_monContScale[i].first, m_monContScale[i].second, gp.decorated, gp.floating);
            }
        }

        monitorRefreshRate = 60;

        // for non-fullscreen to always stay on top
        glfwWindowHint(GLFW_DECORATED, gp.decorated ? GL_TRUE : GL_FALSE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_FOCUSED, GL_TRUE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, gp.transparentFramebuffer);
        glfwWindowHint(GLFW_REFRESH_RATE, monitorRefreshRate);
        glfwWindowHint(GLFW_SAMPLES, gp.nrSamples);
        glfwWindowHint(GLFW_RESIZABLE, gp.resizeable);
#if defined(_WIN32) || defined(__linux__)
        glfwWindowHint(GLFW_SCALE_TO_MONITOR,
                       gp.scaleToMonitor ? GL_TRUE : GL_FALSE);  // if GL_FALSE pixel sizing is 1:1 if GL_TRUE the
                                                                 // required size will be different from the
                                                                 // resulting window size
#endif

#ifdef __APPLE__
        glfwWindowHint(GLFW_SCALE_TO_MONITOR,
                       GL_TRUE);  // if GL_FALSE pixel sizing is 1:1 if GL_TRUE
                                  // the required size will be different from
                                  // the resulting window size
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // uncomment this statement to fix compilation on OS X
        glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GL_FALSE);  // uncomment this statement to fix compilation on OS X
#endif

#ifndef __EMSCRIPTEN__
        glfwWindowHint(GLFW_FLOATING, gp.floating);  // ignored for fullscreen, necessary
#endif
        const GLFWvidmode *mode = glfwGetVideoMode(m_monitors[0]);
        m_monWidth              = mode->width;
        m_monHeight             = mode->height;
    }

    m_window = glfwCreateWindow(m_widthVirt, m_heightVirt, "", gp.fullScreen ? m_mon : nullptr, (GLFWwindow *)gp.shareCont);
    if (!m_window) {
        LOGE << " GWindow ERROR creating window";
        return false;
    }

    int fbWidth = 0, fbHeight = 0;
    glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);

#ifdef _WIN32
    m_hwndHandle = glfwGetWin32Window(m_window);
    m_wglHandle  = glfwGetWGLContext(m_window);
#endif

    if (gp.fullScreen) {
        glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    }

    // need for hiding the cursor during runtime
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    // creation of new context crashes when using shareCont
    if (!m_window) {
        return false;
    }

    glfwMakeContextCurrent(m_window);

#ifdef _WIN32
    m_nativeHandle = (void *)wglGetCurrentContext();
#elif __linux__
#ifndef ARA_USE_GLES31
    m_nativeHandle = (void *)glXGetCurrentContext();
#endif
#endif

    if (!gp.fullScreen && !gp.createHidden) {
        open();
    }

    // get content relative scaling = system default dpi to monitor dpi
#if defined(_WIN32) || defined __linux__
    if (gp.scaleToMonitor) {
        glfwGetWindowContentScale(m_window, &m_contentScale.x, &m_contentScale.y);
    }
#endif

#ifndef __APPLE__
    // if there is content scaling on this display, adjust size and position
    m_widthReal  = fbWidth;
    m_heightReal = fbHeight;
    m_posXreal   = gp.shiftX * m_contentScale.x;
    m_posYreal   = gp.shiftY * m_contentScale.y;
#else
    m_widthReal  = (int)((float)m_widthVirt * m_contentScale.x);
    m_heightReal = (int)((float)m_heightVirt * m_contentScale.y);
    m_posXreal   = (int)(gp.shiftX * m_contentScale.x);
    m_posYreal   = (int)(gp.shiftY * m_contentScale.y);
#endif

    glfwSetWindowPos(m_window, static_cast<int>(m_posXreal), static_cast<int>(m_posYreal));

    if (gp.debug) {
        LOG << "Vendor:  " << glGetString(GL_VENDOR);
        LOG << "Renderer: " << glGetString(GL_RENDERER);
        LOG << "Version:  " << glGetString(GL_VERSION);
        LOG << "GLSL:     " << glGetString(GL_SHADING_LANGUAGE_VERSION);
    }

    if (gp.nrSamples > 2) {
        glEnable(GL_MULTISAMPLE_ARB);
        if (gp.debug) {
            printf("enabling Multisampling \n");
        }
    }

    // init cursors
    for (auto & cursor : m_mouseCursors) {
        cursor = nullptr;
    }

    // create std cursors
    m_mouseCursors[toType(WinMouseIcon::arrow)]     = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    m_mouseCursors[toType(WinMouseIcon::hresize)]   = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    m_mouseCursors[toType(WinMouseIcon::vresize)]   = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    m_mouseCursors[toType(WinMouseIcon::crossHair)] = glfwCreateStandardCursor(GLFW_CROSSHAIR_CURSOR);

    // if this window is an instance without the GLFWWindowManager, hid callback functions are set here unfortunately
    // passing this classes member function as c-pointer would require them to be static what would mean that a
    // second instance will overwrite them. To solve this problem, lambdas are used inside which a static cast happens
    if (gp.hidInput && !gp.hidExtern) {
        glfwSetWindowUserPointer(m_window, this);

        auto keyCb = [](GLFWwindow *w, int key, int scancode, int action, int mods) {
            static_cast<GLFWWindow *>(glfwGetWindowUserPointer(w))->onKey(key, scancode, action, mods);
        };
        glfwSetKeyCallback(m_window, keyCb);

        auto charCb = [](GLFWwindow *w, unsigned int codepoint) {
            static_cast<GLFWWindow *>(glfwGetWindowUserPointer(w))->onChar(codepoint);
        };
        glfwSetCharCallback(m_window, charCb);

        auto mouseButCb = [](GLFWwindow *w, int button, int action, int mods) {
            static_cast<GLFWWindow *>(glfwGetWindowUserPointer(w))->onMouseButton(button, action, mods);
        };
        glfwSetMouseButtonCallback(m_window, mouseButCb);

        auto scrollCb = [](GLFWwindow *w, double xpos, double ypos) {
            static_cast<GLFWWindow *>(glfwGetWindowUserPointer(w))->onScroll(xpos, ypos);
        };
        glfwSetScrollCallback(m_window, scrollCb);

        auto windowCloseCb = [](GLFWwindow *w) {
            static_cast<GLFWWindow *>(glfwGetWindowUserPointer(w))->onWindowClose();
        };
        glfwSetWindowCloseCallback(m_window, windowCloseCb);

        auto windowMaxCb = [](GLFWwindow *w, int flag) {
            static_cast<GLFWWindow *>(glfwGetWindowUserPointer(w))->onWindowMaximize(flag);
        };
        glfwSetWindowMaximizeCallback(m_window, windowMaxCb);

        auto windowIconifyCb = [](GLFWwindow *w, int flag) {
            static_cast<GLFWWindow *>(glfwGetWindowUserPointer(w))->onWindowIconify(flag);
        };
        glfwSetWindowIconifyCallback(m_window, windowIconifyCb);

        auto windowFocusCb = [](GLFWwindow *w, int flag) {
            static_cast<GLFWWindow *>(glfwGetWindowUserPointer(w))->onWindowFocus(flag);
        };
        glfwSetWindowFocusCallback(m_window, windowFocusCb);

        auto windowSizeCb = [](GLFWwindow *w, int width, int height) {
            auto win = static_cast<GLFWWindow *>(glfwGetWindowUserPointer(w));
#if defined(_WIN32) || defined(__linux__)
            win->onWindowSize((int)((float)width / win->getContentScale().x),
                              (int)((float)height / win->getContentScale().y));
#else
            win->onWindowSize(width, height);
#endif
        };
        glfwSetWindowSizeCallback(m_window, windowSizeCb);

        auto mouseCursorCb = [](GLFWwindow *w, double xpos, double ypos) {
            auto win = static_cast<GLFWWindow *>(glfwGetWindowUserPointer(w));
#if defined(_WIN32) || defined(__linux__)
            win->onMouseCursor(xpos / (double)win->getContentScale().x, ypos / (double)win->getContentScale().y);
#else
            win->onMouseCursor(xpos, ypos);
#endif
        };
        glfwSetCursorPosCallback(m_window, mouseCursorCb);
    }

    m_inited = true;
    m_initSema.notify();

    return true;
}

void GLFWWindow::runLoop(std::function<bool(double, double, int)> f, bool eventBased, bool terminateGLFW,
                         bool destroyWinOnExit) {
    bool unlock      = false;
    m_drawFunc       = std::move(f);
    m_run            = true;
    m_eventBasedLoop = eventBased;

    glfwMakeContextCurrent(m_window);

    // Loop until the user closes the window
    while (!glfwWindowShouldClose(m_window) && m_run) {
        if (eventBased && m_initSignaled && !m_forceRedraw) {
            m_iterate.wait(0);
        }

        m_forceRedraw = false;

        // don't draw when iconified
        if (!glfwGetWindowAttrib(m_window, GLFW_ICONIFIED)) {
            draw();
        }

        if (m_run && m_glCb) {
            m_glCb();
            m_glCb = nullptr;
        }

        if (eventBased && !m_initSignaled) {
            m_initSignaled = true;
            m_glInitedSema.notify();
        }
    }

    m_run = false;

    if (m_onCloseCb) {
        m_onCloseCb();
    }
    if (destroyWinOnExit) {
        glfwDestroyWindow(m_window);
    }
    if (terminateGLFW) {
        glfwTerminate();
    }

    m_exitSignal.notify();
}

void GLFWWindow::draw() {
    m_lastTime = glfwGetTime();
    glViewport(0, 0, m_widthReal, m_heightReal);

    if (m_drawFunc(m_lastTime, 0, 0)) {
        glfwSwapBuffers(m_window);
    }
}

void GLFWWindow::startDrawThread(std::function<bool(double, double, int)> f) {
    if (!isRunning()) {
        m_drawThread = std::thread(&GLFWWindow::runLoop, this, f, true, false, false);
        m_drawThread.detach();
    }
}

/** note: must be called from a different thread than the window's draw thread in order to avoid dead-locking */
void GLFWWindow::stopDrawThread() {
    if (m_run) {
        m_run = false;
        close();
        m_iterate.notify();
        m_exitSignal.wait(0);
    }
}

void GLFWWindow::onWindowSize(int width, int height) {
    // when moving the window very fast between m_monitors
    // on Windows, resize calls are sent from the OS
    // be sure that the window doesn't change it's size if this is not desired
#ifdef _WIN32
    if (m_blockResizing) {
        if (width != m_widthVirt || height != m_heightVirt) glfwSetWindowSize(m_window, m_widthVirt, m_heightVirt);
        return;
    }
#endif

#ifdef __APPLE__
    m_widthVirt  = (int)((float)width * m_contentScale.x);
    m_heightVirt = (int)((float)height * m_contentScale.y);
#else
    m_widthVirt  = width;
    m_heightVirt = height;
#endif
    if (m_windowSizeCb) {
        m_windowSizeCb(width, height);
    }
}

double GLFWWindow::getFps() {
#ifdef USE_WEBGL
    // update time counter, get dt and smooth it
    if (m_medDt == 0.066) {
        if (m_lastTime != 0.0) m_medDt = emscripten_get_now() - m_lastTime;
    } else {
        m_medDt = ((emscripten_get_now() - m_lastTime) + (m_medDt * 30.0)) / 31.0;
    }

    m_lastTime = emscripten_get_now();

    if ((m_lastTime - lastPrintFps) > 1000.0) {
        printf("FPS: %f dt: %fms\n", 1000.0 / m_medDt, m_medDt);
        lastPrintFps = m_lastTime;
    }
#else
    // update time counter, get dt and smooth it
    if (m_medDt == 0.066) {
        if (m_lastTime != 0.0) m_medDt = glfwGetTime() - m_lastTime;
    } else {
        m_medDt = ((glfwGetTime() - m_lastTime) + (m_medDt * 30.0)) / 31.0;
    }

    m_lastTime = glfwGetTime();

    if ((m_lastTime - lastPrintFps) > printFpsIntv) {
        LOG << "FPS: " << 1.0 / m_medDt << " dt: " << m_medDt;
        lastPrintFps = m_lastTime;
    }
    return 1.0 / m_medDt;
#endif
}

std::vector<GLFWvidmode> GLFWWindow::getVideoModes() {
    if (m_monOffsets.empty()) {
        for (int i = 0; i < m_count; i++) {
            const GLFWvidmode *thisMode = glfwGetVideoMode(m_monitors[i]);
            m_modes.emplace_back();
            m_modes.back().width  = thisMode->width;
            m_modes.back().height = thisMode->height;
        }
    }

    return m_modes;
}

std::vector<std::pair<int, int>> GLFWWindow::getMonitorOffsets() {
    if (m_monOffsets.empty()) {
        for (int i = 0; i < m_count; i++) {
            int monOffsX, monOffsY;
            glfwGetMonitorPos(m_monitors[i], &monOffsX, &monOffsY);

            m_monOffsets.emplace_back(monOffsX, monOffsY);
        }
    }
    return m_monOffsets;
}

std::vector<std::pair<float, float>> GLFWWindow::getMonitorScales() {
    if (m_monContScale.empty()) {
        for (int i = 0; i < m_count; i++) {
            float xScale, yScale;
            glfwGetMonitorContentScale(m_monitors[i], &xScale, &yScale);
            m_monContScale.emplace_back(xScale, yScale);
        }
    }
    return m_monContScale;
}

glm::vec2 GLFWWindow::getPrimaryMonitorWindowContentScale() {
    glm::vec2 contentScale{};

    if (!glfwInit()) {
        LOGE << "GLFW init failed!!!!";
        return {};
    }

    auto window = glfwCreateWindow(50, 50, "", nullptr, nullptr);
    if (!window) {
        LOGE << " GWindow ERROR creating window";
        return {};
    }

    glfwMakeContextCurrent(window);

    // get content relative scaling = system default dpi to monitor dpi
#if defined(_WIN32) || defined __linux__
    glfwGetWindowContentScale(window, &contentScale.x, &contentScale.y);
#endif
    glfwDestroyWindow(window);

    return contentScale;
}

/// returns virtual coordinates
glm::ivec2 GLFWWindow::getSize()  {
    glm::ivec2 size;
    glfwGetWindowSize(m_window, &size.x, &size.y);
#ifdef __APPLE__
    m_widthVirt  = size.x;
    m_heightVirt = size.y;
    m_widthReal  = size.x * m_contentScale.x;
    m_heightReal = size.y * m_contentScale.y;
#else
    m_widthReal  = size.x;
    m_heightReal = size.y;
    m_widthVirt  = static_cast<uint32_t>(size.x / m_contentScale.x);
    m_heightVirt = static_cast<uint32_t>(size.y / m_contentScale.y);
#endif
    return glm::ivec2{static_cast<int>(m_widthVirt), static_cast<int>(m_heightVirt)};
}

/// returns virtual coordinates
glm::ivec2 GLFWWindow::getPosition() {
    glm::ivec2 pos;
    glfwGetWindowPos(m_window, &pos.x, &pos.y);
#ifdef __APPLE__
    m_posXreal = static_cast<float>(pos.x * m_contentScale.x);
    m_posYreal = static_cast<float>(pos.y * m_contentScale.y);
    m_posXvirt = static_cast<float>(pos.x);
    m_posYvirt = static_cast<float>(pos.y);
#else
    m_posXreal = static_cast<float>(pos.x);
    m_posYreal = static_cast<float>(pos.y);
    m_posXvirt = static_cast<float>(pos.x) / m_contentScale.x;
    m_posYvirt = static_cast<float>(pos.y) / m_contentScale.y;
#endif
    return glm::ivec2{static_cast<int>(m_posXvirt), static_cast<int>(m_posYvirt)};
}

void GLFWWindow::close() {
    if (m_isOpen) {
        glfwSetWindowShouldClose(m_window, GL_TRUE);
        m_isOpen = false;
    }
}
}  // namespace ara
#endif