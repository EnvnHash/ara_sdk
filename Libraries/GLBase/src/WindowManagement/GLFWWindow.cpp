//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//


#ifdef ARA_USE_GLFW

#include <Utils/Texture.h>
#include <WindowManagement/GLFWWindow.h>

#include <utility>

using namespace std;
using namespace glm;

namespace ara {

int GLFWWindow::init(const glWinPar &gp) {
    m_virtSize  = gp.size;
    m_monWidth   = 0;
    m_monHeight  = 0;

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
    m_workArea.x = std::numeric_limits<int>::max();  // left
    m_workArea.y = std::numeric_limits<int>::max();  // top
    m_workArea.z = 0;                                // right
    m_workArea.w = 0;                                // bottom

    int w, h, x, y;
    for (int i = 0; i < m_count; i++) {
        glfwGetMonitorWorkarea(m_monitors[i], &x, &y, &w, &h);
        if (x < m_workArea.x) {
            m_workArea[0] = x;
        }
        if (x + w > m_workArea.z) {
            m_workArea[2] = x + w;
        }
        if (y < m_workArea.y) {
            m_workArea[1] = y;
        }
        if (y + h > m_workArea.w) {
            m_workArea[3] = y + h;
        }
    }

    getMonitorScales();

    if (gp.fullScreen) {
        initFullScreen(gp);
    } else {
        initNonFullScreen(gp);
    }

    m_window = glfwCreateWindow(m_virtSize.x, m_virtSize.y, "", gp.fullScreen ? m_mon : nullptr, static_cast<GLFWwindow *>(gp.shareCont));
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
    m_nativeHandle = static_cast<void *>(wglGetCurrentContext());
#elif __linux__
#ifndef ARA_USE_GLES31
    m_nativeHandle = static_cast<void*>(glXGetCurrentContext());
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
    m_realSize = { fbWidth, fbHeight };
    m_posReal = { gp.shift.x * m_contentScale.x, gp.shift.y * m_contentScale.y };
#else
    m_realSize.x  = (int)(static_cast<float>(m_virtSize.x) * m_contentScale.x);
    m_realSize.y = (int)(static_cast<float>(m_virtSize.y) * m_contentScale.y);
    m_posReal   = { (int)(gp.shift.x * m_contentScale.x), (int)(gp.shift.y * m_contentScale.y) };
#endif

    glfwSetWindowPos(m_window, static_cast<int>(m_posReal.x), static_cast<int>(m_posReal.y));

    if (gp.debug) {
        LOG << "Vendor:  " << glGetString(GL_VENDOR);
        LOG << "Renderer: " << glGetString(GL_RENDERER);
        LOG << "Version:  " << glGetString(GL_VERSION);
        LOG << "GLSL:     " << glGetString(GL_SHADING_LANGUAGE_VERSION);
    }

    if (gp.nrSamples > 2) {
        glEnable(GL_MULTISAMPLE_ARB);
        if (gp.debug) {
            LOG << "enabling Multisampling";
        }
    }

    // init cursors
    for (auto cursor : m_mouseCursors) {
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
            win->onWindowSize(static_cast<int>(static_cast<float>(width) / win->getContentScale().x),
                              static_cast<int>(static_cast<float>(height) / win->getContentScale().y));
#else
            win->onWindowSize(width, height);
#endif
        };
        glfwSetWindowSizeCallback(m_window, windowSizeCb);

        auto mouseCursorCb = [](GLFWwindow *w, double xpos, double ypos) {
            auto win = static_cast<GLFWWindow *>(glfwGetWindowUserPointer(w));
#if defined(_WIN32) || defined(__linux__)
            win->onMouseCursor(xpos / static_cast<double>(win->getContentScale().x), ypos / static_cast<double>(win->getContentScale().y));
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

void GLFWWindow::initFullScreen(const glWinPar &gp) {
    if (gp.debug) {
        for (int i = 0; i < m_count; i++) {
            auto vm = glfwGetVideoMode(m_monitors[i]);
            LOG << "monitor " << i << " " << glfwGetMonitorName(m_monitors[i]) << " current video mode: width: " << vm->width << " height: " << vm->height << " refreshRate: " << vm->refreshRate;
        }
    }

    if (m_count > gp.monitorNr) {
        useMonitor = gp.monitorNr;
    }

    // get Video Modes
    int                countVm;
    bool               found       = false;
    const GLFWvidmode *modes       = glfwGetVideoModes(m_monitors[useMonitor], &countVm);
    const GLFWvidmode *useThisMode = nullptr;

    if (gp.debug) {
        for (int j = 0; j < countVm; j++) {
            printf("%i: current video mode: width: %i height: %i refreshRate: %i\n", j, modes[j].width, modes[j].height, modes[j].refreshRate);
        }
    }

    for (auto i = 0; i < countVm; i++) {
        if (modes[i].width == gp.size.x && modes[i].height == gp.size.y && modes[i].refreshRate == gp.refreshRate &&
            modes[i].redBits == 8) {
            found       = true;
            useThisMode = &modes[i];
        }
    }

    if (!found) {
        const GLFWvidmode *mode = glfwGetVideoMode(m_monitors[useMonitor]);
        if (gp.debug) {
            LOG << "current video mode: width: " << mode->width << " height: " <<  mode->height << "  refreshRate: " << mode->refreshRate;
        }
    } else {
        if (gp.debug) {
            LOG << "set video mode: width: " << useThisMode->width << " height: " << useThisMode->height << " refreshRate: " << useThisMode->refreshRate;
        }
    }

    m_mon = m_monitors[useMonitor];

    // get the video mode of the current selected monitor
    useThisMode = glfwGetVideoMode(m_mon);

    glfwWindowHint(GLFW_RED_BITS, useThisMode->redBits);
    glfwWindowHint(GLFW_GREEN_BITS, useThisMode->greenBits);
    glfwWindowHint(GLFW_BLUE_BITS, useThisMode->blueBits);
    glfwWindowHint(GLFW_REFRESH_RATE, useThisMode->refreshRate);
    glfwWindowHint(GLFW_SAMPLES, gp.nrSamples);

    monitorRefreshRate = useThisMode->refreshRate;
    m_virtSize.x       = useThisMode->width;
    m_virtSize.y       = useThisMode->height;
    m_monWidth         = useThisMode->width;
    m_monHeight        = useThisMode->height;
}

void GLFWWindow::initNonFullScreen(const glWinPar &gp) {
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

void GLFWWindow::initLibrary() {
    if (!glfwInit()) {
        printf("ERROR: Couldn't init glfw\n");
        exit(EXIT_FAILURE);
    }
}

void GLFWWindow::runLoop(const std::function<bool(double, double, int)>& f, bool eventBased, bool terminateGLFW,
                         bool destroyWinOnExit) {
    bool unlock      = false;
    m_drawFunc       = f;
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
    glViewport(0, 0, m_realSize.x, m_realSize.y);

    if (m_drawFunc(m_lastTime, 0, 0)) {
        glfwSwapBuffers(m_window);
    }
}

void GLFWWindow::startDrawThread(const std::function<bool(double, double, int)>& f) {
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
        if (width != m_virtSize.x || height != m_virtSize.y) glfwSetWindowSize(m_window, m_virtSize.x, m_virtSize.y);
        return;
    }
#endif

#ifdef __APPLE__
    m_virtSize.x  = (int)(static_cast<float>(width) * m_contentScale.x);
    m_virtSize.y = (int)(static_cast<float>(height) * m_contentScale.y);
#else
    m_virtSize.x  = width;
    m_virtSize.y = height;
#endif
    if (m_windowSizeCb) {
        m_windowSizeCb(width, height);
    }
}

void GLFWWindow::open() {
    glfwShowWindow(m_window);
    m_isOpen = true;
}

/** hide window, including removing it from the taskbar, dock or windowlist */
void GLFWWindow::hide() {
    glfwHideWindow(m_window);
    m_isOpen = false;
}

void GLFWWindow::makeCurrent() {
    if (m_window) {
        glfwMakeContextCurrent(m_window);
    }
}

void GLFWWindow::destroy() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
    m_exitSema.notify();
}

void GLFWWindow::destroy(bool terminate) {
    glfwDestroyWindow(m_window);
    if (terminate) {
        glfwTerminate();
    }
    m_exitSema.notify();
}

void GLFWWindow::resize(GLsizei width, GLsizei height)  {
    bool unlock = m_drawMtx.try_lock();
    glfwSetWindowSize(m_window, width, height);
    m_virtSize.x  = width;
    m_virtSize.y = height;
    if (unlock) {
        m_drawMtx.unlock();
    }
}

void GLFWWindow::focus() const {
    if (m_window) {
        glfwFocusWindow(m_window);
    }
}

void GLFWWindow::removeMouseCursors() {
    if (m_window) {
        for (auto it : m_mouseCursors | std::views::filter([](const auto it){ return it != nullptr; })) {
            glfwDestroyCursor(it);
        }
    }
}

void GLFWWindow::error_callback(int error, const char *description) {
    LOGE << "GLFW ERROR: " << description;
    fputs(description, stderr);
}

/// input in virtual pixels
void GLFWWindow::setSize(int inWidth, int inHeight) {
    m_virtSize.x  = inWidth;
    m_virtSize.y = inHeight;
    m_realSize.x  = static_cast<int>(static_cast<float>(inWidth) * m_contentScale.x);
    m_realSize.y = static_cast<int>(static_cast<float>(inHeight) * m_contentScale.y);
#ifdef __APPLE__
    glfwSetWindowSize(m_window, inWidth, inHeight);
#else
    // glfw calls immediately the corresponding callback without passing through glfwWaitEvents
    glfwSetWindowSize(m_window, m_realSize.x, m_realSize.y);
#endif
    iterate();
}

/// input in virtual pixels
void GLFWWindow::setPosition(int posx, int posy) {
    m_posVirt.x = static_cast<float>(posx);
    m_posVirt.y = static_cast<float>(posy);
    m_posReal.x = static_cast<float>(posx) * m_contentScale.x;
    m_posReal.y = static_cast<float>(posy) * m_contentScale.y;
#ifdef __APPLE__
    glfwSetWindowPos(m_window, m_posXvirt, m_posYvirt);
#else
    glfwSetWindowPos(m_window, static_cast<int>(m_posReal.x), static_cast<int>(m_posReal.y));
#endif
    iterate();
}

glm::vec2 GLFWWindow::getDpi() {
#ifdef __linux__
    auto   dpy = glfwGetX11Display();
    auto   scr = 0;
    double dDisplayDPI_H, dDisplayDPI_V;
    dDisplayDPI_H = (double)DisplayWidth(dpy, scr) / ((double)DisplayWidthMM(dpy, scr) / 25.4);
    dDisplayDPI_V = (double)DisplayHeight(dpy, scr) / ((double)DisplayHeightMM(dpy, scr) / 25.4);
    return glm::vec2{dDisplayDPI_H, dDisplayDPI_V};
#else
    return glm::vec2{92.f, 92.f};
#endif
}

glm::ivec2 GLFWWindow::getLastMousePos() {
    double xpos, ypos;
    glfwGetCursorPos(m_window, &xpos, &ypos);
#ifdef __APPLE__
    return {(int)xpos, (int)ypos};
#else
    return {static_cast<int>(xpos / m_contentScale.x), static_cast<int>(ypos / m_contentScale.y)};
#endif
}

glm::ivec2 GLFWWindow::getAbsMousePos() const {
    glm::ivec2 mp{};
    mouse::getAbsMousePos(mp.x, mp.y);
#ifdef __APPLE__
    return mp;
#else
    return glm::ivec2{static_cast<int>(static_cast<float>(mp.x) / m_contentScale.x), static_cast<int>(static_cast<float>(mp.y) / m_contentScale.y)};
#endif
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

    if ((m_lastTime - m_lastPrintFps) > 1000.0) {
        printf("FPS: %f dt: %fms\n", 1000.0 / m_medDt, m_medDt);
        m_lastPrintFps = m_lastTime;
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
ivec2 GLFWWindow::getSize()  {
    ivec2 size;
    glfwGetWindowSize(m_window, &size.x, &size.y);
#ifdef __APPLE__
    m_virtSize = size;
    m_realSize = size * m_contentScale;
#else
    m_realSize = size;
    m_virtSize = static_cast<ivec2>(static_cast<vec2>(size) / m_contentScale);
#endif
    return m_virtSize;
}

/// returns virtual coordinates
ivec2 GLFWWindow::getPosition() {
    ivec2 pos;
    glfwGetWindowPos(m_window, &pos.x, &pos.y);
#ifdef __APPLE__
    m_posReal = static_cast<vec2>(pos * m_contentScale);
    m_posVirt = static_cast<vec2>(pos);
#else
    m_posReal = static_cast<vec2>(pos);
    m_posVirt = static_cast<vec2>(pos) / m_contentScale;
#endif
    return m_posVirt;
}

void GLFWWindow::close() {
    if (m_isOpen) {
        glfwSetWindowShouldClose(m_window, GL_TRUE);
        m_isOpen = false;
    }
}

}  // namespace ara
#endif