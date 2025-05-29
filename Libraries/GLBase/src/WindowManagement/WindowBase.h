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

#include "Shaders/ShaderCollector.h"
#include "GlbCommon/GlbCommon.h"

namespace ara {

class WindowBase {
public:
    WindowBase() = default;
    virtual ~WindowBase() = default;

    virtual void init(const glm::ivec2& pos, const glm::ivec2& size);
    virtual bool draw(double time, double dt, int ctxNr) = 0;
    virtual void procHid();
    virtual void procChangeWin();

    // direct HID input (non-sync)
    virtual void osKeyDown(int keyNum, bool shiftPressed, bool ctrlPressed, bool altPressed) {
        std::unique_lock<std::mutex> lock(s_procHidMtx);
        s_keyEvents[hidEvent::KeyDown].emplace_back([this, keyNum, shiftPressed, ctrlPressed, altPressed]() {
            onKeyDown(keyNum, shiftPressed, ctrlPressed, altPressed);
        });
    }

    virtual void osKeyUp(int keyNum, bool shiftPressed, bool ctrlPressed, bool altPressed) {
        std::unique_lock<std::mutex> lock(s_procHidMtx);
        s_keyEvents[hidEvent::KeyUp].emplace_back([this, keyNum, shiftPressed, ctrlPressed, altPressed]() {
            onKeyUp(keyNum, shiftPressed, ctrlPressed, altPressed);
        });
    }

    virtual void osChar(unsigned int codepoint) {
        std::unique_lock<std::mutex> lock(s_procHidMtx);
        s_hidEvents[hidEvent::onChar] = [this, codepoint]() { onChar(codepoint); };
    }

    virtual void osMouseDownLeft(float xPos, float yPos, bool shiftPressed, bool ctrlPressed, bool altPressed) {
        std::unique_lock<std::mutex> lock(s_procHidMtx);
        s_hidEvents[hidEvent::MouseDownLeft] = [this, xPos, yPos, shiftPressed, ctrlPressed, altPressed]() {
            onMouseDownLeft(xPos, yPos, shiftPressed, ctrlPressed, altPressed);
        };
    }

    virtual void osMouseDownLeftNoDrag(float xPos, float yPos) {
        std::unique_lock<std::mutex> lock(s_procHidMtx);
        s_hidEvents[hidEvent::MouseDownLeftNoDrag] = [this, xPos, yPos]() { onMouseDownLeftNoDrag(xPos, yPos); };
    }

    virtual void osMouseUpLeft() {
        std::unique_lock<std::mutex> lock(s_procHidMtx);
        s_hidEvents[hidEvent::MouseUpLeft] = [this]() { onMouseUpLeft(); };
    }

    virtual void osMouseDownRight(float xPos, float yPos, bool shiftPressed, bool ctrlPressed, bool altPressed) {
        std::unique_lock<std::mutex> lock(s_procHidMtx);
        s_hidEvents[hidEvent::MouseDownRight] = [this, xPos, yPos, shiftPressed, ctrlPressed, altPressed]() {
            onMouseDownRight(xPos, yPos, shiftPressed, ctrlPressed, altPressed);
        };
    }

    virtual void osMouseUpRight() {
        std::unique_lock<std::mutex> lock(s_procHidMtx);
        s_hidEvents[hidEvent::MouseUpRight] = [this]() { onMouseUpRight(); };
    }

    virtual void osMouseMove(float xPos, float yPos, ushort mode) {
        std::unique_lock<std::mutex> lock(s_procHidMtx);
        s_hidEvents[hidEvent::MouseMove] = [this, xPos, yPos, mode]() { onMouseMove(xPos, yPos, mode); };
    }

    virtual void osWheel(float deg) {
        std::unique_lock<std::mutex> lock(s_procHidMtx);
        s_hidEvents[hidEvent::MouseWheel] = [this, deg]() { onWheel(deg); };
    }

    virtual void osSetViewport(int x, int y, int width, int height) {
        if (!width || !height) {
            return;
        }
        std::unique_lock<std::mutex> lock(s_procHidMtx);
        s_hidEvents[hidEvent::SetViewport] = [this, x, y, width, height]() { onSetViewport(x, y, width, height); };
    }

