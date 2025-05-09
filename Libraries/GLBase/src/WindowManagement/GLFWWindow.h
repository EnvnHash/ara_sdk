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

#pragma once

#include "GlbCommon/GlbCommon.h"

#ifdef ARA_USE_GLFW

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#elif __linux__
#define GLFW_EXPOSE_NATIVE_GLX
#define GLFW_EXPOSE_NATIVE_X11
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#ifdef _WIN32
#include <Windows/WindowsMousePos.h>
#include <wingdi.h>
#elif __linux__

#include <X11/X11MousePos.h>

#elif __APPLE__
#include <OsX/OSXMousePos.h>
#endif

#include <WindowManagement/GLWindowBase.h>

namespace ara {
class GLFWWindow : public GLWindowBase {
public:
    ~GLFWWindow() override = default;
    bool create(const glWinPar &gp) override { return init(gp); }
    int  init(const glWinPar &gp);

    /**
     * @param f the drawing function which will be executed or every iteration
     * @param eventBased choose whether the loop should run freely or stop and wait for the m_iterate signal
     * @param terminateGLFW shall GLFW be terminated when the loop exits? in case of multiple window it probably shouldn't
     * @param destroyWinOnExit destroy window on exit
     */
    void runLoop(std::function<bool(double, double, int)> f, bool eventBased = false, bool terminateGLFW = true,
                 bool destroyWinOnExit = true);

    void   startDrawThread(std::function<bool(double, double, int)> f);
    void   stopDrawThread();
    void   draw();
    double getFps();

    /** also sets the focus to the respective window */
    void open() override {
        glfwShowWindow(m_window);
        m_isOpen = true;
    }

    /** hide window, including removing it from the taskbar, dock or windowlist */
    void hide() {
        glfwHideWindow(m_window);
        m_isOpen = false;
    }

    void close() override;

    void makeCurrent() override {
        if (m_window) {
            glfwMakeContextCurrent(m_window);
        }
    }

    void swap() override { glfwSwapBuffers(m_window); }

    void destroy() override {
        glfwDestroyWindow(m_window);
        glfwTerminate();
        m_exitSema.notify();
    }

    void destroy(bool terminate) {
        glfwDestroyWindow(m_window);
        if (terminate) {
            glfwTerminate();
        }
        m_exitSema.notify();
    }

    void resize(GLsizei width, GLsizei height) override {
        bool unlock = m_drawMtx.try_lock();
        glfwSetWindowSize(m_window, width, height);
        m_widthVirt  = width;
        m_heightVirt = height;
        if (unlock) {
            m_drawMtx.unlock();
        }
    }

    void focus() const {
        if (m_window) {
            glfwFocusWindow(m_window);
        }
    }
    void minimize() override { glfwIconifyWindow(m_window); }

    void maximize() {
#ifdef __APPLE__
        // on macOS glfwMaximizeWindow doesn't work
        m_restoreWinPar.x = getPosition().x;
        m_restoreWinPar.y = getPosition().y;
        m_restoreWinPar.z = getSize().x;
        m_restoreWinPar.w = getSize().y;
        glfwSetWindowPos(m_window, 0, 0);
        resize(m_monWidth, m_monHeight);
#else
        glfwMaximizeWindow(m_window);
#endif
    }  /// must be called from main thread

    void restore() override {
#ifdef __APPLE__
        // on macOS glfwMaximizeWindow doesn't work
        glfwSetWindowPos(m_window, m_restoreWinPar.x, m_restoreWinPar.y);
        resize(m_restoreWinPar.z, m_restoreWinPar.w);
#else
        glfwRestoreWindow(m_window);
#endif
    }

    std::vector<GLFWvidmode>             getVideoModes();
    std::vector<std::pair<int, int>>     getMonitorOffsets();
    std::vector<std::pair<float, float>> getMonitorScales();
    glm::ivec2                           getSize();
    glm::ivec2                           getPosition();

