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

using glfwAssignWinCbFunc = std::variant<
    std::function<void(GLFWwindow*, void(*)(GLFWwindow*, int, int, int, int)) >,
    std::function<void(GLFWwindow*, void(*)(GLFWwindow*, int, int, int)) >,
    std::function<void(GLFWwindow*, void(*)(GLFWwindow*, int, int)) >,
    std::function<void(GLFWwindow*, void(*)(GLFWwindow*, int)) >,
    std::function<void(GLFWwindow*, void(*)(GLFWwindow*, unsigned int)) >,
    std::function<void(GLFWwindow*, void(*)(GLFWwindow*, double, double)) >,
    std::function<void(GLFWwindow*, void(*)(GLFWwindow*)) >
>;

class GLFWWindow : public GLWindowBase {
public:
    ~GLFWWindow() override = default;
    bool create(const glWinPar &gp) override { return init(gp); }
    bool init(const glWinPar &gp);
    void initFullScreen(const glWinPar &gp);
    void initNonFullScreen(const glWinPar &gp);

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
    void maximize();  /// must be called from main thread
    void restore() override;

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
    auto    getWorkArea() { return m_workArea; }
    auto    getIterateSema() { return &m_iterate; }
    auto    getBlockResizing() const { return m_blockResizing; }
    bool    getRequestOpen() const override { return m_requestOpen; }
    bool    getRequestClose() const override { return m_requestClose; }
    auto&   getOnCloseCb() { return m_onCloseCb; }

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
    void setMouseCursor(WinMouseIcon iconTyp) const;
    void setBlockHid(bool val) { m_hidBlocked = val; }
    static void setFloating(bool val) { glfwWindowHint(GLFW_FLOATING, val); }

    template <typename... Args>
    static void onWinHidFromCtx(GLFWwindow *w, const winCb& key, Args&&... args) {
        // adjust parameters here
        static_cast<GLFWWindow*>(glfwGetWindowUserPointer(w))->onWinHid(key, std::forward<Args>(args)...);
    }

    void setWinCallback(winCb tp, const winHidCb& f) override;
    void setOnCloseCb(std::function<void()> f) { m_onCloseCb = std::move(f); }
    void onWindowSize(int width, int height);

    static void  pollEvents() { glfwPollEvents(); }
    static void  waitEvents() { glfwWaitEvents(); }
    static void  postEmptyEvent() { glfwPostEmptyEvent(); }
    static void  setErrorCallback(GLFWerrorfun f) { glfwSetErrorCallback(f); }
    static void *getWindowUserPointer(GLFWwindow *win) { return glfwGetWindowUserPointer(win); }
    static glm::vec2 getPrimaryMonitorWindowContentScale();

    static void initLibrary();
    static void terminateLibrary() { glfwTerminate(); }
    static void focusWin(GLFWwindow *win) { glfwFocusWindow(win); }
    void        removeMouseCursors();
    static void error_callback(int error, const char *description);
    static void makeNoneCurrent() { glfwMakeContextCurrent(nullptr); }

protected:
    std::vector<GLFWvidmode>             m_modes;
    std::vector<std::pair<int, int>>     m_monOffsets;
    std::vector<std::pair<float, float>> m_monContScale;
    std::function<void()>                m_onCloseCb;

    int monitorRefreshRate{};
    int useMonitor = 0;
    glm::ivec4 m_workArea{};

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

    static inline std::array<glfwAssignWinCbFunc, toType(winCb::Size)> m_setGlfwCbMap {
        [](GLFWwindow* w, void (*f)(GLFWwindow*, int, int, int, int)){ glfwSetKeyCallback(w, f); },
        [](GLFWwindow* w, void (*f)(GLFWwindow*, unsigned int)){ glfwSetCharCallback(w, f); },
        [](GLFWwindow* w, void (*f)(GLFWwindow*, int, int, int)){ glfwSetMouseButtonCallback(w, f); },
        [](GLFWwindow* w, void (*f)(GLFWwindow*, double, double)){ glfwSetCursorPosCallback(w, f); },
        [](GLFWwindow* w, void (*f)(GLFWwindow*, int, int)){ glfwSetWindowSizeCallback(w, f); },
        [](GLFWwindow* w, void (*f)(GLFWwindow*)){ glfwSetWindowCloseCallback(w, f); },
        [](GLFWwindow* w, void (*f)(GLFWwindow*, int)){ glfwSetWindowMaximizeCallback(w, f); },
        [](GLFWwindow* w, void (*f)(GLFWwindow*, int)){ glfwSetWindowIconifyCallback(w, f); },
        [](GLFWwindow* w, void (*f)(GLFWwindow*, int)){ glfwSetWindowFocusCallback(w, f); },
        [](GLFWwindow* w, void (*f)(GLFWwindow*, int, int)){ glfwSetWindowPosCallback(w, f); },
        [](GLFWwindow* w, void (*f)(GLFWwindow*, double, double)){ glfwSetScrollCallback(w, f); },
        [](GLFWwindow* w, void (*f)(GLFWwindow*)){ glfwSetWindowRefreshCallback(w, f); }
    };
};

}  // namespace ara
#endif
