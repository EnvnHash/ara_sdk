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
    void runMainLoop(const std::function<void(double time, double dt, unsigned int ctxNr)>& f);

    /**
     * iterates through all windows, calls the draw callback function set for them, polls the events and calls a
     * frambuffer swap -> execute the gl command queue marco.m_g : only_open_windows : This only implemented if
     * m_multCtx is true, since not sure if other apps are using this
     */
    void iterate(bool only_open_windows);

    /** add a new Window using the Window Wrapper class parameters are set via pre filled gWinPar struct */
    GLWindowBase *addWin(glWinPar& gp);

    /** global Keyboard callback function, will be called from all contexts
     * context individual keyboard callbacks are called from here */
    void gWinKeyCallback(GLWindow *window, int key, int scancode, int action, int mods);
    void gMouseButCallback(GLWindow *window, int button, int action, int mods);
    void gMouseCursorCallback(GLWindow *window, double xpos, double ypos);
    void setGlobalKeyCallback(const std::function<void(GLWindow *, int, int, int, int)>& f) { m_keyCbFun = f; }
    void setGlobalMouseCursorCallback(const std::function<void(GLWindow *, double, double)>& f) { m_mouseCursorCbFun = f; }
    void setGlobalMouseButtonCallback(const std::function<void(GLWindow *window, int button, int action, int mods)>& f) { m_mouseButtonCbFun = f; }
    void setWinResizeCallback(const std::function<void(GLWindow *, int, int)>& f) { m_winResizeCbFun = f; }
    void addKeyCallback(unsigned int winInd, const std::function<void(int, int, int, int)>& f);
    void addMouseButCallback(unsigned int winInd, const std::function<void(int, int, int, double, double)>& f);
    void addCursorCallback(unsigned int winInd, const std::function<void(double, double)>& f);
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
    void stop() { m_run = false; }
    bool isRunning() { return m_run; }
    int getDispOffsetX(unsigned int idx);
    int getDispOffsetY(unsigned int idx);

    GLWindow                               *getBack() { return m_windows.back().get(); }
    GLWindow                               *getFirstWin() { return m_windows.front().get(); }
    std::vector<std::unique_ptr<GLWindow>> *getWindows() { return &m_windows; }
    uint32_t                                getNrWindows() { return static_cast<uint32_t>(m_windows.size()); }
    size_t                                  getNrDisplays() { return m_dispCount; }
    std::mutex                             *getDrawMtx() { return &m_drawMtx; }
    std::vector<NativeDisplay>              getDisplays() { return m_displays; }
    void                                    setPrintFps(bool _val) { m_showFps = _val; }
    void                                    setDispFunc(const std::function<void(double, double, unsigned int)>& f) { m_dispFunc = f; }

    void IterateAll(bool only_open_windows, std::function<void(double time, double dt, unsigned int ctxNr)> render_function);
    void EndMainLoop();

    static void error_callback(int error, const char *description) {
        LOGE << "GlNativeWindowManager ERROR: " << description;
    }

    glm::vec2 m_lastMousePos{};

    std::vector<glVidMode>           m_dispModes;
    std::vector<std::pair<int, int>> m_dispOffsets;

    std::function<void(GLWindow *, int, int, int, int)> m_keyCbFun;
    std::function<void(GLWindow *, double, double)>     m_mouseCursorCbFun;
    std::function<void(GLWindow *, int, int, int)>      m_mouseButtonCbFun;
    std::function<void(GLWindow *, int, int)>           m_winResizeCbFun;

    bool                                   m_run;
    std::vector<std::unique_ptr<GLWindow>> m_windows;  // in order to use pointers to the windows of this arrays use unique_ptr.

private:
    std::mutex              m_drawMtx;
    std::vector<glWinPar>   m_addWindows;

    // a vector may rearrange it element and thus change their pointer addresses
    std::map<GLWindow *, std::vector<std::function<void(int, int, int, int)>>>            m_keyCbMap;
    std::map<GLWindow *, std::vector<std::function<void(int, int, int, double, double)>>> m_mouseButCbMap;
    std::map<GLWindow *, std::vector<std::function<void(double, double)>>>                m_cursorCbMap;

    std::function<void(double, double, unsigned int)>   m_dispFunc;
    std::vector<NativeDisplay>                          m_displays;

    bool            m_showFps       = false;
    bool            m_multCtx       = false;
    bool            m_inited        = false;
    unsigned int    m_winIdx        = 0;
    size_t          m_dispCount     = 0;
    double          m_medDt         = 0.066;
    double          m_lastTime      = 0;
    double          printFpsIntv    = 2.0;
    double          m_lastPrintFps  = 0.0;
    GLWindow*       m_shareCtx      = nullptr;


#if !defined(__ANDROID__) && (defined(__linux__) || defined(__APPLE__))
    //GLFWmonitor                      **m_monitors        = nullptr;
    static constexpr const GLFWvidmode m_defaultDispMode = {0, 0, 0, 0, 0, 0};
#endif
};

}  // namespace ara