    // GLFWcursor* createMouseCursor(const char* file, float xHot, float yHot);
    void          setMouseCursorIcon(GLFWcursor *icon, WinMouseIcon tp) { m_mouseCursors[toType(tp)] = icon; }
    bool          isOpen() const override { return m_isOpen; }
    bool          isMinimized() const { return glfwGetWindowAttrib(m_window, GLFW_ICONIFIED); }
    bool          isMaximized() const { return glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED); }
    bool          isHidBlocked() const { return m_hidBlocked; }
    int           getMonitorId() const { return useMonitor; }
    unsigned int  getMonitorWidth() const { return m_monWidth; }
    unsigned int  getMonitorHeight() const { return m_monHeight; }
    unsigned int  getWidth() const override { return m_widthVirt; }        /// in virtual pixels
    unsigned int  getHeight() const override { return m_heightVirt; }      /// in virtual pixels
    unsigned int  getWidthReal() const override { return m_widthReal; }    /// in real pixels
    unsigned int  getHeightReal() const override { return m_heightReal; }  /// in real pixels
    void         *getWin() override { return m_window; }
    GLFWwindow   *getCtx() const { return m_window; }
    GLFWmonitor **getMonitors() const { return m_monitors; }
    GLFWmonitor  *getMonitor(int i) const { return m_monitors[i]; }
    int           getNrMonitors() const { return m_count; }
    uint32_t      getPosX() const override { return static_cast<int>(m_posXvirt); }      /// in virtual pixels
    uint32_t      getPosY() const override { return static_cast<int>(m_posYvirt); }      /// in virtual pixels
    int           getPosXReal() const { return static_cast<int>(m_posXreal); }  /// in real pixels
    int           getPosYReal() const { return static_cast<int>(m_posYreal); }  /// in real pixels
    int           getFocus() const { return glfwGetWindowAttrib(m_window, GLFW_FOCUSED); }

    void *getNativeCtx() override { return m_nativeHandle; }

    /// return virtual pixels
    glm::ivec2 getLastMousePos() {
        double xpos, ypos;
        glfwGetCursorPos(m_window, &xpos, &ypos);
#ifdef __APPLE__
        return {(int)xpos, (int)ypos};
#else
        return {static_cast<int>(xpos / m_contentScale.x), static_cast<int>(ypos / m_contentScale.y)};
#endif
    }

    glm::ivec2 getAbsMousePos() const {
        glm::ivec2 mp{};
        mouse::getAbsMousePos(mp.x, mp.y);
#ifdef __APPLE__
        return mp;
#else
        return glm::ivec2{static_cast<int>(static_cast<float>(mp.x) / m_contentScale.x), static_cast<int>(static_cast<float>(mp.y) / m_contentScale.y)};
#endif
    }

#ifdef _WIN32
    HWND  getHwndHandle() const { return m_hwndHandle; }
    HGLRC getHglrcHandle() const { return m_wglHandle; }
#endif

    int         *getWorkArea() { return m_workArea; }
    Conditional *getIterateSema() { return &m_iterate; }
    bool         getBlockResizing() const { return m_blockResizing; }
    bool         getRequestOpen() const override { return m_requestOpen; }
    bool         getRequestClose() const override { return m_requestClose; }
    void         requestOpen(bool val) override { m_requestOpen = val; }
    void         requestClose(bool val) override { m_requestClose = val; }
    std::function<void()>& getOnCloseCb() { return m_onCloseCb; }

