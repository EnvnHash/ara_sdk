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
    void initFullScreen(const glWinPar &gp);
    void initNonFullScreen(const glWinPar &gp);

    /**
     * @param f the drawing function which will be executed or every iteration
     * @param eventBased choose whether the loop should run freely or stop and wait for the m_iterate signal
     * @param terminateGLFW shall GLFW be terminated when the loop exits? in case of multiple window it probably shouldn't
     * @param destroyWinOnExit destroy window on exit
     */
    void    runLoop(const std::function<bool(double, double, int)>& f, bool eventBased = false, bool terminateGLFW = true,
                 bool destroyWinOnExit = true);
    void    execGlCb();
    void    cleanUp(bool terminateGLFW, bool destroyWinOnExit);
    void    startDrawThread(const std::function<bool(double, double, int)>& f);
    void    stopDrawThread();
    void    draw();
    double  getFps();

    /** also sets the focus to the respective window */
    void open() override;
    /** hide window, including removing it from the taskbar, dock or windowlist */
    void hide();
    void close() override;
    void makeCurrent() override;
    void swap() override { glfwSwapBuffers(m_window); }
    void destroy() override;
    void destroy(bool terminate);
    void resize(GLsizei width, GLsizei height) override;
    void focus() const;
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
    void         *getWin() override { return m_window; }
    GLFWwindow   *getCtx() const { return m_window; }
    GLFWmonitor **getMonitors() const { return m_monitors; }
    GLFWmonitor  *getMonitor(int i) const { return m_monitors[i]; }
    int           getNrMonitors() const { return m_count; }
    uint32_t      getPosX() const override { return static_cast<int>(m_posVirt.x); }      /// in virtual pixels
    uint32_t      getPosY() const override { return static_cast<int>(m_posVirt.y); }      /// in virtual pixels
    int           getPosXReal() const { return static_cast<int>(m_posReal.x); }  /// in real pixels
    int           getPosYReal() const { return static_cast<int>(m_posReal.y); }  /// in real pixels
    int           getFocus() const { return glfwGetWindowAttrib(m_window, GLFW_FOCUSED); }
    void*         getNativeCtx() override { return m_nativeHandle; }
    /// return virtual pixels
    glm::ivec2    getLastMousePos();
    glm::ivec2    getAbsMousePos() const;
#ifdef _WIN32
    HWND  getHwndHandle() const { return m_hwndHandle; }
    HGLRC getHglrcHandle() const { return m_wglHandle; }
#endif
    auto        getIterateSema() { return &m_iterate; }
    auto        getBlockResizing() const { return m_blockResizing; }
    bool        getRequestOpen() const override { return m_requestOpen; }
    bool        getRequestClose() const override { return m_requestClose; }
    auto&       getOnCloseCb() { return m_onCloseCb; }

    void requestOpen(bool val) override { m_requestOpen = val; }
    void requestClose(bool val) override { m_requestClose = val; }

    static glm::vec2 getDpi();

    /// input in virtual pixels
    void setSize(int inWidth, int inHeight);
    /// input in virtual pixels
    void setPosition(int posx, int posy);
    void setVSync(bool set) override { glfwSwapInterval(set); }
    void setBlockResizing(bool val) { m_blockResizing = val; }
    void setBlockMouseIconSwitch(bool val) { m_blockMouseIconSwitch = val; }

    void setMouseCursor(WinMouseIcon iconTyp) const {
        if (!m_blockMouseIconSwitch) {
            glfwSetCursor(m_window, m_mouseCursors[toType(iconTyp)]);
        }
    }

    void setBlockHid(const bool val) { m_hidBlocked = val; }
    static void setFloating(const bool val) { glfwWindowHint(GLFW_FLOATING, val); }

    // utility methods for unified window handling (GLFWWindow -> GLWindow)
    void setKeyCallback(const GLFWkeyfun& f) const { glfwSetKeyCallback(m_window, f); }
    void setCharCallback(const GLFWcharfun& f) const { glfwSetCharCallback(m_window, f); }
    void setMouseButtonCallback(const GLFWmousebuttonfun& f) const { glfwSetMouseButtonCallback(m_window, f); }
    void setCursorPosCallback(const GLFWcursorposfun& f) const { glfwSetCursorPosCallback(m_window, f); }
    void setWindowSizeCallback(const GLFWwindowsizefun& f) const { glfwSetWindowSizeCallback(m_window, f); }
    void setWindowCloseCallback(const GLFWwindowclosefun& f) const { glfwSetWindowCloseCallback(m_window, f); }
    void setWindowMaximizeCallback(const GLFWwindowmaximizefun& f) const { glfwSetWindowMaximizeCallback(m_window, f); }
    void setWindowIconifyCallback(const GLFWwindowiconifyfun& f) const { glfwSetWindowIconifyCallback(m_window, f); }
    void setWindowFocusCallback(const GLFWwindowfocusfun& f) const { glfwSetWindowFocusCallback(m_window, f); }
    void setWindowPosCallback(const GLFWwindowposfun& f) const { glfwSetWindowPosCallback(m_window, f); }
    void setScrollCallback(const GLFWscrollfun& f) const { glfwSetScrollCallback(m_window, f); }
    void setWindowRefreshCallback(const GLFWwindowrefreshfun& f) const { glfwSetWindowRefreshCallback(m_window, f); }
    void setOnCloseCb(const std::function<void()>& f) { m_onCloseCb = f; }
    void onWindowSize(int width, int height) override;

    static void  pollEvents() { glfwPollEvents(); }
    static void  waitEvents() { glfwWaitEvents(); }
    static void  postEmptyEvent() { glfwPostEmptyEvent(); }
    static void  setErrorCallback(const GLFWerrorfun& f) { glfwSetErrorCallback(f); }
    static void *getWindowUserPointer(GLFWwindow *win) { return glfwGetWindowUserPointer(win); }
    
    static glm::vec2 getPrimaryMonitorWindowContentScale();

    static void initLibrary();
    static void terminateLibrary() { glfwTerminate(); }
    static void focusWin(GLFWwindow *win) { glfwFocusWindow(win); }
    void        removeMouseCursors();
    static void error_callback(int error, const char *description);

    static void makeNoneCurrent() {
        glfwMakeContextCurrent(nullptr);
    }

protected:
    std::vector<GLFWvidmode>             m_modes;
    std::vector<std::pair<int, int>>     m_monOffsets;
    std::vector<std::pair<float, float>> m_monContScale;
    std::function<void()>                m_onCloseCb;

    int monitorRefreshRate{};
    int useMonitor = 0;

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

    glm::vec2 m_posVirt{};
    glm::vec2 m_posReal{};

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
