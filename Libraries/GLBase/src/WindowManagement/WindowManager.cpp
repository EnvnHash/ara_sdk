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

#include <GLBase.h>
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)

#include <WindowManagement/WindowManager.h>
#include <Utils/Texture.h>
#include <Asset/AssetManager.h>

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

    while (m_run) {
        iterate(true);
    }

    for (auto &it : m_windows) {
        it->destroy(false);
    }

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
        LOG <<  "FPS: " << (1.0 / m_medDt) << " dt: " << m_medDt;
        m_lastPrintFps = m_lastTime;
    }

    GLWindow::pollEvents();  // Poll for and process events.
    // Note: this should be glfwWaitEvents in case of event based rendering, but e.g. on window resizing on windows
    // events are blocked, so no update would be possible during this operation. This is why we need to manually
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
            glViewport(0, 0, static_cast<GLsizei>(it->getWidth()), static_cast<GLsizei>(it->getHeight()));

            // note: screen clearing is managed by draw function
            // if there is global draw function set, take it, otherwise take the Windows' draw function
            if (m_globalDrawFunc) {
                m_globalDrawFunc(m_lastTime, m_medDt, m_winInd);
            } else if (it->getDrawFunc()) {
                it->getDrawFunc()(m_lastTime, m_medDt, m_winInd);
            }
            it->swap();
        }
        ++m_winInd;
    }

    // check if new windows were requested
    for (auto &it : m_addWindows) {
        addWin(it);
    }

    GLWindow::makeNoneCurrent();
}

#ifdef _WIN32
std::thread WindowManager::getOutOfBoundsHIDLoop() {
    return std::thread([this] {
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
                    if (!x && !y) {
                        return;
                    }

                    for (auto &w : *getWindows()) {
                        if (w && w->isInited() && w->isOpen()) {
                            isOutOfBounds = isOutOfBounds ||
                                            !(x >= w->getPosition().x && x <= (w->getPosition().x + w->getSize().x) &&
                                              y >= w->getPosition().y && y <= (w->getPosition().y + w->getSize().y));
                        }
                    }
                }
            }

            for (int i = 0; i < 2; i++) {
                if (isOutOfBounds && mousePressed[i] && !m_didSend[i]) {
                    m_didSend[i] = true;
                    callGlobalHidCb(this, winCb::MouseButton, i == 0 ? GLFW_MOUSE_BUTTON_LEFT : GLFW_MOUSE_BUTTON_RIGHT);
                }

                if (!mousePressed[i] && m_didSend[i]) {
                    m_didSend[i] = false;
                }
            }

            this_thread::sleep_for(5ms);
        }
    });
}
#endif

void WindowManager::startEventLoop() {
#ifndef __ANDROID__
    GLWindow::makeNoneCurrent();  // be sure no context is current in order to be able to create new windows
    m_run          = true;
    m_mainThreadId = this_thread::get_id();

#ifdef _WIN32
    // glfw can't capture clicks outside the active windows ... so do this here manually
    auto th = getOutOfBoundsHIDLoop();
#endif

    m_startEventLoopSema.notify();

    while (m_run) {
        GLWindow::waitEvents();  // process HID callbacks, there is a regular interval from glfw aprox. each second
        procEventQueue();
    }

#ifdef _WIN32
    if (th.joinable()) {
        th.join();
    }
#endif

    m_custEventQueue.clear();
    m_stopEventLoopSema.notify();
#endif
}

void WindowManager::stopEventLoop() {
    m_run = false;
    GLWindow::postEmptyEvent();
}

void WindowManager::procEventQueue() {
    if (!m_custEventQueue.empty()) {
        bool funcSuccess = false;
        m_evtLoopCbMtx.lock();
        for (auto it = m_custEventQueue.begin(); it != m_custEventQueue.end();) {
            funcSuccess = (*it)();
            if (m_breakEvtLoop) {
                break;
            }

            if (funcSuccess) {
                it = m_custEventQueue.erase(it);
            } else {
                ++it;
            }
        }
        m_evtLoopCbMtx.unlock();
    }
}