    static glm::vec2 getDpi() {
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

    /// input in virtual pixels
    void setSize(int inWidth, int inHeight) {
        m_widthVirt  = inWidth;
        m_heightVirt = inHeight;
        m_widthReal  = static_cast<int>(static_cast<float>(inWidth) * m_contentScale.x);
        m_heightReal = static_cast<int>(static_cast<float>(inHeight) * m_contentScale.y);
#ifdef __APPLE__
        glfwSetWindowSize(m_window, inWidth, inHeight);
#else
        // glfw calls immediately the corresponding callback without passing through glfwWaitEvents
        glfwSetWindowSize(m_window, m_widthReal, m_heightReal);
#endif
        iterate();
    }

    /// input in virtual pixels
    void setPosition(int posx, int posy) {
        m_posXvirt = static_cast<float>(posx);
        m_posYvirt = static_cast<float>(posy);
        m_posXreal = static_cast<float>(posx) * m_contentScale.x;
        m_posYreal = static_cast<float>(posy) * m_contentScale.y;
#ifdef __APPLE__
        glfwSetWindowPos(m_window, m_posXvirt, m_posYvirt);
#else
        glfwSetWindowPos(m_window, static_cast<int>(m_posXreal), static_cast<int>(m_posYreal));
#endif
        iterate();
    }

    void setVSync(bool set) override { glfwSwapInterval(set); }
    void setBlockResizing(bool val) { m_blockResizing = val; }
    void setBlockMouseIconSwitch(bool val) { m_blockMouseIconSwitch = val; }

    void setMouseCursor(WinMouseIcon iconTyp) const {
        if (!m_blockMouseIconSwitch) {
            glfwSetCursor(m_window, m_mouseCursors[toType(iconTyp)]);
        }
    }

    void setBlockHid(bool val) { m_hidBlocked = val; }
    static void setFloating(bool val) { glfwWindowHint(GLFW_FLOATING, val); }

    // utility methods for unified window handling (GLFWWindow -> GLWindow)
    void setKeyCallback(GLFWkeyfun f) const { glfwSetKeyCallback(m_window, f); }
    void setCharCallback(GLFWcharfun f) const { glfwSetCharCallback(m_window, f); }
    void setMouseButtonCallback(GLFWmousebuttonfun f) const { glfwSetMouseButtonCallback(m_window, f); }
    void setCursorPosCallback(GLFWcursorposfun f) const { glfwSetCursorPosCallback(m_window, f); }
    void setWindowSizeCallback(GLFWwindowsizefun f) const { glfwSetWindowSizeCallback(m_window, f); }
    void setWindowCloseCallback(GLFWwindowclosefun f) const { glfwSetWindowCloseCallback(m_window, f); }
    void setWindowMaximizeCallback(GLFWwindowmaximizefun f) const { glfwSetWindowMaximizeCallback(m_window, f); }
    void setWindowIconifyCallback(GLFWwindowiconifyfun f) const { glfwSetWindowIconifyCallback(m_window, f); }
    void setWindowFocusCallback(GLFWwindowfocusfun f) const { glfwSetWindowFocusCallback(m_window, f); }
    void setWindowPosCallback(GLFWwindowposfun f) const { glfwSetWindowPosCallback(m_window, f); }
    void setScrollCallback(GLFWscrollfun f) const { glfwSetScrollCallback(m_window, f); }
    void setWindowRefreshCallback(GLFWwindowrefreshfun f) const { glfwSetWindowRefreshCallback(m_window, f); }
    void setOnCloseCb(std::function<void()> f) { m_onCloseCb = std::move(f); }

    void onWindowSize(int width, int height) override;

    static void  pollEvents() { glfwPollEvents(); }
    static void  waitEvents() { glfwWaitEvents(); }
    static void  postEmptyEvent() { glfwPostEmptyEvent(); }
    static void  setErrorCallback(GLFWerrorfun f) { glfwSetErrorCallback(f); }
    static void *getWindowUserPointer(GLFWwindow *win) { return glfwGetWindowUserPointer(win); }
    
    static glm::vec2 getPrimaryMonitorWindowContentScale();

    static void initLibrary() {
        if (!glfwInit()) {
            printf("ERROR: Couldn't init glfw\n");
            exit(EXIT_FAILURE);
        }
    }

    static void terminateLibrary() { glfwTerminate(); }
    static void focusWin(GLFWwindow *win) { glfwFocusWindow(win); }
    void        removeMouseCursors() {
        if (m_window) {
            for (auto &it : m_mouseCursors) {
                if (it) {
                    glfwDestroyCursor(it);
                }
            }
        }
    }
    static void makeNoneCurrent() { glfwMakeContextCurrent(nullptr); }

    static void error_callback(int error, const char *description) {
        LOGE << "GLFW ERROR: " << description;
        fputs(description, stderr);
    }

protected:
    std::vector<GLFWvidmode>             m_modes;
    std::vector<std::pair<int, int>>     m_monOffsets;
    std::vector<std::pair<float, float>> m_monContScale;
    std::function<void()>                m_onCloseCb;

    int monitorRefreshRate{};
    int useMonitor = 0;
    int m_workArea[4]{};

    bool m_isOpen               = false;
    bool m_requestOpen          = false;
    bool m_requestClose         = false;
    bool m_blockResizing        = false;
    bool m_blockMouseIconSwitch = false;

    double m_medDt      = 0.066;
    double m_lastTime   = 0;
    double printFpsIntv = 2.0;
    double lastPrintFps = 0.0;

    int   m_count     = 0;
    int   m_monWidth  = 0;
    int   m_monHeight = 0;
    float m_posXvirt  = 0;
    float m_posYvirt  = 0;
    float m_posXreal  = 0;
    float m_posYreal  = 0;

    void *m_nativeHandle = nullptr;

#ifdef _WIN32
    HWND  m_hwndHandle = nullptr;
    HGLRC m_wglHandle  = nullptr;
#endif
    GLFWwindow   *m_window   = nullptr;
    GLFWmonitor **m_monitors = nullptr;
    GLFWmonitor  *m_mon      = nullptr;
    GLFWcursor   *m_mouseCursors[toType(WinMouseIcon::count)]{};

    glm::ivec4 m_restoreWinPar{0};
    std::mutex m_drawMtx;
};
}  // namespace ara
#endif
