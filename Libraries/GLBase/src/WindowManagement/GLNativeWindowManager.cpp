//
// Created by hahne on 28.04.2025.
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

#include "GLNativeWindowManager.h"

namespace ara {

GLNativeWindowManager::GLNativeWindowManager() {
#ifdef _WIN32
    DWORD          DispNum       = 0;
    DISPLAY_DEVICE DisplayDevice = {0};
    DEVMODEA       defaultMode{};

    // initialize DisplayDevice
    DisplayDevice.cb = sizeof(DisplayDevice);

    // get all display devices
    while (EnumDisplayDevices(nullptr, DispNum, &DisplayDevice, 0)) {
        defaultMode.dmSize = sizeof(DEVMODE);
        if (!EnumDisplaySettingsA((LPSTR)DisplayDevice.DeviceName, ENUM_REGISTRY_SETTINGS, &defaultMode))
            OutputDebugStringA("Store default failed\n");

        if (DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
            m_displays.emplace_back(NativeDisplay());
            m_displays.back().width  = defaultMode.dmPelsWidth;
            m_displays.back().height = defaultMode.dmPelsHeight;
            m_displays.back().offsX  = defaultMode.dmPosition.x;
            m_displays.back().offsY  = defaultMode.dmPosition.y;
        }

        DisplayDevice.cb = sizeof(DisplayDevice);
        ++DispNum;
    }  // end while for all display devices

    m_dispCount = m_displays.size();
#endif
}

void GLNativeWindowManager::runMainLoop(const std::function<void(double time, double dt, unsigned int ctxNr)>& f) {
    m_run        = true;
    m_dispFunc = f;

    while (m_run) {
        iterate(true);
    }

    for (const auto &it : m_windows) {
        it->destroy();
    }
}

void GLNativeWindowManager::iterate(bool only_open_windows) {
    m_drawMtx.lock();

    // proc open / close request
    for (const auto &it : m_windows) {
        if (it->getRequestOpen()) {
            it->open();
        }
        if (it->getRequestClose()) {
            it->close();
        }
    }

    // update windows
    if (m_multCtx) {
        m_winIdx = 0;
        for (auto &it : m_windows) {
            if (only_open_windows ? it->isOpen() : true) {
                it->makeCurrent();
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // JUST FOR DEMO: should be
                // done only when needed
                if (m_dispFunc) {
                    m_dispFunc(m_lastTime, m_medDt, m_winIdx);
                }
                it->swap();
            }
            m_winIdx++;
        }
    } else if (!m_windows.empty()) {
        if (m_windows[0]->isOpen()) {
            m_windows[0]->makeCurrent();
            if (m_dispFunc) {
                m_dispFunc(m_lastTime, m_medDt, m_winIdx);
            }
            m_windows[0]->swap();
        }
    }

    m_drawMtx.unlock();

#if !defined(_WIN32) && !defined(__ANDROID__)
    glfwPollEvents();  // Poll for and process events
#endif

    // check if new windows were requested
    for (auto &it : m_addWindows) {
        addWin(it);
    }

    // clear the list of added windows
    if (!m_addWindows.empty()) {
        m_addWindows.clear();
    }

#ifdef _WIN32
    wglMakeCurrent(nullptr, nullptr);
#endif
}


GLWindowBase *GLNativeWindowManager::addWin(glWinPar& gp) {
    m_windows.emplace_back(std::make_unique<GLWindow>());

    // if there is no explicit request to share a specific content, and we already got another context to share, take
    // the first added window as a shared context
    gp.shareCont = gp.shareCont ? gp.shareCont : m_shareCtx;
    gp.doInit    = false;
    gp.nrSamples = 2;

    if (!m_windows.back()->create(gp)) {
        std::cerr << "GWindowManager addWin windows.back().create failed " << std::endl;
        return nullptr;
    }

    if (static_cast<unsigned int>(m_windows.size()) == 1) {
        m_shareCtx = m_windows.back().get();
    }

    // add new window specific callback functions
    m_keyCbMap[m_windows.back().get()]      = std::vector<std::function<void(int, int, int, int)>>();
    m_mouseButCbMap[m_windows.back().get()] = std::vector<std::function<void(int, int, int, double, double)>>();
    m_multCtx                             = m_windows.size() > 1;
    return m_windows.back().get();
}

void GLNativeWindowManager::gWinKeyCallback(GLWindow *window, int key, int scancode, int action, int mods) {
    // go through the keyCallback vectors and call the corresponding
    // functions
    auto winIt = m_keyCbMap.find(window);
    if (winIt != m_keyCbMap.end()) {
        for (const auto &it : m_keyCbMap[window]) {
            it(key, scancode, action, mods);
        }
    }

    // call the global function which applies for all windows
    if (m_keyCbFun) {
        m_keyCbFun(window, key, scancode, action, mods);
    }
}

void GLNativeWindowManager::gMouseButCallback(GLWindow *window, int button, int action, int mods) {
    // go through the mouseCallback vectors and call the corresponding
    // functions of the actual window that is the rootWidget callback of
    // this window
    for (const auto &it : m_mouseButCbMap[window]) {
        it(button, action, mods, m_lastMousePos.x, m_lastMousePos.y);
    }

    // call the global function which applies for all windows
    if (m_mouseButtonCbFun) {
        m_mouseButtonCbFun(window, button, action, mods);
    }
}

void GLNativeWindowManager::gMouseCursorCallback(GLWindow *window, double xpos, double ypos) {
    // go through the keyCallback vectors and call the corresponding
    // functions
    auto keyIt = m_cursorCbMap.find(window);
    if (keyIt != m_cursorCbMap.end()) {
        for (auto &it : m_cursorCbMap[window]) {
            it(xpos, ypos);
        }
    }

    // call the global function which applies for all windows
    if (m_mouseCursorCbFun) {
        m_mouseCursorCbFun(window, xpos, ypos);
    }

    // save last mouse Pos
    m_lastMousePos.x = static_cast<float>(xpos);
    m_lastMousePos.y = static_cast<float>(ypos);
}

void GLNativeWindowManager::addKeyCallback(unsigned int winInd, const std::function<void(int, int, int, int)>& func) {
    if (static_cast<unsigned int>(m_windows.size()) >= (winInd + 1)) {
        m_keyCbMap[m_windows[winInd].get()].emplace_back(func);
    } else {
        LOGE << "tav::GLWindowManager::setKeyCallback Error: m_window doesn´t exist.";
    }
}

void GLNativeWindowManager::addMouseButCallback(unsigned int winInd, const std::function<void(int, int, int, double, double)>& func) {
    if (static_cast<unsigned int>(m_windows.size()) >= (winInd + 1)) {
        m_mouseButCbMap[m_windows[winInd].get()].push_back(func);
    } else {
        LOGE << "tav::GLNativeWindowManager::addMouseButCallback Error: m_window doesn´t exist.";
    }
}

void GLNativeWindowManager::addCursorCallback(unsigned int winInd, const std::function<void(double, double)>& func) {
    if (static_cast<unsigned int>(m_windows.size()) >= (winInd + 1)) {
        m_cursorCbMap[m_windows[winInd].get()].push_back(func);
    } else {
        LOGE << "tav::GLNativeWindowManager::addCursorCallback Error: m_window doesn´t exist.";
    }
}

unsigned int GLNativeWindowManager::getInd(GLWindow *win) {
    unsigned int ctxInd = 0;
    unsigned int winInd = 0;
    for (auto & m_window : m_windows) {
        if (win == m_window.get()) {
            ctxInd = winInd;
        }
        ++winInd;
    }
    return ctxInd;
}

void GLNativeWindowManager::open(unsigned int nr) {
    if (m_windows.size() > nr) {
        m_windows[nr]->open();
    }
}

void GLNativeWindowManager::hide(unsigned int nr) {
    if (m_windows.size() > nr) {
        m_windows[nr]->close();
    }
}

void GLNativeWindowManager::closeAll() {
    m_run = false;
    for (auto &it : m_windows) {
        it->close();
    }
}

int GLNativeWindowManager::getDispOffsetX(unsigned int idx) {
    if (m_dispOffsets.size() > idx) {
        return m_dispOffsets[idx].first;
    } else {
        return 0;
    }
}

int GLNativeWindowManager::getDispOffsetY(unsigned int idx) {
    if (m_dispOffsets.size() > idx) {
        return m_dispOffsets[idx].second;
    } else {
        return 0;
    }
}

void GLNativeWindowManager::IterateAll(bool only_open_windows, std::function<void(double time, double dt, unsigned int ctxNr)> renderFunction) {
    if ((m_dispFunc = std::move(renderFunction))) {
        iterate(only_open_windows);
    }
}

void GLNativeWindowManager::EndMainLoop() {
    for (auto &it : m_windows) {
        it->destroy();
    }
}


}