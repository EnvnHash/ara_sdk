//
// Created by user on 26.11.2020.
//

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)

#include "WindowManagement/WindowManager.h"

#include <Utils/Texture.h>

#include "Res/ResInstance.h"

using namespace std;
using namespace std::chrono;
using namespace ara::mouse;
namespace fs = std::filesystem;

namespace ara {
WindowManager::WindowManager(GLBase *glbase) : m_glbase(glbase) {
    GLWindow::setErrorCallback(error_callback);
    GLWindow::initLibrary();  // Initialize the library (in case of glfw)
    getHwInfo();
    m_startTime = system_clock::now();
}

[[maybe_unused]] void WindowManager::runMainLoop(std::function<void(double time, double dt, unsigned int ctxNr)> f) {
    m_run            = true;
    m_globalDrawFunc = std::move(f);

    while (m_run) iterate(true);

    for (auto &it : m_windows) it->destroy(false);

    GLWindow::terminateLibrary();
}

void WindowManager::iterate(bool only_open_windows) {
    m_now        = system_clock::now();
    auto actTime = std::chrono::duration<double, std::milli>(m_now - m_startTime).count();

    // update time counter, get dt and smooth it
    if (m_medDt == 0.066) {
        if (m_lastTime != 0.0) {
            m_medDt = actTime - m_lastTime;
        }
    } else {
        m_medDt = ((actTime - m_lastTime) + (m_medDt * 30.0)) / 31.0;
    }

    m_lastTime = actTime;

    if (m_showFps && (m_lastTime - m_lastPrintFps) > m_printFpsIntv) {
        printf("FPS: %f dt: %f\n", 1.0 / m_medDt, m_medDt);
        m_lastPrintFps = m_lastTime;
    }

    GLWindow::pollEvents();  // Poll for and process events.
    // Note: this should be glfwWaitEvents in case of event based rendering, but
    // e.m_g. on window resizing on windows events are blocked, so no update
    // would be possible during this operation. This is why we need to manually
    // interrupt the rendering loop

    // update windows
    m_winInd = 0;
    for (auto &it : m_windows) {
        if (!only_open_windows || it->isOpen()
#ifdef ARA_USE_GFLW
              && !glfwWindowShouldClose(it->getWin())
#endif
        ) {
            it->makeCurrent();
            glViewport(0, 0, (GLsizei)it->getWidth(), (GLsizei)it->getHeight());

            // note: screen clearing is managed by draw function
            // if there is global draw function set, take it, otherwise take the
            // Windows' draw function
            if (m_globalDrawFunc) {
                m_globalDrawFunc(m_lastTime, m_medDt, m_winInd);
            } else if (it->getDrawFunc()) {
                it->getDrawFunc()(m_lastTime, m_medDt, m_winInd);
            }

            it->swap();
        }
        m_winInd++;
    }

    // check if new windows were requested
    for (auto &it : m_addWindows) {
        addWin(&it);
    }

    GLWindow::makeNoneCurrent();
}

void WindowManager::startEventLoop() {
    GLWindow::makeNoneCurrent();  // be sure no context is current in order to be able to create new windows

    m_run          = true;
    m_mainThreadId = this_thread::get_id();
#ifdef _WIN32
    // glfw can't capture clicks outside the active windows ... so do this here
    // manually
    auto th = std::thread([this] {
        int  x = 0, y = 0;
        bool mousePressed[2] = {false, false};
        bool isOutOfBounds   = false;
        bool m_didSend[2]    = {false, false};

        while (m_run) {
            {
                unique_lock<mutex> lock(m_globMouseLoopMtx);
                mousePressed[0] = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
                mousePressed[1] = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;

                // check if this click is out of bounds of any active windows
                if (mousePressed[0] || mousePressed[1]) {
                    isOutOfBounds = false;
                    getAbsMousePos(x, y);
                    if (!x && !y) return;
                    for (auto &w : *getWindows())
                        if (w && w->isInited() && w->isOpen())
                            isOutOfBounds = isOutOfBounds ||
                                            !(x >= w->getPosition().x && x <= (w->getPosition().x + w->getSize().x) &&
                                              y >= w->getPosition().y && y <= (w->getPosition().y + w->getSize().y));
                }
            }

            for (int i = 0; i < 2; i++) {
                if (isOutOfBounds && mousePressed[i] && !m_didSend[i]) {
                    m_didSend[i] = true;
                    callGlobalMouseButCb(i == 0 ? GLFW_MOUSE_BUTTON_LEFT : GLFW_MOUSE_BUTTON_RIGHT);
                }
                if (!mousePressed[i] && m_didSend[i]) {
                    m_didSend[i] = false;
                }
            }

            this_thread::sleep_for(5ms);
        }

        // m_globalMouseClickExited.notify();
    });
    // th.detach();
#endif
    bool funcSuccess = false;
    m_startEventLoopSema.notify();

    while (m_run) {
        GLWindow::waitEvents();  // process HID callbacks, there is a regular interval from glfw aprox. each second
        if (!m_evtLoopCbs.empty()) {
            m_evtLoopCbMtx.lock();

            // process the event loop callbacks
            for (auto it = m_evtLoopCbs.begin(); it != m_evtLoopCbs.end();) {
                funcSuccess = (*it)();
                if (m_breakEvtLoop) {
                    break;
                }

                if (funcSuccess) {
                    it = m_evtLoopCbs.erase(it);
                } else {
                    it++;
                }
            }

            m_evtLoopCbMtx.unlock();
        }
    }

#ifdef _WIN32
    if (th.joinable()) th.join();
#endif

    m_evtLoopCbs.clear();
    m_stopEventLoopSema.notify();
}

[[maybe_unused]] void WindowManager::procEventQueue() {
    bool funcSuccess = false;

    if (!m_evtLoopCbs.empty()) {
#ifndef __ANDROID__
        m_evtLoopCbMtx.lock();
#endif
        // process the event loop callbacks
        for (auto it = m_evtLoopCbs.begin(); it != m_evtLoopCbs.end();) {
            funcSuccess = (*it)();
            if (m_breakEvtLoop) {
                break;
            }

            if (funcSuccess) {
                it = m_evtLoopCbs.erase(it);
            } else {
                it++;
            }
        }
#ifndef __ANDROID__
        m_evtLoopCbMtx.unlock();
#endif
    }
}

void WindowManager::startThreadedRendering() {
    GLWindow::makeNoneCurrent();

    // run through all existing windows and start renderloops for them
    for (auto &it : m_windows)
        if (!it->isRunning() && it->getDrawFunc()) it->startDrawThread(it->getDrawFunc());
}

GLWindow *WindowManager::addWin(int width, int height, int refreshRate, bool fullScreen, bool useGL32p, int shiftX,
                                int shiftY, int monitorNr, bool decorated, bool floating, unsigned int nrSamples,
                                bool hidden, bool scaleToMonitor, void *sharedCtx, bool transparentFB,
                                void *extWinHandle, bool debug) {
    glWinPar gp;
    gp.width                  = width;
    gp.height                 = height;
    gp.refreshRate            = refreshRate;
    gp.fullScreen             = fullScreen;
    gp.useGL32p               = useGL32p;
    gp.shiftX                 = shiftX;
    gp.shiftY                 = shiftY;
    gp.monitorNr              = monitorNr;
    gp.decorated              = decorated;
    gp.floating               = floating;
    gp.nrSamples              = nrSamples;
    gp.debug                  = debug;
    gp.createHidden           = hidden;
    gp.doInit                 = false;
    gp.resizeable             = true;
    gp.scaleToMonitor         = scaleToMonitor;
    gp.shareCont              = sharedCtx;
    gp.transparentFramebuffer = transparentFB;
    gp.extWinHandle           = extWinHandle;
    gp.glbase                 = m_glbase;
    return addWin(&gp);
}

GLWindow *WindowManager::addWin(glWinPar *gp) {
#ifdef _WIN32
    {
        unique_lock<mutex> lock(m_globMouseLoopMtx);
#endif
        m_windows.push_back(std::make_unique<GLWindow>());

        // if there is no explicit request to share a specific content, and we already got another context to share,
        // take the first added window as a shared context
        gp->shareCont = gp->shareCont ? gp->shareCont : m_shareCtx;
        gp->hidExtern = true;  // HID input must be managed by WindowManager
        m_windows.back()->init(*gp);
        if (!m_windows.back()->getCtx()) {
            return nullptr;
        }

#ifdef _WIN32
    }
#endif

#ifdef ARA_USE_GLFW
    // pass a pointer to this WindowManager instance to glfw, to be used inside
    // the HID callback functions
    glfwSetWindowUserPointer(m_windows.back()->getCtx(), reinterpret_cast<void *>(this));
#endif

    if (static_cast<unsigned int>(m_windows.size()) == 1) {
        m_shareCtx = (void *)m_windows.back()->getCtx();
    }

    auto win = m_windows.back().get();

    if (gp->hidInput) {
        // bind window HID callbacks to globalCallbacks by adding them to the
        // callback maps
        m_keyCbMap[win->getCtx()]      = [win](int p1, int p2, int p3, int p4) { win->onKey(p1, p2, p3, p4); };
        m_charCbMap[win->getCtx()]     = [win](unsigned int p1) { win->onChar(p1); };
        m_mouseButCbMap[win->getCtx()] = [win](int button, int action, int mods) {
            win->onMouseButton(button, action, mods);
        };
        m_winCloseCbMap[win->getCtx()]     = [win] { win->onWindowClose(); };
        m_winMaxmimizeCbMap[win->getCtx()] = [win](int maximized) { win->onWindowMaximize(maximized); };
        m_winIconfifyCbMap[win->getCtx()]  = [win](int iconified) { win->onWindowIconify(iconified); };
        m_winFocusCbMap[win->getCtx()]     = [win](int focused) { win->onWindowFocus(focused); };
        m_winRefreshCbMap[win->getCtx()]   = [win] { win->onWindowRefresh(); };
        m_scrollCbMap[win->getCtx()]       = [win](double xOffs, double yOffs) { win->onScroll(xOffs, yOffs); };

        m_cursorCbMap[win->getCtx()] = [win](double x, double y) {
#if defined(_WIN32) || defined(__linux__)
            win->onMouseCursor(x / win->getContentScale().x, y / win->getContentScale().y);
#else
            win->onMouseCursor(x, y);
#endif
        };

        m_winResizeCbMap[win->getCtx()] = [win](int w, int h) {
#if defined(_WIN32) || defined(__linux__)
            win->onWindowSize((int)((float)w / win->getContentScale().x), (int)((float)h / win->getContentScale().x));
#else
            win->onWindowSize(w, h);
#endif
        };
        m_winPosCbMap[win->getCtx()] = [win](int posX, int posY) {
#ifdef _WIN32
            win->onWindowPos((int)((float)posX / win->getContentScale().x),
                             (int)((float)posY / win->getContentScale().x));
#else
            win->onWindowPos(posX, posY);
#endif
        };

        // always pass all interaction through the global HID callback functions
        win->setKeyCallback(WindowManager::globalKeyCb);
        win->setCharCallback(WindowManager::globalCharCb);
        win->setMouseButtonCallback(WindowManager::globalMouseButCb);
        win->setCursorPosCallback(WindowManager::globalMouseCursorCb);
        win->setWindowSizeCallback(WindowManager::globalWindowSizeCb);
        win->setWindowCloseCallback(WindowManager::globalWindowCloseCb);
        win->setWindowMaximizeCallback(WindowManager::globalWindowMaximizeCb);
        win->setWindowIconifyCallback(WindowManager::globalWindowIconifyCb);
        win->setWindowFocusCallback(WindowManager::globalWindowFocusCb);
        win->setWindowPosCallback(WindowManager::globalWindowPosCb);
        win->setScrollCallback(WindowManager::globalScrollCb);
        win->setWindowRefreshCallback(WindowManager::globalWindowRefreshCb);
    }

#ifdef ARA_USE_GLFW
    // set diagonal mouse cursor icons, if already loaded
    if (m_diagResizeAscCursor) {
        win->setMouseCursorIcon(m_diagResizeAscCursor, WinMouseIcon::lbtrResize);
    }
    if (m_diagResizeDescCursor) {
        win->setMouseCursorIcon(m_diagResizeDescCursor, WinMouseIcon::ltbrResize);
    }
#endif

    m_multCtx = m_windows.size() > 1;
    return win;
}

void WindowManager::removeWin(GLWindow *win, bool terminateGLFW) {
    if (!win) {
        return;
    }

    // remove window callbacks
    auto kcIt = m_keyCbMap.find(win->getCtx());
    if (kcIt != m_keyCbMap.end()) {
        m_keyCbMap.erase(kcIt);
    }

    auto ccIt = m_charCbMap.find(win->getCtx());
    if (ccIt != m_charCbMap.end()) {
        m_charCbMap.erase(ccIt);
    }

    auto mbIt = m_mouseButCbMap.find(win->getCtx());
    if (mbIt != m_mouseButCbMap.end()) {
        m_mouseButCbMap.erase(mbIt);
    }

    auto mcIt = m_cursorCbMap.find(win->getCtx());
    if (mcIt != m_cursorCbMap.end()) {
        m_cursorCbMap.erase(mcIt);
    }

    auto wrIt = m_winResizeCbMap.find(win->getCtx());
    if (wrIt != m_winResizeCbMap.end()) {
        m_winResizeCbMap.erase(wrIt);
    }

    auto wpIt = m_winPosCbMap.find(win->getCtx());
    if (wpIt != m_winPosCbMap.end()) {
        m_winPosCbMap.erase(wpIt);
    }

    auto wcIt = m_winCloseCbMap.find(win->getCtx());
    if (wcIt != m_winCloseCbMap.end()) {
        m_winCloseCbMap.erase(wcIt);
    }

    auto wmIt = m_winMaxmimizeCbMap.find(win->getCtx());
    if (wmIt != m_winMaxmimizeCbMap.end()) {
        m_winMaxmimizeCbMap.erase(wmIt);
    }

    auto scIt = m_scrollCbMap.find(win->getCtx());
    if (scIt != m_scrollCbMap.end()) {
        m_scrollCbMap.erase(scIt);
    }

    auto icIt = m_winIconfifyCbMap.find(win->getCtx());
    if (icIt != m_winIconfifyCbMap.end()) {
        m_winIconfifyCbMap.erase(icIt);
    }

    auto fmIt = m_winFocusCbMap.find(win->getCtx());
    if (fmIt != m_winFocusCbMap.end()) {
        m_winFocusCbMap.erase(fmIt);
    }

    auto wreIt = m_winRefreshCbMap.find(win->getCtx());
    if (wreIt != m_winRefreshCbMap.end()) {
        m_winRefreshCbMap.erase(wreIt);
    }

    if (m_shareCtx == win->getCtx()) {
        m_shareCtx = nullptr;
    }

    auto wIt = std::find_if(m_windows.begin(), m_windows.end(),
                            [win](const std::unique_ptr<GLWindow> &w) { return w.get() == win; });

    if (wIt != m_windows.end()) {
        // win->removeMouseCursors();
        win->destroy(terminateGLFW);
        m_windows.erase(wIt);
    }
}

void WindowManager::addEvtLoopCb(const std::function<bool()> &f) {
    std::unique_lock<std::mutex> l(m_evtLoopCbMtx);
    m_evtLoopCbs.push_back(f);
}

// Note: all global...Cb are called on the main thread (same as startEventLoop()
// )
void WindowManager::globalKeyCb(GLContext ctx, int key, int scancode, int action, int mods) {
    // on asus GL503V debian linux with spanish keyboard layout, arrow left right are different keycodes glfw doesn't
    // seem to know them...
    if (key == 263) {
        key = 18;
    } else if (key == 262) {
        key = 20;
    } else if (key == 265) {
        key = 19;
    } else if (key == 264) {
        key = 21;
    }

    // get the window user pointer -> this is the GWindowManger instance, set above in "addWin" method pass a pointer
    // to this WindowManager instance to glfw, to be used inside the HID callback functions
    auto winMan = getThis(ctx);
    if (!winMan) {
        return;
    }

    // go through the window specific keyCallback vectors and call the corresponding functions
    auto winIt = winMan->m_keyCbMap.find(ctx);
    if (winIt != winMan->m_keyCbMap.end()) {
        winIt->second(key, scancode, action, mods);
    }

    // call the global function which applies for all windows
    for (auto &it : winMan->m_globalKeyCb) {
        it.second(ctx, key, scancode, action, mods);
    }
}

void WindowManager::globalCharCb(GLContext ctx, unsigned int codepoint) {
    // get the window user pointer -> this is the GWindowManger instance, set above in "addWin" method
    auto winMan = getThis(ctx);
    if (!winMan) {
        return;
    }

    // go through the window specific keyCallback vectors and call the corresponding functions
    auto winIt = winMan->m_charCbMap.find(ctx);
    if (winIt != winMan->m_charCbMap.end()) {
        winIt->second(codepoint);
    }

    // call the global function which applies for all windows
    for (auto &it : winMan->m_globalCharCb) {
        it.second(ctx, codepoint);
    }
}

void WindowManager::globalMouseButCb(GLContext ctx, int button, int action, int mods) {
    // get the window user pointer -> this is the GWindowManger instance, set above in "addWin" method
    auto winMan = getThis(ctx);
    if (!winMan) {
        return;
    }

    // go through the mouseCallback vectors and call the corresponding functions of the actual window that is the
    // rootWidget callback of this window
    auto winIt = winMan->m_mouseButCbMap.find(ctx);
    if (winIt != winMan->m_mouseButCbMap.end()) {
        winIt->second(button, action, mods);
    }

    // call the global function which applies for all windows
    for (auto &it : winMan->m_globalButtonCb) {
        it.second(ctx, button, action, mods);
    }
}

void WindowManager::globalMouseCursorCb(GLContext ctx, double xpos, double ypos) {
    // get the window user pointer -> this is the GWindowManger instance, set
    // above in "addWin" method
    auto winMan = getThis(ctx);
    if (!winMan) {
        return;
    }

    // go through the cursorCallback vectors and call the corresponding
    // functions
    auto cbIt = winMan->m_cursorCbMap.find(ctx);
    if (cbIt != winMan->m_cursorCbMap.end()) {
        cbIt->second(xpos, ypos);
    }

    // call the global function which applies for all windows
    for (auto &it : winMan->m_globalMouseCursorCb) {
        it.second(ctx, xpos, ypos);
    }

    // save last mouse Pos
    winMan->m_lastMouseX = xpos;
    winMan->m_lastMouseY = ypos;
}

void WindowManager::globalWindowSizeCb(GLContext ctx, int width, int height) {
    // get the window user pointer -> this is the GWindowManger instance, set
    // above in "addWin" method
    auto winMan = getThis(ctx);
    if (!winMan) return;

    // go through the windowSizeCallback vectors and call the corresponding
    // functions
    auto cbIt = winMan->m_winResizeCbMap.find(ctx);
    if (cbIt != winMan->m_winResizeCbMap.end()) cbIt->second(width, height);

    // call the global function which applies for all windows
    for (auto &it : winMan->m_globalWinResizeCb) it.second(ctx, width, height);
}

void WindowManager::globalWindowCloseCb(GLContext ctx) {
    // get the window user pointer -> this is the GWindowManger instance, set
    // above in "addWin" method
    auto winMan = getThis(ctx);
    if (!winMan) return;

    // go through the keyCallback vectors and call the corresponding functions
    auto cbIt = winMan->m_winCloseCbMap.find(ctx);
    if (cbIt != winMan->m_winCloseCbMap.end()) cbIt->second();

    // call the global function which applies for all windows
    for (auto &it : winMan->m_globalWinCloseCb) it.second(ctx);
}

void WindowManager::globalWindowMaximizeCb(GLContext ctx, int flag) {
    // get the window user pointer -> this is the GWindowManger instance, set
    // above in "addWin" method
    auto winMan = getThis(ctx);
    if (!winMan) return;

    // go through the keyCallback vectors and call the corresponding functions
    auto cbIt = winMan->m_winMaxmimizeCbMap.find(ctx);
    if (cbIt != winMan->m_winMaxmimizeCbMap.end()) cbIt->second(flag);

    // call the global function which applies for all windows
    for (auto &it : winMan->m_globalWinMaximizeCb) it.second(ctx, flag);
}

void WindowManager::globalWindowIconifyCb(GLContext ctx, int flag) {
    // get the window user pointer -> this is the GWindowManger instance, set
    // above in "addWin" method
    auto winMan = getThis(ctx);
    if (!winMan) return;

    // go through the keyCallback vectors and call the corresponding functions
    auto cbIt = winMan->m_winIconfifyCbMap.find(ctx);
    if (cbIt != winMan->m_winIconfifyCbMap.end()) cbIt->second(flag);

    // call the global function which applies for all windows
    for (auto &it : winMan->m_globalWinIconifyCb) it.second(ctx, flag);
}

void WindowManager::globalWindowFocusCb(GLContext ctx, int flag) {
    // get the window user pointer -> this is the GWindowManger instance, set
    // above in "addWin" method
    auto winMan = getThis(ctx);
    if (!winMan) return;

    auto winIt    = std::find_if(winMan->getWindows()->begin(), winMan->getWindows()->end(),
                                 [ctx](const std::unique_ptr<GLWindow> &p) { return p->getCtx() == ctx; });
    auto isWinMan = winIt != winMan->getWindows()->end();

    if (winMan->m_fixFocusWin && isWinMan && ctx != winMan->m_fixFocusWin->getCtx())
        GLWindow::focusWin(winMan->m_fixFocusWin->getCtx());

    // save this window
    if (isWinMan) winMan->m_focusedWin = winIt->get();

    // go through the keyCallback vectors and call the corresponding functions
    auto cbIt = winMan->m_winFocusCbMap.find(ctx);
    if (cbIt != winMan->m_winFocusCbMap.end()) {
        cbIt->second(flag);
    }

    // call the global function which applies for all windows
    for (auto &it : winMan->m_globalWinFocusCb) it.second(ctx, flag);
}

void WindowManager::globalWindowPosCb(GLContext ctx, int posx, int posy) {
    // get the window user pointer -> this is the GWindowManger instance, set
    // above in "addWin" method
    auto winMan = getThis(ctx);
    if (!winMan) return;

    // go through the keyCallback vectors and call the corresponding functions
    auto cbIt = winMan->m_winPosCbMap.find(ctx);
    if (cbIt != winMan->m_winPosCbMap.end()) cbIt->second(posx, posy);

    // call the global function which applies for all windows
    for (auto &it : winMan->m_globalWinPosCb) it.second(ctx, posx, posy);
}

void WindowManager::globalScrollCb(GLContext ctx, double posx, double posy) {
    // get the window user pointer -> this is the GWindowManger instance, set
    // above in "addWin" method
    auto winMan = getThis(ctx);
    if (!winMan) return;

    // go through the keyCallback vectors and call the corresponding functions
    auto cbIt = winMan->m_scrollCbMap.find(ctx);
    if (cbIt != winMan->m_scrollCbMap.end()) cbIt->second(posx, posy);

    // call the global function which applies for all windows
    for (auto &it : winMan->m_globalScrollCb) it.second(ctx, posx, posy);
}

void WindowManager::globalWindowRefreshCb(GLContext ctx) {
    // get the window user pointer -> this is the GWindowManger instance, set
    // above in "addWin" method
    auto winMan = getThis(ctx);
    if (!winMan) return;

    // go through the keyCallback vectors and call the corresponding functions
    auto cbIt = winMan->m_winRefreshCbMap.find(ctx);
    if (cbIt != winMan->m_winRefreshCbMap.end()) cbIt->second();

    // call the global function which applies for all windows
    for (auto &it : winMan->m_globalWinRefreshCbMap) it.second(ctx);
}

[[maybe_unused]] void WindowManager::setSwapInterval(unsigned int winNr, bool swapInterval) {
    if (static_cast<unsigned int>(m_windows.size()) >= winNr) m_windows[winNr]->setVSync(swapInterval);
}

void WindowManager::getHwInfo() {
#ifdef ARA_USE_GLFW
    m_monitors = glfwGetMonitors(&m_dispCount);
    getVideoModes();
    getMonitorOffsets();
#endif
}

#ifdef ARA_USE_GLFW

std::vector<GLFWvidmode> WindowManager::getVideoModes() {
    if (m_dispOffsets.empty()) {
        for (int i = 0; i < m_dispCount; i++) {
            const GLFWvidmode *thisMode = glfwGetVideoMode(m_monitors[i]);
            m_dispModes.emplace_back();
            m_dispModes.back().width  = thisMode->width;
            m_dispModes.back().height = thisMode->height;
        }
    }
    return m_dispModes;
}

std::vector<std::pair<int, int>> WindowManager::getMonitorOffsets() {
    if (m_dispOffsets.empty()) {
        for (int i = 0; i < m_dispCount; i++) {
            int monOffsX, monOffsY;
            glfwGetMonitorPos(m_monitors[i], &monOffsX, &monOffsY);

            m_dispOffsets.emplace_back(monOffsX, monOffsY);
            m_displayInfo.push_back(DisplayBasicInfo{(uint32_t)monOffsX, (uint32_t)monOffsY,
                                                     (uint32_t)m_dispModes[i].width, (uint32_t)m_dispModes[i].height});
        }
    }
    return m_dispOffsets;
}

[[maybe_unused]] GLFWvidmode const &WindowManager::getDispMode(unsigned int idx) {
    if (m_dispModes.size() > idx)
        return m_dispModes[idx];
    else {
        return m_defaultDispMode;
    }
}

GLFWcursor *WindowManager::createMouseCursor(std::string &file, float xHot, float yHot) {
    GLFWimage image;
    Texture   tex(m_glbase);

#ifdef ARA_USE_FREEIMAGE
    FIBITMAP *pBitmap = nullptr;

#ifdef ARA_DEBUG
    if (fs::exists(fs::path("resdata") / fs::path(file))) {
        pBitmap = tex.ImageLoader((fs::path("resdata") / fs::path(file)).string().c_str(), 0);
#else
    if (m_res) {
        std::vector<uint8_t> vp;
        m_res->loadResource(nullptr, vp, file);
        if (!vp.size()) return nullptr;

        FreeImg_MemHandler mh(&vp[0], vp.size());
        FREE_IMAGE_FORMAT  fif = FreeImage_GetFileTypeFromHandle(mh.io(), (fi_handle)&mh, 0);
        if ((pBitmap = FreeImage_LoadFromHandle(fif, mh.io(), (fi_handle)&mh, 0)) == nullptr) return nullptr;
#endif
        image.pixels                    = (GLubyte *)FreeImage_GetBits(pBitmap);
        image.width                     = (int)FreeImage_GetWidth(pBitmap);
        image.height                    = (int)FreeImage_GetHeight(pBitmap);
        FREE_IMAGE_COLOR_TYPE colorType = FreeImage_GetColorType(pBitmap);

        GLFWcursor *c = glfwCreateCursor(&image, (int)((float)image.width * xHot), (int)((float)image.height * yHot));
        FreeImage_Unload(pBitmap);
        return c;
    }
#endif
    return nullptr;
}

void WindowManager::loadMouseCursors() {
    // glfw is missing standard mouse cursors for diagonal resizing ...
    // so we have to load them manually. In order to keep this class general,
    // but compact in use, we try once to load those image by standard names and
    // folders, if they are not there, they have to be loaded by explicitly
    // calling the method above
    string mouseCursoIconAsc  = "Icons/mouse_cursor_asc.png";
    string mouseCursoIconDesc = "Icons/mouse_cursor_desc.png";

    m_diagResizeAscCursor  = createMouseCursor(mouseCursoIconAsc, 0.5f, 0.5f);
    m_diagResizeDescCursor = createMouseCursor(mouseCursoIconDesc, 0.5f, 0.5f);
}

#endif

[[maybe_unused]] unsigned int WindowManager::getMonitorWidth(unsigned int winNr) {
    if (static_cast<unsigned int>(m_windows.size()) >= winNr - 1) return m_windows[winNr]->getMonitorWidth();
    return 0;
}

[[maybe_unused]] unsigned int WindowManager::getMonitorHeight(unsigned int winNr) {
    if (static_cast<unsigned int>(m_windows.size()) >= winNr - 1) return m_windows[winNr]->getMonitorHeight();
    return 0;
}

void WindowManager::callGlobalMouseButCb(int button) {
    for (auto &it : m_globalButtonCb) it.second(nullptr, button, 0, 0);
}
}  // namespace ara

#endif
