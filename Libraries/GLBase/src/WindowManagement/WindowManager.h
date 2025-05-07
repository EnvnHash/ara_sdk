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

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)

#include <Conditional.h>
#include "GLWindow.h"

#ifdef __linux__
#include <X11/X11MousePos.h>
#endif

namespace ara {
class AssetManager;

class GLBase;

class WindowManager {
public:
    // make constructor private only share_instance method will create an
    // instance
    explicit WindowManager(GLBase *glbase);

    /** starts an infinite drawing loop, destroys all windows on stop */
    [[maybe_unused]] void runMainLoop(std::function<void(double time, double dt, unsigned int ctxNr)> f);

    /**
     * there are several ways to manage multi window rendering. iterate() is for
     * single thread non-event based rendering. This method iterates through all
     * windows, calls the draw callback function set for them, polls the events
     * and calls a framebuffer swap -> execute the gl command queue
     */
    void iterate(bool only_open_windows);

    /**
     * iterate through all windows and call the windows startDrawThread() method, thus starting a new thread with the
     * windows draw loop
     */
    void startThreadedRendering() const;

    /** stop all drawing windows */
    void stopThreadedRendering() const;

#ifdef _WIN32
    std::thread getOutOfBoundsHIDLoop();
#endif

    /** drawing and event thread can run separately. use this for starting a draw-loop independent event loop */
    void                    startEventLoop();
    [[maybe_unused]] void   procEventQueue();
    void                    stopEventLoop();

    [[maybe_unused]] Conditional *getStopEventLoopSema() { return &m_stopEventLoopSema; }
    [[maybe_unused]] Conditional *getStartEventLoopSema() { return &m_startEventLoopSema; }

    /** add a new window (using the GWindow glfw-wrapper class) */
/*    GLWindow *addWin(int width, int height, int refreshRate, bool fullScreen, bool useGL32p, int shiftX = 0,
                     int shiftY = 0, int monitorNr = 0, bool decorated = true, bool floating = false,
                     unsigned int nrSamples = 2, bool hidden = false, bool scaleToMonitor = false,
                     void *sharedCtx = nullptr, bool transparentFB = false, void *extWinHandle = nullptr,
                     bool debug = false);
*/
    /**
     * add a new Window using the GWindow Wrapper class parameters are set via pre-filled gWinPar struct
     */
    GLWindow *addWin(const glWinPar& gp);

    void removeWin(GLWindow *win, bool terminateGLFW = true);

    /** global Keyboard callback function, will be called from all contexts
     * context individual keyboard callbacks are called from here */
    static void globalKeyCb(GLContext ctx, int key, int scancode, int action, int mods);
    static void globalCharCb(GLContext ctx, unsigned int codepoint);
    static void globalMouseButCb(GLContext ctx, int button, int action, int mods);
    static void globalMouseCursorCb(GLContext ctx, double xpos, double ypos);
    static void globalWindowSizeCb(GLContext ctx, int width, int height);
    static void globalWindowCloseCb(GLContext ctx);
    static void globalWindowMaximizeCb(GLContext ctx, int flag);
    static void globalWindowIconifyCb(GLContext ctx, int flag);
    static void globalWindowFocusCb(GLContext ctx, int flag);
    static void globalWindowPosCb(GLContext ctx, int posx, int posy);
    static void globalScrollCb(GLContext ctx, double posx, double posy);
    static void globalWindowRefreshCb(GLContext ctx);

    [[maybe_unused]] void setSwapInterval(unsigned int winNr, bool swapInterval) const;

    void getHwInfo();

#ifdef ARA_USE_GLFW
    std::vector<GLFWvidmode>            getVideoModes();
    std::vector<std::pair<int, int>>    getMonitorOffsets();
    [[maybe_unused]] GLFWvidmode const& getDispMode(unsigned int idx) const;
    GLFWcursor*                         createMouseCursor(const std::string &file, float xHot, float yHot) const;
    void                                loadMouseCursors();
#endif

    [[maybe_unused]] unsigned int getMonitorWidth(unsigned int winNr) const;
    [[maybe_unused]] unsigned int getMonitorHeight(unsigned int winNr) const;

    /** make a specific context visible */
    void open(unsigned int nr) const {
        if (m_windows.size() > nr) {
            m_windows[nr]->open();
        }
    }

    /** make a specific context inVisible */
    void hide(unsigned int nr) const {
        if (m_windows.size() > nr) {
            m_windows[nr]->hide();
        }
    }

    /** close all glfw windows that have been added through this WindowManager instance before */
    [[maybe_unused]] void closeAll() {
        m_run = false;
        for (const auto &it : m_windows) {
            it->close();
        }
    }

    void stop() { m_run = false; }
    bool isRunning() const { return m_run; }

    [[maybe_unused]] int getDispOffsetX(unsigned int idx) const {
        if (m_dispOffsets.size() > idx) {
            return m_dispOffsets[idx].first;
        } else {
            return 0;
        }
    }

