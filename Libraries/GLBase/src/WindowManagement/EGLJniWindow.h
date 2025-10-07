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

#if defined(ARA_USE_EGL) && !defined(ARA_USE_GLFW)

#include <WindowManagement/GLWindowBase.h>
#include <WindowManagement/esutil/esUtil.h>

namespace ara {

class GLBase;

class EGLJniWindow : public GLWindowBase {
public:
    EGLJniWindow() : GLWindowBase() {}
    ~EGLJniWindow() override = default;

    bool create(const glWinPar& gp) override { return init(gp); }
    int  init(const glWinPar& gp);

    /**
     * @param f the drawing function which will be execute or every iteration
     * @param eventBased choose wheter the loop should run freely or stop and
     * wait for the m_iterate signal
     * @param terminateGLFW shall GLFW be terminated when the loop exits? in
     * case of multiple window it probably shouldn't
     */
    void runLoop(const std::function<bool(double, double, int)>& f, bool eventBased = false, bool destroyWinOnExit = true);
    void startDrawThread(const std::function<bool(double, double, int)>& f);
    void stopDrawThread();
    void draw();
    void procEventQueue();

    void            open() override {}
    void            close() override {}
    void            swap() override;
    void            hide() {}
    void            makeCurrent() override;
    void            setVSync(bool val) override {}
    void            focus() {}
    void            destroy() override { destroy(false); }
    virtual void    destroy(bool val) {}
    void            minimize() override {  }
    void            restore() override {  }
    bool            isMinimized() { return false; }
    void            checkSize();
    void            onWindowSize(int width, int height) override;
    void            resize(GLsizei width, GLsizei height) override {}
    void*           getNativeCtx() override { return nullptr; }

    EGLint GetContextRenderableType(EGLDisplay eglDisplay);

    void*           getCtx() { return m_esContext.eglContext; }

    unsigned int    getMonitorWidth() { return 0; }
    unsigned int    getMonitorHeight() { return 0; }
    int             getFocus() { return 1; }
    glm::ivec2      getLastMousePos() { return {}; }
    glm::ivec4&     getWorkArea() override { return m_fakeWorkArea; }

    void setSize(int, int) {}
    void setPosition(int, int) {}

    // utility methods for unified window handling (EGLWindow -> GLWindow)
    void setKeyCallback(const std::function<void(EGLContext, int, int, int, int)>& f) {}
    void setCharCallback(const std::function<void(EGLContext, unsigned int)>& f) {}
    void setMouseButtonCallback(const std::function<void(EGLContext, int, int, int)>& f) {}
    void setCursorPosCallback(const std::function<void(EGLContext, double, double)>& f) {}
    void setWindowSizeCallback(const std::function<void(EGLContext, int, int)>& f) {}
    void setWindowCloseCallback(const std::function<void(EGLContext)>& f) {}
    void setWindowMaximizeCallback(const std::function<void(EGLContext, int)>& f) {}
    void setWindowIconifyCallback(const std::function<void(EGLContext, int)>& f) {}
    void setWindowFocusCallback(const std::function<void(EGLContext, int)>& f) {}
    void setWindowPosCallback(const std::function<void(EGLContext, int, int)>& f) {}
    void setScrollCallback(const std::function<void(EGLContext, double, double)>& f) {}
    void setWindowRefreshCallback(const std::function<void(EGLContext)>& f) {}
    void setOnCloseCb(const std::function<void()>& f) { m_onCloseCb = f; }

    static void waitEvents();
    static void pollEvents();
    static void addEventToQueue(const std::function<bool()>& f) { m_eventQueue.emplace_back(f); }
    static void postEmptyEvent() {}
    static void setErrorCallback(const std::function<void(int, const char*)>& f) {}
    static void initLibrary() {}
    static void terminateLibrary() {}
    static void focusWin(EGLContext ctx) {}
    static void makeNoneCurrent() {
    }

    static void error_callback(int error, const char* description) {
        LOGE << "EGL ERROR: " << description;
        fputs(description, stderr);
    }

    static EGLDisplay                    m_display;  // EGL display connection

protected:
    glm::ivec4  m_initialClearColor{};  ///< 0-255 window alpha value
    ESContext   m_esContext{};
    orientation m_orientation = orientation::default_ori;
    glm::ivec4  m_fakeWorkArea{};
    std::chrono::system_clock::time_point m_lastTime;
    std::chrono::system_clock::time_point m_startTime;

    float                 m_androidDefaultDpi = 160.f;
    GLBase*               m_glbase            = nullptr;
    std::function<void()> m_onCloseCb;

    static inline std::vector<std::function<bool()>> m_eventQueue;
    static inline std::mutex m_eventQueueMtx;
};

}  // namespace ara

#endif
