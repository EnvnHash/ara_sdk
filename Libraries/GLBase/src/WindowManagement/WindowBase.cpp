#include "WindowManagement/WindowBase.h"

using namespace glm;
using namespace std;

namespace ara {

void WindowBase::init(uint x, uint y, uint scrWidth, uint scrHeight) {
    s_mousePos                  = ivec2(0);
    s_mouse_down_right_init_pos = ivec2(0);

    s_windowViewport.x = (float)x;
    s_windowViewport.y = (float)y;
    s_windowViewport.z = (float)scrWidth;
    s_windowViewport.w = (float)scrHeight;

    s_viewPort = vec4(static_cast<float>(x), static_cast<float>(y), static_cast<float>(scrWidth * s_devicePixelRatio),
                      static_cast<float>(scrHeight * s_devicePixelRatio));

    s_orthoMat = glm::ortho(0.f, static_cast<float>(scrWidth), static_cast<float>(scrHeight), 0.f);
}

// process opengl callbacks from other thread and delete them afterwards if they
// returned true
void WindowBase::iterateGlCallbacks() {
    int i = 0;
    for (auto objIt = s_openGlCbs.begin(); objIt != s_openGlCbs.end();)  // unordered_map<void*, unordered_map<string,
                                                                         // function<bool()>>>
    {
        // iterate all cbs of assign to this Object ptr
        for (auto cbIt = objIt->second.begin(); cbIt != objIt->second.end();) {
            if ((cbIt->second)())
                cbIt = objIt->second.erase(cbIt);
            else
                cbIt++;
        }

        if (objIt->second.empty())
            objIt = s_openGlCbs.erase(objIt);
        else
            objIt++;

        i++;
    }
}

// process hid events, received in the WinBase HID callbacks
// this function passes down HID events to the UINodes and by this assures,
// that all UINode HID functions are called whithin the GL-context of their
// parent window
void WindowBase::procHid() {
    if (s_hidEvents.empty()) return;

    std::unique_lock<mutex> lock(s_procHidMtx);

    // key events have to be treated differently. mouse events can always be
    // overwritten with the latest event but there may be multiple different key
    // events so closely after each other that they could get lost
    for (auto &it : s_keyEvents) {
        if (!it.second.empty())
            for (auto &kit : it.second) kit();
        it.second.clear();
    }

    for (auto &it : s_hidEvents) {
        if (it.second) it.second();
        it.second = nullptr;
    }
}

// process events changing the Window size or position received in the WinBase
// callbacks this function passes down events to the UINodes and by this
// assures, that all UINode HID functions are called whithin the GL-context of
// their parent window
void WindowBase::procChangeWin() {
    if (s_changeWinEvents.empty()) return;

    std::unique_lock<mutex> lock(s_changeWinMtx);
    for (auto &it : s_changeWinEvents) {
        if (it.second) it.second();
        it.second = nullptr;
    }
}

}  // namespace ara