    [[maybe_unused]] int getDispOffsetY(unsigned int idx) const {
        if (m_dispOffsets.size() > idx) {
            return m_dispOffsets[idx].second;
        } else {
            return 0;
        }
    }

    [[maybe_unused]] GLWindow *getFirstWin() const { return m_windows.empty() ? nullptr : m_windows.front().get(); }
    [[maybe_unused]] GLWindow *getBack() const  { return m_windows.back().get(); }
    std::vector<std::unique_ptr<GLWindow>>        *getWindows() { return &m_windows; }
    GLWindow                                      *getFocusedWin() const { return m_focusedWin; }
    [[maybe_unused]] uint32_t                      getNrWindows() const { return static_cast<uint32_t>(m_windows.size()); }
    [[maybe_unused]] unsigned int                  getNrDisplays() const { return m_dispCount; }
    [[maybe_unused]] std::vector<DisplayBasicInfo> getDisplays() { return m_displayInfo; }
    [[maybe_unused]] void                          setPrintFps(bool _val) { m_showFps = _val; }
    void                                           setFixFocus(GLWindow *win) { m_fixFocusWin = win; }
    GLWindow                                      *getFixFocus() const { return m_fixFocusWin; }
    void                                           setAssetManager(AssetManager *res) { m_assetManager = res; }
    AssetManager                                   *getAssetManager() { return m_assetManager; }
    [[maybe_unused]] std::thread::id               getMainThreadId() { return m_mainThreadId; }
    void                                           setMainThreadId(std::thread::id inId) { m_mainThreadId = inId; }
    void                                           setBreakEvtLoop(bool val) { m_breakEvtLoop = val; }
    [[maybe_unused]] std::mutex                   *getGlobMouseLoopMtx() { return &m_globMouseLoopMtx; }

    static WindowManager *getThis(GLContext ctx) {
#ifdef ARA_USE_GLFW
        return static_cast<WindowManager *>(glfwGetWindowUserPointer((GLFWwindow *)ctx));
#else
        return nullptr;
#endif
    }

    [[maybe_unused]] void addGlobalKeyCb(void *ptr, std::function<void(GLContext, int, int, int, int)> f) {
        m_globalKeyCb[ptr] = std::move(f);
    }

    [[maybe_unused]] void addGlobalCharCb(void *ptr, std::function<void(GLContext, unsigned int)> f) {
        m_globalCharCb[ptr] = std::move(f);
    }

    [[maybe_unused]] void addGlobalMouseButtonCb(void *ptr, std::function<void(GLContext, int, int, int)> f) {
        m_globalButtonCb[ptr] = std::move(f);
    }

    [[maybe_unused]] void removeGlobalMouseButtonCb(void *ptr) {
        if (const auto it = m_globalButtonCb.find(ptr); it != m_globalButtonCb.end()) {
            m_globalButtonCb.erase(it);
        }
    }

    [[maybe_unused]] void addGlobalMouseCursorCb(void *ptr, std::function<void(GLContext, double, double)> f) {
        m_globalMouseCursorCb[ptr] = std::move(f);
    }

    [[maybe_unused]] void addWinResizeCb(void *ptr, std::function<void(GLContext, int, int)> f) {
        m_globalWinResizeCb[ptr] = std::move(f);
    }

    [[maybe_unused]] void addWinCloseCb(void *ptr, std::function<void(GLContext)> f) {
        m_globalWinCloseCb[ptr] = std::move(f);
    }

    [[maybe_unused]] void addWinMaximizeCb(void *ptr, std::function<void(GLContext, int)> f) {
        m_globalWinMaximizeCb[ptr] = std::move(f);
    }

    [[maybe_unused]] void addWinPosCb(void *ptr, std::function<void(GLContext, int, int)> f) {
        m_globalWinPosCb[ptr] = std::move(f);
    }

    [[maybe_unused]] void addWinScrollCb(void *ptr, std::function<void(GLContext, double, double)> f) {
        m_globalScrollCb[ptr] = std::move(f);
    }

    [[maybe_unused]] void addWinIconifyCb(void *ptr, std::function<void(GLContext, int)> f) {
        m_globalWinIconifyCb[ptr] = std::move(f);
    }

    [[maybe_unused]] void addWinFocusCb(void *ptr, std::function<void(GLContext, int)> f) {
        m_globalWinFocusCb[ptr] = std::move(f);
    }

    [[maybe_unused]] void addWinRefreshCb(void *ptr, std::function<void(GLContext)> f) {
        m_globalWinRefreshCbMap[ptr] = std::move(f);
    }

    void callGlobalMouseButCb(int button) const;
    void addEvtLoopCb(const std::function<bool()> &);

    static void error_callback(int error, const char *description) {
        printf(" GFLW ERROR: %s \n", description);
        fputs(description, stderr);
    }

    // ----------------------------------------------------------
    // Set of interactive functions for independent sequence calls (marco.m_g)
    // These are useful if we want to call certain functions before and after
    // the render cycle.
    // ------------------------------

