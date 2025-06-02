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

    /** add a new Window using the GWindow Wrapper class parameters are set via pre-filled gWinPar struct */
    GLWindow *addWin(const glWinPar& gp);

    void removeWin(GLWindow *win, bool terminateGLFW = true);

    template <typename T, typename... Args>
    static void resolveWinCbFunc(const T& cb, Args&&... args) {
        std::visit([&] (auto&& func) {
            if constexpr (std::is_invocable_v<std::decay_t<decltype(func)>, Args...>) {
                func(args...);
            }
        }, cb);
    }

    template <typename... Args>
    static void callGlobalHidCb(WindowManager* winMan, winCb tp, Args... args) {
        if (winMan->m_globalWinHidCpMap.find(tp) != winMan->m_globalWinHidCpMap.end()) {
            for (auto& [key, winCtxHidCbFunc] : winMan->m_globalWinHidCpMap[tp] ) {
                resolveWinCbFunc<winCtxHidCb>(winCtxHidCbFunc, args...);
            }
        }
    }

    template <typename... Args>
    static void callWinAndGlobalHidCb(GLContext& ctx, winCb tp, Args&&... args) {
        auto winMan = getThis(ctx);
        if (winMan) {
            auto winIt = winMan->m_winHidCbMap[tp].find(ctx);
            if (winIt != winMan->m_winHidCbMap[tp].end()) {
                resolveWinCbFunc<winHidCb>(winIt->second, args...);
            }

            callGlobalHidCb(winMan, tp, args...);
        }
    }

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
    void open(unsigned int nr) const;
    /** make a specific context inVisible */
    void hide(unsigned int nr) const;
    /** close all glfw windows that have been added through this WindowManager instance before */
    [[maybe_unused]] void closeAll();
    void destroyAll();

    void stop() { m_run = false; }
    bool isRunning() const { return m_run; }

    [[maybe_unused]] int getDispOffsetX(unsigned int idx) const {
        return m_dispOffsets.size() > idx ? m_dispOffsets[idx].first : 0;
    }

    [[maybe_unused]] int getDispOffsetY(unsigned int idx) const {
        return m_dispOffsets.size() > idx ? m_dispOffsets[idx].second : 0;
    }

    [[maybe_unused]] GLWindow *getFirstWin() const {
        return m_windows.empty() ? nullptr : m_windows.front().get();
    }

    [[maybe_unused]] auto   getBack() const  { return m_windows.back().get(); }
    auto                    getWindows() { return &m_windows; }
    auto                    getFocusedWin() const { return m_focusedWin; }
    [[maybe_unused]] auto   getNrWindows() const { return static_cast<uint32_t>(m_windows.size()); }
    [[maybe_unused]] auto   getNrDisplays() const { return m_dispCount; }
    [[maybe_unused]] auto   getDisplays() { return m_displayInfo; }
    [[maybe_unused]] void   setPrintFps(bool _val) { m_showFps = _val; }
    void                    setFixFocus(GLWindow *win) { m_fixFocusWin = win; }
    auto                    getFixFocus() const { return m_fixFocusWin; }
    void                    setAssetManager(AssetManager *res) { m_assetManager = res; }
    auto                    getAssetManager() { return m_assetManager; }
    [[maybe_unused]] auto   getMainThreadId() { return m_mainThreadId; }
    void                    setMainThreadId(std::thread::id inId) { m_mainThreadId = inId; }
    void                    setBreakEvtLoop(bool val) { m_breakEvtLoop = val; }
    [[maybe_unused]] auto   getGlobMouseLoopMtx() { return &m_globMouseLoopMtx; }

    static WindowManager *getThis(GLContext ctx) {
#ifdef ARA_USE_GLFW
        return static_cast<WindowManager *>(glfwGetWindowUserPointer((GLFWwindow *)ctx));
#else
        return nullptr;
#endif
    }

    void addGlobalHidCb(winCb tp, void* ptr, const winCtxHidCb& f);
    void removeGlobalHidCb(winCb tp, void *ptr);
    void addEvtLoopCb(const std::function<bool()> &);

    static void error_callback(int error, const char *description) {
        printf(" GFLW ERROR: %s \n", description);
        fputs(description, stderr);
    }

    // ----------------------------------------------------------
    // Set of interactive functions for independent sequence calls (marco.g)
    // These are useful if we want to call certain functions before and after the render cycle.
    // ------------------------------

    void IterateAll(bool                                              only_open_windows,
                    std::function<void(double, double, unsigned int)> render_function) {
        if ((m_globalDrawFunc = std::move(render_function))) {
            iterate(only_open_windows);
        }
    }

    [[maybe_unused]] void EndMainLoop() {
        destroyAll();
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
    std::vector<std::function<bool()>>     m_custEventQueue;

    std::function<void(double, double, unsigned int)> m_globalDrawFunc;

    std::unordered_map<winCb, std::unordered_map<GLContext, winHidCb>>  m_winHidCbMap;
    std::unordered_map<winCb, std::unordered_map<void*, winCtxHidCb>>   m_globalWinHidCpMap;

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