    virtual void osMinimize(std::function<void()> f) {
        std::unique_lock<std::mutex> lock(s_changeWinMtx);
        s_changeWinEvents[changeWinEvent::Minimize] = std::move(f);
    }

    virtual void osMaximize(std::function<void()> f) {
        std::unique_lock<std::mutex> lock(s_changeWinMtx);
        s_changeWinEvents[changeWinEvent::Maximize] = std::move(f);
    }

    virtual void osSetWinPos(std::function<void()> f) {
        std::unique_lock<std::mutex> lock(s_changeWinMtx);
        s_changeWinEvents[changeWinEvent::SetWinPos] = std::move(f);
    }

    virtual void osResize(std::function<void()> f) {
        std::unique_lock<std::mutex> lock(s_changeWinMtx);
        s_changeWinEvents[changeWinEvent::OnResize] = std::move(f);
    }

    // gl synced calls
    virtual void onKeyDown(int keyNum, bool shiftPressed, bool ctrlPressed, bool altPressed) = 0;
    virtual void onKeyUp(int keyNum, bool shiftPressed, bool ctrlPressed, bool altPressed) = 0;
    virtual void onChar(unsigned int codepoint) = 0;
    virtual void onMouseDownLeft(float xPos, float yPos, bool shiftPressed, bool ctrlPressed, bool altPressed) = 0;
    virtual void onMouseDownLeftNoDrag(float xPos, float yPos) = 0;
    virtual void onMouseUpLeft() = 0;
    virtual void onMouseDownRight(float xPos, float yPos, bool shiftPressed, bool ctrlPressed, bool altPressed) = 0;
    virtual void onMouseUpRight() = 0;
    virtual void onMouseMove(float xPos, float yPos, ushort mode) = 0;
    virtual void onWheel(float deg) {};
    virtual void onResizeDone() = 0;
    virtual void onSetViewport(int x, int y, int width, int height) = 0;
    virtual void iterateGlCallbacks();

    bool        getDrawMtxIsLocked() const { return s_drawMtxLocked; }
    std::mutex *getDrawMtx() { return &s_drawMtx; }
    glm::mat4  *getOrthoMat() { return &s_orthoMat; }
    std::mutex *getHidMutex() { return &s_procHidMtx; }

    virtual void addGlCb(void *cbName, const std::string &fName, std::function<bool()> func);
    virtual bool hasCb(void *cbName, const std::string &fName);
    virtual void eraseGlCb(void *cbName, const std::string &fName);
    virtual void eraseGlCb(void *cbName);
    virtual void clearGlCbQueue();

    virtual void removeGLResources() {}

public:
    bool              s_inited        = false;
    bool              s_drawMtxLocked = false;
    std::atomic<bool> s_processingHID = false;
    std::atomic<bool> s_isDrawing     = false;
    ShaderCollector   s_shCol;

protected:
    glm::vec4  s_viewPort{0.f};  /// in hw pixels
    glm::ivec2 s_mousePos{0};
    glm::ivec2 s_mouse_down_right_init_pos{0};
    bool       s_debug = false;

    std::mutex                                                                         s_procHidMtx;
    std::mutex                                                                         s_drawMtx;
    std::mutex                                                                         s_changeWinMtx;
    std::unordered_map<hidEvent, std::function<void()>>                                s_hidEvents;
    std::unordered_map<changeWinEvent, std::function<void()>>                          s_changeWinEvents;
    std::unordered_map<hidEvent, std::vector<std::function<void()>>>                   s_keyEvents;
    std::unordered_map<void *, std::unordered_map<std::string, std::function<bool()>>> s_openGlCbs;

    glm::vec4            s_windowViewport{0.f};  /// in virtual pixels
    glm::mat4            s_orthoMat = glm::mat4(1.f);
    std::array<GLint, 4> s_lastViewport{0};
    float                s_devicePixelRatio = 1.f;

    std::map<winProcStep, ProcStep> m_procSteps;
};

}  // namespace ara
