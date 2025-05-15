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

#include "WindowManagement/WindowBase.h"

using namespace glm;
using namespace std;

namespace ara {

void WindowBase::init(const glm::ivec2& pos, const glm::ivec2& size) {
    s_mousePos                  = ivec2{};
    s_mouse_down_right_init_pos = ivec2{};

    s_windowViewport.x = static_cast<float>(pos.x);
    s_windowViewport.y = static_cast<float>(pos.y);
    s_windowViewport.z = static_cast<float>(size.x);
    s_windowViewport.w = static_cast<float>(size.y);

    s_viewPort = vec4(pos, size.x * s_devicePixelRatio, size.y * s_devicePixelRatio);
    s_orthoMat = glm::ortho(0.f, static_cast<float>(size.x), static_cast<float>(size.y), 0.f);
}

// process opengl callbacks from another thread and delete them afterward if they returned true
void WindowBase::iterateGlCallbacks() {
    int i = 0;
    for (auto objIt = s_openGlCbs.begin(); objIt != s_openGlCbs.end();) {
        // iterate all cbs of assign to this Object ptr
        for (auto cbIt = objIt->second.begin(); cbIt != objIt->second.end();) {
            if ((cbIt->second)()) {
                cbIt = objIt->second.erase(cbIt);
            } else {
                ++cbIt;
            }
        }

        if (objIt->second.empty()) {
            objIt = s_openGlCbs.erase(objIt);
        } else {
            ++objIt;
        }
        ++i;
    }
}

// process hid events, received in the WinBase HID callbacks
// this function passes down HID events to the UINodes and by this assure
// that all UINode HID functions are called within the GL-context of their
// parent window
void WindowBase::procHid() {
    if (s_hidEvents.empty()) {
        return;
    }

    unique_lock<mutex> lock(s_procHidMtx);

    // key events have to be treated differently. mouse events can always be
    // overwritten with the latest event, but there may be multiple different key
    // events so closely after each other that they could get lost
    for (auto &val: s_keyEvents | views::values) {
        if (!val.empty()) {
            for (auto &kit : val) {
                kit();
            }
        }
        val.clear();
    }

    for (auto &val: s_hidEvents | views::values) {
        if (val) {
            val();
        }
        val = nullptr;
    }
}

void WindowBase::procChangeWin() {
    if (s_changeWinEvents.empty()) {
        return;
    }

    unique_lock<mutex> lock(s_changeWinMtx);
    for (auto &val: s_changeWinEvents | views::values) {
        if (val) {
            val();
        }
        val = nullptr;
    }
}

void WindowBase::addGlCb(void *cbName, const std::string &fName, std::function<bool()> func) {
    bool gotLock = s_drawMtx.try_lock();
    if (!s_openGlCbs.contains(cbName)) {
        s_openGlCbs[cbName] = std::unordered_map<std::string, std::function<bool()>>();
    }
    s_openGlCbs[cbName][fName] = std::move(func);
    if (gotLock) {
        s_drawMtx.unlock();
    }
}

bool WindowBase::hasCb(void *cbName, const std::string &fName) {
    bool gotLock = s_drawMtx.try_lock();
    bool ret     = (s_openGlCbs.contains(cbName) && s_openGlCbs[cbName].contains(fName));
    if (gotLock) {
        s_drawMtx.unlock();
    }
    return ret;
}

void WindowBase::eraseGlCb(void *cbName, const std::string &fName) {
    bool gotLock = s_drawMtx.try_lock();
    auto it      = s_openGlCbs.find(cbName);
    if (it != s_openGlCbs.end() && s_openGlCbs[cbName].contains(fName)) {
        s_openGlCbs[cbName].erase(fName);
    }
    if (gotLock) {
        s_drawMtx.unlock();
    }
}

void WindowBase::eraseGlCb(void *cbName) {
    bool gotLock = s_drawMtx.try_lock();
    auto it      = s_openGlCbs.find(cbName);
    if (it != s_openGlCbs.end()) {
        s_openGlCbs.erase(cbName);
    }
    if (gotLock) {
        s_drawMtx.unlock();
    }
}

void WindowBase::clearGlCbQueue() {
    bool gotLock = s_drawMtx.try_lock();
    s_openGlCbs.clear();
    if (gotLock) {
        s_drawMtx.unlock();
    }
}

}  // namespace ara