    void IterateAll(bool                                                            only_open_windows,
                    std::function<void(double time, double dt, unsigned int ctxNr)> render_function) {
        if ((m_globalDrawFunc = std::move(render_function))) {
            iterate(only_open_windows);
        }
    }

    [[maybe_unused]] void EndMainLoop() const {
        for (auto &it : m_windows) {
            it->destroy(false);
        }
        GLWindow::terminateLibrary();
    }

#ifdef ARA_USE_GLFW
    GLFWcursor *m_diagResizeAscCursor  = nullptr;
    GLFWcursor *m_diagResizeDescCursor = nullptr;
#endif
private:
    std::vector<glWinPar>                  m_addWindows;
    std::vector<std::unique_ptr<GLWindow>> m_windows;
    std::vector<DisplayBasicInfo>          m_displayInfo;
    std::vector<std::pair<int, int>>       m_dispOffsets;
    std::vector<std::function<bool()>>     m_evtLoopCbs;

    std::function<void(double, double, unsigned int)> m_globalDrawFunc;

    // window specific HID callbacks
    std::unordered_map<GLContext, std::function<void(int, int, int, int)>> m_keyCbMap;
    std::unordered_map<GLContext, std::function<void(unsigned int)>>       m_charCbMap;
    std::unordered_map<GLContext, std::function<void(int, int, int)>>      m_mouseButCbMap;
    std::unordered_map<GLContext, std::function<void(double, double)>>     m_cursorCbMap;
    std::unordered_map<GLContext, std::function<void(int, int)>>           m_winResizeCbMap;
    std::unordered_map<GLContext, std::function<void()>>                   m_winCloseCbMap;
    std::unordered_map<GLContext, std::function<void(int, int)>>           m_winPosCbMap;
    std::unordered_map<GLContext, std::function<void(double, double)>>     m_scrollCbMap;
    std::unordered_map<GLContext, std::function<void(int)>>                m_winMaxmimizeCbMap;
    std::unordered_map<GLContext, std::function<void(int)>>                m_winIconfifyCbMap;
    std::unordered_map<GLContext, std::function<void(int)>>                m_winFocusCbMap;
    std::unordered_map<GLContext, std::function<void()>>                   m_winRefreshCbMap;

    // global HID callbacks
    std::unordered_map<void *, std::function<void(GLContext, int, int, int, int)>> m_globalKeyCb;
    std::unordered_map<void *, std::function<void(GLContext, unsigned int)>>       m_globalCharCb;
    std::unordered_map<void *, std::function<void(GLContext, int, int, int)>>      m_globalButtonCb;
    std::unordered_map<void *, std::function<void(GLContext, double, double)>>     m_globalMouseCursorCb;
    std::unordered_map<void *, std::function<void(GLContext, int, int)>>           m_globalWinResizeCb;
    std::unordered_map<void *, std::function<void(GLContext, int)>>                m_globalWinMaximizeCb;
    std::unordered_map<void *, std::function<void(GLContext, int)>>                m_globalWinIconifyCb;
    std::unordered_map<void *, std::function<void(GLContext, int)>>                m_globalWinFocusCb;
    std::unordered_map<void *, std::function<void(GLContext)>>                     m_globalWinCloseCb;
    std::unordered_map<void *, std::function<void(GLContext, int, int)>>           m_globalWinPosCb;
    std::unordered_map<void *, std::function<void(GLContext, double, double)>>     m_globalScrollCb;
    std::unordered_map<void *, std::function<void(GLContext)>>                     m_globalWinRefreshCbMap;

    std::list<Conditional *> m_semaqueue;

    bool              m_run          = false;
    bool              m_showFps      = false;
    bool              m_multCtx      = false;
    bool              m_inited       = false;
    std::atomic<bool> m_breakEvtLoop = false;

    unsigned int m_winInd = 0;

    double m_medDt        = 0.066;
    double m_lastTime     = 0;
    double m_printFpsIntv = 2.0;
    double m_lastPrintFps = 0.0;
    double m_lastMouseX   = 0.0;
    double m_lastMouseY   = 0.0;

    void            *m_shareCtx    = nullptr;
    GLWindow        *m_fixFocusWin = nullptr;
    GLWindow        *m_focusedWin  = nullptr;
    GLBase          *m_glbase = nullptr;
    Conditional     m_stopEventLoopSema;
    Conditional     m_startEventLoopSema;
    std::thread::id m_mainThreadId;
    std::mutex      m_evtLoopCbMtx;
    std::mutex      m_globMouseLoopMtx;
    int             m_dispCount = 0;
    AssetManager    *m_assetManager = nullptr;

    std::chrono::system_clock::time_point m_now;
    std::chrono::system_clock::time_point m_startTime;
#ifdef ARA_USE_GLFW
    static constexpr const GLFWvidmode m_defaultDispMode = {0, 0, 0, 0, 0, 0};
    std::vector<GLFWvidmode>           m_dispModes;
    GLFWmonitor                      **m_monitors = nullptr;
#endif
};
}  // namespace ara
#endif
