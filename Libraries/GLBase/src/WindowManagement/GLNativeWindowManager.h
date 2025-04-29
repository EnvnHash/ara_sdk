#pragma once

#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <vector>

#include "WindowManagement/GLWindow.h"
#include "WindowManagement/GLWindowCommon.h"

namespace ara {

struct NativeDisplay {
    uint32_t offsX;
    uint32_t offsY;
    uint32_t width;
    uint32_t height;
};

class GLNativeWindowManager {
public:
    // make constructor private only share_instance method will create an instance
    GLNativeWindowManager();

    /** starts a infinte drawing loop, destroys all windows on stop */
    void runMainLoop(std::function<void(double time, double dt, unsigned int ctxNr)> f);

    /**
     * iterates through all windows, calls the draw callback function set for them, polls the events and calls a
     * frambuffer swap -> execute the gl command queue marco.m_g : only_open_windows : This only implemented if
     * multCtx is true, since not sure if other apps are using this
     */
    void iterate(bool only_open_windows);

    /** add a new Window using the Window Wrapper class parameters are set via pre filled gWinPar struct */
    GLWindowBase *addWin(glWinPar& gp);

    /** global Keyboard callback function, will be called from all contexts
     * context individual keyboard callbacks are called from here */
    void gWinKeyCallback(GLWindow *window, int key, int scancode, int action, int mods);
    void gMouseButCallback(GLWindow *window, int button, int action, int mods);
    void gMouseCursorCallback(GLWindow *window, double xpos, double ypos);
    void setGlobalKeyCallback(std::function<void(GLWindow *, int, int, int, int)> f) { m_keyCbFun = f; }
    void setGlobalMouseCursorCallback(std::function<void(GLWindow *, double, double)> f) { m_mouseCursorCbFun = f; }
    void setGlobalMouseButtonCallback(std::function<void(GLWindow *window, int button, int action, int mods)> f) { m_mouseButtonCbFun = f; }
    void setWinResizeCallback(std::function<void(GLWindow *, int, int)> f) { winResizeCbFun = f; }
    void addKeyCallback(unsigned int winInd, std::function<void(int, int, int, int)> _func);
    void addMouseButCallback(unsigned int winInd, std::function<void(int, int, int, double, double)> _func);
    void addCursorCallback(unsigned int winInd, std::function<void(double, double)> _func);
    void setSwapInterval(unsigned int winNr, bool swapInterval) {
        //		if (static_cast<unsigned int>(windows.size()) >= winNr)
        //			windows[winNr]->setVSync(swapInterval);
    }

    unsigned int getInd(GLWindow *win);

    /** make a specific context visible */
    void open(unsigned int nr);

    /** make a specific context inVisible */
    void hide(unsigned int nr);

    /** close all glfw windows that have been added through this GWindowManager instance before */
    void closeAll();
    void stop() { run = false; }
    bool isRunning() { return run; }
    int getDispOffsetX(unsigned int idx);
    int getDispOffsetY(unsigned int idx);

    GLWindow                               *getBack() { return windows.back().get(); }
    GLWindow                               *getFirstWin() { return windows.front().get(); }
    std::vector<std::unique_ptr<GLWindow>> *getWindows() { return &windows; }
    uint32_t                                getNrWindows() { return (uint32_t)windows.size(); }
    size_t                                  getNrDisplays() { return m_dispCount; }
    std::mutex                             *getDrawMtx() { return &drawMtx; }
    std::vector<NativeDisplay>              getDisplays() { return m_displays; }
    void                                    setPrintFps(bool _val) { showFps = _val; }
    void                                    setDispFunc(std::function<void(double, double, unsigned int)> f) { m_dispFunc = f; }

    static void error_callback(int error, const char *description) {
        printf(" GFLW ERROR: %s \n", description);
        fputs(description, stderr);
    }

    double m_lastMouseX = 0;
    double m_lastMouseY = 0;

    std::vector<glVidMode>           m_dispModes;
    std::vector<std::pair<int, int>> m_dispOffsets;
    // int nrMonitors=0;

    std::function<void(GLWindow *, int, int, int, int)> m_keyCbFun;
    std::function<void(GLWindow *, double, double)>     m_mouseCursorCbFun;
    std::function<void(GLWindow *, int, int, int)>      m_mouseButtonCbFun;
    std::function<void(GLWindow *, int, int)>           winResizeCbFun;

    // ----------------------------------------------------------	// Set
    // of interactive functions for independent sequence calls (marco.m_g) These
    // are useful if we want to call certain functions before and after the
    // render cycle.
    // ------------------------------	void BeginMainLoop() { run = true; }

    void IterateAll(bool                                                            only_open_windows,
                    std::function<void(double time, double dt, unsigned int ctxNr)> render_function) {
        if ((m_dispFunc = render_function)) {
            iterate(only_open_windows);
        }
    }

    void EndMainLoop() {
        for (auto &it : windows) {
            it->destroy();
        }
    }

    bool                                   run;
    std::vector<std::unique_ptr<GLWindow>> windows;  // in order to use pointers to the windows of this arrays use
                                                     // unique_otr.

private:
    std::mutex              drawMtx;
    std::vector<glWinPar>   addWindows;
    // a vector may rearrange it element and thus change their pointer addresses
    std::map<GLWindow *, std::vector<std::function<void(int, int, int, int)>>>            keyCbMap;
    std::map<GLWindow *, std::vector<std::function<void(int, int, int, double, double)>>> mouseButCbMap;
    std::map<GLWindow *, std::vector<std::function<void(double, double)>>>                cursorCbMap;

    std::function<void(double, double, unsigned int)> m_dispFunc;

    bool showFps  = false;
    bool multCtx  = false;
    bool m_inited = false;

    unsigned int winIdx = 0;

    double    medDt        = 0.066;
    double    lastTime     = 0;
    double    printFpsIntv = 2.0;
    double    lastPrintFps = 0.0;
    GLWindow *shareCtx     = nullptr;

    std::vector<NativeDisplay> m_displays;

#ifdef _WIN32
    size_t m_dispCount = 0;
#endif

#ifdef __linux__
    int                                m_dispCount       = 0;
    GLFWmonitor                      **m_monitors        = nullptr;
    static constexpr const GLFWvidmode m_defaultDispMode = {0, 0, 0, 0, 0, 0};
#endif
};

}  // namespace ara
