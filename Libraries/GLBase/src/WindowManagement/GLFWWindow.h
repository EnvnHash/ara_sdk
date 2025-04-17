#pragma once

#include "glb_common/glb_common.h"

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
    bool create(glWinPar &gp) override { return init(gp); }
    int  init(glWinPar &gp);

    /**
     * @param f the drawing function which will be execute or every iteration
     * @param eventBased choose wheter the loop should run freely or stop and
     * wait for the m_iterate signal
     * @param terminateGLFW shall GLFW be terminated when the loop exits? in
     * case of multiple window it probably shouldn't
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

    /** hide window, including removing it from the taskbar, dock or windowlist
     */
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

    void focus() {
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
    bool          isOpen() const { return m_isOpen; }
    bool          isMinimized() { return glfwGetWindowAttrib(m_window, GLFW_ICONIFIED); }
    bool          isMaximized() { return glfwGetWindowAttrib(m_window, GLFW_MAXIMIZED); }
    bool          isHidBlocked() { return m_hidBlocked; }
    int           getMonitorId() const { return useMonitor; }
    unsigned int  getMonitorWidth() const { return m_monWidth; }
    unsigned int  getMonitorHeight() const { return m_monHeight; }
    unsigned int  getWidth() { return m_widthVirt; }        /// in virtual pixels
    unsigned int  getHeight() { return m_heightVirt; }      /// in virtual pixels
    unsigned int  getWidthReal() { return m_widthReal; }    /// in real pixels
    unsigned int  getHeightReal() { return m_heightReal; }  /// in real pixels
    void         *getWin() override { return m_window; }
    GLFWwindow   *getCtx() { return m_window; }
    GLFWmonitor **getMonitors() { return m_monitors; }
    GLFWmonitor  *getMonitor(int i) { return m_monitors[i]; }
    int           getNrMonitors() const { return m_count; }
    int           getPosX() const { return (int)m_posXvirt; }      /// in virtual pixels
    int           getPosY() const { return (int)m_posYvirt; }      /// in virtual pixels
    int           getPosXReal() const { return (int)m_posXreal; }  /// in real pixels
    int           getPosYReal() const { return (int)m_posYreal; }  /// in real pixels
    int           getFocus() {
        int foc = glfwGetWindowAttrib(m_window, GLFW_FOCUSED);
        return foc;
    }

    void *getNativeCtx() override { return m_nativeHandle; }

    /// return virtual pixels
    glm::ivec2 getLastMousePos() {
        double xpos, ypos;
        glfwGetCursorPos(m_window, &xpos, &ypos);
#ifdef __APPLE__
        return {(int)xpos, (int)ypos};
#else
        return glm::ivec2((int)(xpos / m_contentScale.x), (int)(ypos / m_contentScale.y));
#endif
    }

    glm::ivec2 getAbsMousePos() {
        glm::ivec2 mp;
        mouse::getAbsMousePos(mp.x, mp.y);
#ifdef __APPLE__
        return mp;
#else
        return glm::ivec2{(int)((float)mp.x / m_contentScale.x), (int)((float)mp.y / m_contentScale.y)};
#endif
    }

#ifdef _WIN32
    HWND  getHwndHandle() { return m_hwndHandle; }
    HGLRC getHglrcHandle() { return m_wglHandle; }
#endif

    int         *getWorkArea() { return m_workArea; }
    Conditional *getIterateSema() { return &m_iterate; }
    bool         getBlockResizing() { return m_blockResizing; }
    bool         getRequestOpen() const { return m_requestOpen; }
    bool         getRequestClose() { return m_requestClose; }
    void         requestOpen(bool val) { m_requestOpen = val; }
    void         requestClose(bool val) { m_requestClose = val; }
    std::function<void()>& getOnCloseCb() { return m_onCloseCb; }

    glm::vec2 getDpi() {
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
        m_widthReal  = (int)((float)inWidth * m_contentScale.x);
        m_heightReal = (int)((float)inHeight * m_contentScale.y);
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
        m_posXvirt = (float)posx;
        m_posYvirt = (float)posy;
        m_posXreal = (float)posx * m_contentScale.x;
        m_posYreal = (float)posy * m_contentScale.y;
#ifdef __APPLE__
        glfwSetWindowPos(m_window, m_posXvirt, m_posYvirt);
#else
        glfwSetWindowPos(m_window, (int)m_posXreal, (int)m_posYreal);
#endif
        iterate();
    }

    void setVSync(bool set) override { glfwSwapInterval(set); }
    void setBlockResizing(bool val) { m_blockResizing = val; }
    void setBlockMouseIconSwitch(bool val) { m_blockMouseIconSwitch = val; }

    void setMouseCursor(WinMouseIcon iconTyp) {
        if (!m_blockMouseIconSwitch) {
            glfwSetCursor(m_window, m_mouseCursors[toType(iconTyp)]);
        }
    }

    void setBlockHid(bool val) { m_hidBlocked = val; }
    void setFloating(bool val) { glfwWindowHint(GLFW_FLOATING, val); }
    // utility methods for unified window handling (GLFWWindow -> GLWindow)
    void setKeyCallback(GLFWkeyfun f) { glfwSetKeyCallback(m_window, f); }
    void setCharCallback(GLFWcharfun f) { glfwSetCharCallback(m_window, f); }
    void setMouseButtonCallback(GLFWmousebuttonfun f) { glfwSetMouseButtonCallback(m_window, f); }
    void setCursorPosCallback(GLFWcursorposfun f) { glfwSetCursorPosCallback(m_window, f); }
    void setWindowSizeCallback(GLFWwindowsizefun f) { glfwSetWindowSizeCallback(m_window, f); }
    void setWindowCloseCallback(GLFWwindowclosefun f) { glfwSetWindowCloseCallback(m_window, f); }
    void setWindowMaximizeCallback(GLFWwindowmaximizefun f) { glfwSetWindowMaximizeCallback(m_window, f); }
    void setWindowIconifyCallback(GLFWwindowiconifyfun f) { glfwSetWindowIconifyCallback(m_window, f); }
    void setWindowFocusCallback(GLFWwindowfocusfun f) { glfwSetWindowFocusCallback(m_window, f); }
    void setWindowPosCallback(GLFWwindowposfun f) { glfwSetWindowPosCallback(m_window, f); }
    void setScrollCallback(GLFWscrollfun f) { glfwSetScrollCallback(m_window, f); }
    void setWindowRefreshCallback(GLFWwindowrefreshfun f) { glfwSetWindowRefreshCallback(m_window, f); }
    void setOnCloseCb(std::function<void()> f) { m_onCloseCb = std::move(f); }

    void onWindowSize(int width, int height) override;

    static void  pollEvents() { glfwPollEvents(); }
    static void  waitEvents() { glfwWaitEvents(); }
    static void  postEmptyEvent() { glfwPostEmptyEvent(); }
    static void  setErrorCallback(GLFWerrorfun f) { glfwSetErrorCallback(f); }
    static void *getWindowUserPointer(GLFWwindow *win) { return glfwGetWindowUserPointer(win); }
    
    static glm::vec2 getPrimaryMonitorWindowContentScale();

    static void initLibrary() {
        printf("initLibrary");
        if (!m_glfwInited && !glfwInit()) {
            printf("ERROR: Couldn't init glfw\n");
            exit(EXIT_FAILURE);
        }
        m_glfwInited = true;
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
        printf(" GFLW ERROR: %s \n", description);
        fputs(description, stderr);
    }

protected:
    std::vector<GLFWvidmode>             m_modes;
    std::vector<std::pair<int, int>>     m_monOffsets;
    std::vector<std::pair<float, float>> m_monContScale;
    std::function<void()>                m_onCloseCb;

    int monitorRefreshRate;
    int useMonitor = 0;
    int m_workArea[4];

    bool m_isOpen               = false;
    bool m_requestOpen          = false;
    bool m_requestClose         = false;
    bool m_blockResizing        = false;
    bool m_blockMouseIconSwitch = false;

    static inline bool m_glfwInited = false;

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
    GLFWcursor   *m_mouseCursors[toType(WinMouseIcon::count)];

    glm::ivec4 m_restoreWinPar{0};
    std::mutex m_drawMtx;
};
}  // namespace ara
#endif