void WindowManager::startThreadedRendering() const {
    GLWindow::makeNoneCurrent();

    // run through all existing windows and start renderloops for them
    for (auto &it : m_windows) {
        if (!it->isRunning() && it->getDrawFunc()) {
            it->startDrawThread(it->getDrawFunc());
        }
    }
}

void WindowManager::stopThreadedRendering() const {
    for (auto &it : m_windows) {
        if (it->getDrawFunc()) {
            // DON'T try to stop the GLBase loop by this function, it works differently. we filter
            // it out by checking for its draw function
            it->stopDrawThread();
        }
    }
}

GLWindow *WindowManager::addWin(const glWinPar& gp) {
#ifdef _WIN32
    {
        unique_lock<mutex> lock(m_globMouseLoopMtx);
#endif
        m_windows.emplace_back(make_unique<GLWindow>());

        // if there is no explicit request to share a specific content, and we already got another context to share,
        // take the first added window as a shared context
        auto wp = gp;
        wp.shareCont = gp.shareCont ? gp.shareCont : m_shareCtx;
        wp.hidExtern = true;  // HID input must be managed by WindowManager
        m_windows.back()->init(wp);
        if (!m_windows.back()->getCtx()) {
            return nullptr;
        }

#ifdef _WIN32
    }
#endif

#ifdef ARA_USE_GLFW
    // pass a pointer to this WindowManager instance to glfw, to be used inside the HID callback functions
    glfwSetWindowUserPointer(m_windows.back()->getCtx(), reinterpret_cast<void *>(this));
#endif

    if (static_cast<unsigned int>(m_windows.size()) == 1) {
        m_shareCtx = static_cast<void *>(m_windows.back()->getCtx());
    }

    auto win = m_windows.back().get();

    if (gp.hidInput) {
        // bind window HID callbacks to globalCallbacks by adding them to the
        // callback maps
        m_winHidCbMap[winCb::Key][win->getCtx()]      = [win](int p1, int p2, int p3, int p4) { win->onKey(p1, p2, p3, p4); };
        m_winHidCbMap[winCb::Char][win->getCtx()]     = std::function([win](unsigned int p1) { win->onChar(p1); });
        m_winHidCbMap[winCb::MouseButton][win->getCtx()] = [win](int button, int action, int mods) {
            win->onMouseButton(button, action, mods);
        };
        m_winHidCbMap[winCb::WindowClose][win->getCtx()]     = [win] { win->onWindowClose(); };
        m_winHidCbMap[winCb::WindowMaximize][win->getCtx()] = std::function([win](int maximized) { win->onWindowMaximize(maximized); });
        m_winHidCbMap[winCb::WindowIconify][win->getCtx()]  = std::function([win](int iconified) { win->onWindowIconify(iconified); });
        m_winHidCbMap[winCb::WindowFocus][win->getCtx()]     = std::function([win](int focused) { win->onWindowFocus(focused); });
        m_winHidCbMap[winCb::WindowRefresh][win->getCtx()]   = [win] { win->onWindowRefresh(); };
        m_winHidCbMap[winCb::Scroll][win->getCtx()]       = std::function([win](double xOffs, double yOffs) { win->onScroll(xOffs, yOffs); });

        m_winHidCbMap[winCb::CursorPos][win->getCtx()] = std::function([win](double x, double y) {
#if defined(_WIN32) || defined(__linux__)
            win->onMouseCursor(x / win->getContentScale().x, y / win->getContentScale().y);
#else
            win->onMouseCursor(x, y);
#endif
        });

        m_winHidCbMap[winCb::WindowSize][win->getCtx()] = std::function([win](int w, int h) {
#if defined(_WIN32) || defined(__linux__)
            win->onWindowSize(static_cast<int>(static_cast<float>(w) / win->getContentScale().x), static_cast<int>(static_cast<float>(h) / win->getContentScale().x));
#else
            win->onWindowSize(w, h);
#endif
        });

        m_winHidCbMap[winCb::WindowPos][win->getCtx()] = std::function([win](int posX, int posY) {
#ifdef _WIN32
            win->onWindowPos(static_cast<int>(static_cast<float>(posX) / win->getContentScale().x),
                             static_cast<int>(static_cast<float>(posY) / win->getContentScale().x));
#else
            win->onWindowPos(posX, posY);
#endif
        });

        // always pass all interaction through the global HID callback functions
        win->setKeyCallback(globalKeyCb);
        win->setCharCallback(globalCharCb);
        win->setMouseButtonCallback(globalMouseButCb);
        win->setCursorPosCallback(globalMouseCursorCb);
        win->setWindowSizeCallback(globalWindowSizeCb);
        win->setWindowCloseCallback(globalWindowCloseCb);
        win->setWindowMaximizeCallback(globalWindowMaximizeCb);
        win->setWindowIconifyCallback(globalWindowIconifyCb);
        win->setWindowFocusCallback(globalWindowFocusCb);
        win->setWindowPosCallback(globalWindowPosCb);
        win->setScrollCallback(globalScrollCb);
        win->setWindowRefreshCallback(globalWindowRefreshCb);
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
    for (auto& [cbTp, map] : m_winHidCbMap) {
        auto kcIt = map.find(win->getCtx());
        if (kcIt != map.end()) {
            map.erase(kcIt);
        }
    }

    if (m_shareCtx == win->getCtx()) {
        m_shareCtx = nullptr;
    }

    auto wIt = ranges::find_if(m_windows, [win](const auto &w) { return w.get() == win; });

    if (wIt != m_windows.end()) {
        win->destroy(terminateGLFW);
        m_windows.erase(wIt);
    }
}

void WindowManager::addEvtLoopCb(const std::function<bool()> &f) {
    std::unique_lock<std::mutex> l(m_evtLoopCbMtx);
    m_custEventQueue.emplace_back(f);
}

// Note: all global...Cb are called on the main thread (same as startEventLoop())
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

    callWinAndGlobalHidCb(ctx, winCb::Key, key, scancode, action, mods);
}

