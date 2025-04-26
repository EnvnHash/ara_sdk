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
    // make constructor private only share_instance method will create an
    // instance
    GLNativeWindowManager() {
#ifdef _WIN32
        DWORD          DispNum       = 0;
        DISPLAY_DEVICE DisplayDevice = {0};
        DEVMODEA       defaultMode;

        // initialize DisplayDevice
        DisplayDevice.cb = sizeof(DisplayDevice);

        // get all display devices
        while (EnumDisplayDevices(NULL, DispNum, &DisplayDevice, 0)) {
            ZeroMemory(&defaultMode, sizeof(DEVMODE));
            defaultMode.dmSize = sizeof(DEVMODE);
            if (!EnumDisplaySettingsA((LPSTR)DisplayDevice.DeviceName, ENUM_REGISTRY_SETTINGS, &defaultMode))
                OutputDebugStringA("Store default failed\n");

            if (DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
                m_displays.push_back(NativeDisplay());
                m_displays.back().width  = defaultMode.dmPelsWidth;
                m_displays.back().height = defaultMode.dmPelsHeight;
                m_displays.back().offsX  = defaultMode.dmPosition.x;
                m_displays.back().offsY  = defaultMode.dmPosition.y;

                // printf(" got display: offs: %d %d  physical size %d %d \n",
                //       m_displays.back().offsX, m_displays.back().offsY,
                //       m_displays.back().width, m_displays.back().height);
            }

            ZeroMemory(&DisplayDevice, sizeof(DisplayDevice));
            DisplayDevice.cb = sizeof(DisplayDevice);
            DispNum++;
        }  // end while for all display devices

        m_dispCount = m_displays.size();
#endif
    }

    ~GLNativeWindowManager() {}

    /** starts a infinte drawing loop, destroys all windows on stop */
    void runMainLoop(std::function<void(double time, double dt, unsigned int ctxNr)> f) {
        run        = true;
        m_dispFunc = f;

        while (run) {
            iterate(true);
        }

        for (auto &it : windows) {
            it->destroy();
        }
    }

    /**
     * iterates through all windows, calls the draw callback function set
     * for them, polls the events and calls a frambuffer swap -> execute the gl
     * command queue marco.m_g : only_open_windows : This only implemented if
     * multCtx is true, since not sure if other apps are using this
     */
    void iterate(bool only_open_windows) {
        drawMtx.lock();

        // proc open / close request
        for (auto &it : windows) {
            if (it->getRequestOpen()) it->open();
            if (it->getRequestClose()) it->close();
        }

        // update windows
        if (multCtx) {
            winIdx = 0;
            for (auto &it : windows) {
                if (only_open_windows ? it->isOpen() : true) {
                    it->makeCurrent();
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // JUST FOR DEMO: should be
                                                                         // done only when needed
                    if (m_dispFunc) {
                        m_dispFunc(lastTime, medDt, winIdx);
                    }
                    it->swap();
                }
                winIdx++;
            }
        } else if (windows.size() > 0) {
            // marco.m_g : only_open_windows should be implemented here too, but
            // have to verify it won't affect previous apps using this class

            if (windows[0]->isOpen()) {
                windows[0]->makeCurrent();
                if (m_dispFunc) {
                    m_dispFunc(lastTime, medDt, winIdx);
                }
                windows[0]->swap();
            }
        }

        drawMtx.unlock();

#ifndef _WIN32
        glfwPollEvents();  // Poll for and process events
#endif

        // check if new windows were requested
        for (auto &it : addWindows) {
            addWin(it);
        }

        // clear the list of added windows
        if (addWindows.size() > 0) {
            addWindows.clear();
        }

#ifdef _WIN32
        wglMakeCurrent(nullptr, nullptr);
#endif
    }

    /**
     * add a new window (using the GWindow glfw-wrapper class)
     */
    GLWindowBase *addWin(int width, int height, int refreshRate, bool fullScreen, bool useGL32p, int shiftX = 0,
                         int shiftY = 0, int monitorNr = 0, bool decorated = true, bool floating = false,
                         unsigned int nrSamples = 2, bool hidden = false, bool scaleToMonitor = false,
                         GLWindowBase *sharedCtx = nullptr, bool debug = false) {
        addWindows.emplace_back(glWinPar{
            .doInit         = false,
            .fullScreen     = fullScreen,
            .useGL32p       = useGL32p,
            .decorated      = decorated,
            .floating       = floating,
            .createHidden   = hidden,
            .debug          = debug,
            .nrSamples      = nrSamples,
            .shiftX         = shiftX,
            .shiftY         = shiftY,
            .monitorNr      = monitorNr,
            .width          = width,
            .height         = height,
            .refreshRate    = refreshRate,
            .scaleToMonitor = scaleToMonitor,
            .shareCont      = sharedCtx,
        });
        return addWin(addWindows.back());
    }

    /**
     * add a new Window using the Window Wrapper class
     * parameters are set via pre filled gWinPar struct
     */
    GLWindowBase *addWin(glWinPar& gp) {
        windows.push_back(std::make_unique<GLWindow>());

        // if there is no explicit request to share a specific content, and we
        // already got another context to share, take the first added window as
        // a shared context
        gp.shareCont = gp.shareCont ? gp.shareCont : shareCtx;
        gp.doInit    = false;
        if (!windows.back()->create(gp)) {
            std::cerr << "GWindowManager addWin windows.back().create failed " << std::endl;
            return nullptr;
        }

        if (static_cast<unsigned int>(windows.size()) == 1) {
            shareCtx = windows.back().get();
        }

        // add new window specific callback functions
        keyCbMap[windows.back().get()]      = std::vector<std::function<void(int, int, int, int)>>();
        mouseButCbMap[windows.back().get()] = std::vector<std::function<void(int, int, int, double, double)>>();
        multCtx                             = windows.size() > 1;
        return windows.back().get();
    }

    /** global Keyboard callback function, will be called from all contexts
     * context individual keyboard callbacks are called from here */
    void gWinKeyCallback(GLWindow *window, int key, int scancode, int action, int mods) {
        // go through the keyCallback vectors and call the corresponding
        // functions
        auto winIt = keyCbMap.find(window);
        if (winIt != keyCbMap.end()) {
            for (auto &it : keyCbMap[window]) {
                it(key, scancode, action, mods);
            }
        }

        // call the global function which applies for all windows
        if (m_keyCbFun) {
            m_keyCbFun(window, key, scancode, action, mods);
        }
    }

    void gMouseButCallback(GLWindow *window, int button, int action, int mods) {
        // go through the mouseCallback vectors and call the corresponding
        // functions of the actual window that is the rootWidget callback of
        // this window
        for (auto &it : mouseButCbMap[window]) {
            it(button, action, mods, m_lastMouseX, m_lastMouseY);
        }

        // call the global function which applies for all windows
        if (m_mouseButtonCbFun) {
            m_mouseButtonCbFun(window, button, action, mods);
        }
    }

    void gMouseCursorCallback(GLWindow *window, double xpos, double ypos) {
        // go through the keyCallback vectors and call the corresponding
        // functions
        auto keyIt = cursorCbMap.find(window);
        if (keyIt != cursorCbMap.end()) {
            for (auto &it : cursorCbMap[window]) {
                it(xpos, ypos);
            }
        }

        // call the global function which applies for all windows
        if (m_mouseCursorCbFun) {
            m_mouseCursorCbFun(window, xpos, ypos);
        }

        // save last mouse Pos
        m_lastMouseX = xpos;
        m_lastMouseY = ypos;
    }

    void setGlobalKeyCallback(std::function<void(GLWindow *, int, int, int, int)> f) { m_keyCbFun = f; }
    void setGlobalMouseCursorCallback(std::function<void(GLWindow *, double, double)> f) { m_mouseCursorCbFun = f; }
    void setGlobalMouseButtonCallback(std::function<void(GLWindow *window, int button, int action, int mods)> f) {
        m_mouseButtonCbFun = f;
    }

    void setWinResizeCallback(std::function<void(GLWindow *, int, int)> f) { winResizeCbFun = f; }

    void addKeyCallback(unsigned int winInd, std::function<void(int, int, int, int)> _func) {
        if (static_cast<unsigned int>(windows.size()) >= (winInd + 1)) {
            keyCbMap[windows[winInd].get()].push_back(_func);
        } else {
           printf("tav::GLWindowManager::setKeyCallback Error: m_window doesn´t exist. \n");
        }
    }

    void addMouseButCallback(unsigned int winInd, std::function<void(int, int, int, double, double)> _func) {
        if (static_cast<unsigned int>(windows.size()) >= (winInd + 1)) {
            mouseButCbMap[windows[winInd].get()].push_back(_func);
        } else {
            printf("tav::GLNativeWindowManager::addMouseButCallback Error: m_window doesn´t exist. \n");
        }
    }

    void addCursorCallback(unsigned int winInd, std::function<void(double, double)> _func) {
        if (static_cast<unsigned int>(windows.size()) >= (winInd + 1)) {
            cursorCbMap[windows[winInd].get()].push_back(_func);
        } else {
            printf("tav::GLNativeWindowManager::addCursorCallback Error: m_window doesn´t exist. \n");
        }
    }

    void setSwapInterval(unsigned int winNr, bool swapInterval) {
        //		if (static_cast<unsigned int>(windows.size()) >= winNr)
        //			windows[winNr]->setVSync(swapInterval);
    }

    unsigned int getInd(GLWindow *win) {
        unsigned int ctxInd = 0;
        unsigned int winInd = 0;
        for (auto it = windows.begin(); it != windows.end(); ++it) {
            if (win == it->get()) {
                ctxInd = winInd;
            }
            winInd++;
        }
        return ctxInd;
    }

    /** make a specific context visible */
    void open(unsigned int nr) {
        if (windows.size() > nr) {
            windows[nr]->open();
        }
    }

    /** make a specific context inVisible */
    void hide(unsigned int nr) {
        if (windows.size() > nr) {
            windows[nr]->close();
        }
    }

    /** close all glfw windows that have been added through this GWindowManager
     * instance before */
    void closeAll() {
        run = false;
        for (auto &it : windows) {
            it->close();
        }
    }

    void stop() { run = false; }
    bool isRunning() { return run; }

    int getDispOffsetX(unsigned int idx) {
        if (m_dispOffsets.size() > idx) {
            return m_dispOffsets[idx].first;
        } else {
            return 0;
        }
    }

    int getDispOffsetY(unsigned int idx) {
        if (m_dispOffsets.size() > idx) {
            return m_dispOffsets[idx].second;
        } else {
            return 0;
        }
    }

    GLWindow                               *getBack() { return windows.back().get(); }
    GLWindow                               *getFirstWin() { return windows.front().get(); }
    std::vector<std::unique_ptr<GLWindow>> *getWindows() { return &windows; }

    // GLWindow* getWindow(unsigned int nr) { if (windows.size() > nr) return
    // windows[nr]->getWin(); else return nullptr; }
    uint32_t                   getNrWindows() { return (uint32_t)windows.size(); }
    size_t                     getNrDisplays() { return m_dispCount; }
    std::mutex                *getDrawMtx() { return &drawMtx; }
    std::vector<NativeDisplay> getDisplays() { return m_displays; }
    void                       setPrintFps(bool _val) { showFps = _val; }
    void                       setDispFunc(std::function<void(double, double, unsigned int)> f) { m_dispFunc = f; }

    static void error_callback(int error, const char *description) {
        printf(" GFLW ERROR: %s \n", description);
        fputs(description, stderr);
    }

    double m_lastMouseX;
    double m_lastMouseY;

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