void WindowManager::globalCharCb(GLContext ctx, unsigned int codepoint) {
    callWinAndGlobalHidCb(ctx, winCb::Char, codepoint);
}

void WindowManager::globalMouseButCb(GLContext ctx, int button, int action, int mods) {
    callWinAndGlobalHidCb(ctx, winCb::MouseButton, button, action, mods);
}

void WindowManager::globalMouseCursorCb(GLContext ctx, double xpos, double ypos) {
    callWinAndGlobalHidCb(ctx, winCb::CursorPos, xpos, ypos);

    // save last mouse Pos
    auto winMan = getThis(ctx);
    if (!winMan) {
        return;
    }
    winMan->m_lastMouseX = xpos;
    winMan->m_lastMouseY = ypos;
}

void WindowManager::globalWindowSizeCb(GLContext ctx, int width, int height) {
    callWinAndGlobalHidCb(ctx, winCb::WindowSize, width, height);

}

void WindowManager::globalWindowCloseCb(GLContext ctx) {
    callWinAndGlobalHidCb(ctx, winCb::WindowClose);
}

void WindowManager::globalWindowMaximizeCb(GLContext ctx, int flag) {
    callWinAndGlobalHidCb(ctx, winCb::WindowMaximize, flag);
}

void WindowManager::globalWindowIconifyCb(GLContext ctx, int flag) {
    callWinAndGlobalHidCb(ctx, winCb::WindowIconify, flag);
}

void WindowManager::globalWindowFocusCb(GLContext ctx, int flag) {
    // get the window user pointer -> this is the GWindowManger instance, set
    // above in "addWin" method
    auto winMan = getThis(ctx);
    if (!winMan) {
        return;
    }

    auto winIt    = ranges::find_if(*winMan->getWindows(),
                                    [ctx](const auto &p) { return p->getCtx() == ctx; });
    auto isWinMan = winIt != winMan->getWindows()->end();

    if (winMan->m_fixFocusWin && isWinMan && ctx != winMan->m_fixFocusWin->getCtx()) {
        GLWindow::focusWin(winMan->m_fixFocusWin->getCtx());
    }

    // save this window
    if (isWinMan) {
        winMan->m_focusedWin = winIt->get();
    }

    callWinAndGlobalHidCb(ctx, winCb::WindowFocus, flag);
}

void WindowManager::globalWindowPosCb(GLContext ctx, int posx, int posy) {
    callWinAndGlobalHidCb(ctx, winCb::WindowPos, posx, posy);
}

void WindowManager::globalScrollCb(GLContext ctx, double posx, double posy) {
    callWinAndGlobalHidCb(ctx, winCb::Scroll, posx, posy);
}

void WindowManager::globalWindowRefreshCb(GLContext ctx) {
    callWinAndGlobalHidCb(ctx, winCb::WindowRefresh);
}

void WindowManager::setSwapInterval(unsigned int winNr, bool swapInterval) const {
    if (static_cast<unsigned int>(m_windows.size()) >= winNr) {
        m_windows[winNr]->setVSync(swapInterval);
    }
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
            m_displayInfo.emplace_back(DisplayBasicInfo{static_cast<uint32_t>(monOffsX), static_cast<uint32_t>(monOffsY),
                                                     static_cast<uint32_t>(m_dispModes[i].width), static_cast<uint32_t>(m_dispModes[i].height)});
        }
    }
    return m_dispOffsets;
}

[[maybe_unused]] GLFWvidmode const &WindowManager::getDispMode(unsigned int idx) const {
    if (m_dispModes.size() > idx)
        return m_dispModes[idx];
    else {
        return m_defaultDispMode;
    }
}

GLFWcursor *WindowManager::createMouseCursor(const std::string &file, float xHot, float yHot) const {
    Texture   tex(m_glbase);

#ifdef ARA_USE_FREEIMAGE
    std::vector<uint8_t> vp;
    auto& al = m_glbase->getAssetManager()->getAssetLoader();
    al.loadAssetToMem(vp, file);
    if (vp.empty()) {
        return nullptr;
    }

    FreeImg_MemHandler mh(&vp[0], vp.size());
    FREE_IMAGE_FORMAT  fif = FreeImage_GetFileTypeFromHandle(mh.io(), (fi_handle)&mh, 0);
    FIBITMAP *pBitmap = nullptr;
    if ((pBitmap = FreeImage_LoadFromHandle(fif, mh.io(), (fi_handle)&mh, 0)) == nullptr) {
        return nullptr;
    }

    GLFWimage image;
    image.pixels    = FreeImage_GetBits(pBitmap);
    image.width     = static_cast<int>(FreeImage_GetWidth(pBitmap));
    image.height    = static_cast<int>(FreeImage_GetHeight(pBitmap));

    auto c = glfwCreateCursor(&image, static_cast<int>(static_cast<float>(image.width) * xHot), static_cast<int>(static_cast<float>(image.height) * yHot));
    FreeImage_Unload(pBitmap);
    return c;
#endif
}

void ara::WindowManager::loadMouseCursors() {
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

unsigned int WindowManager::getMonitorWidth(unsigned int winNr) const {
    if (static_cast<unsigned int>(m_windows.size()) >= winNr - 1) {
        return m_windows[winNr]->getMonitorWidth();
    }
    return 0;
}

unsigned int WindowManager::getMonitorHeight(unsigned int winNr) const {
    if (static_cast<unsigned int>(m_windows.size()) >= winNr - 1) {
        return m_windows[winNr]->getMonitorHeight();
    }
    return 0;
}

}  // namespace ara

#endif
