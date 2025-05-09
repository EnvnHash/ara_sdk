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

#ifdef __ANDROID__
#include <android/native_window.h>
#include <android/looper.h>
#elif __linux__
#include <X11Window.h>
#endif

namespace ara {

class GLBase;

class EGLWindow : public GLWindowBase {
public:
    EGLWindow() : GLWindowBase() {}
    virtual ~EGLWindow() = default;

    bool create(const glWinPar& gp) override { return init(gp); }
    int  init(const glWinPar& gp);

    /**
     * @param f the drawing function which will be execute or every iteration
     * @param eventBased choose wheter the loop should run freely or stop and
     * wait for the m_iterate signal
     * @param terminateGLFW shall GLFW be terminated when the loop exits? in
     * case of multiple window it probably shouldn't
     */
    void runLoop(std::function<bool(double, double, int)> f, bool eventBased = false, bool destroyWinOnExit = true);
    void startDrawThread(std::function<bool(double, double, int)> f);
    void stopDrawThread();
    void draw();

    void            open() override;
    void            close() override;
    void            swap() override;
    void            hide();
    void            makeCurrent() override;
    void            setVSync(bool val) override;
    void            focus() {}
    void            destroy() override { destroy(false); }
    virtual void    destroy(bool val);
    void            minimize() override { if (m_osWin) m_osWin->minimize(); }
    void            restore() override { if (m_osWin) m_osWin->restore(); }
    bool            isMinimized() { return false; }
    void            checkSize();
    void            onWindowSize(int width, int height) override;
    void            resize(GLsizei width, GLsizei height) override;
    void*           getNativeCtx() override;

#ifndef __APPLE__
    EGLint GetContextRenderableType(EGLDisplay eglDisplay);
#endif

    EGLDisplay      getEglDisplay() const { return m_display; }
    EGLSurface      getEglSurface() const { return m_surface; }
    void*           getCtx() { return (void*)m_esContext.eglContext; }
    void*           getWin() override { return &m_esContext.eglNativeWindow; }
    void*           getDisp() override { return &m_esContext.eglNativeDisplay; }
    EGLConfig       getEglConfig() const { return m_eglConfig; }
    EGLint          getEglVersionMajor() const { return m_eglVersionMajor; }
    EGLint          getEglVersionMinor() const { return m_eglVersionMinor; }
    unsigned int    getMonitorWidth() { return 0; }
    unsigned int    getMonitorHeight() { return 0; }
    int             getFocus() { return 1; }
    glm::ivec2      getLastMousePos() { return m_osWin ? m_osWin->getLastMousePos() : glm::ivec2 {}; }
    int*            getWorkArea() { return m_osWin ? m_osWin->getWorkArea() : nullptr; }

    void setSize(int m_widthVirt, int m_heightVirt) {}
    void setPosition(int posx, int posy) {}

#ifdef __ANDROID__
    void setALooper(void* looper) { m_looper = reinterpret_cast<ALooper*>(looper); }
    ALooper* getALooper() { return m_looper; }
    void forceRedraw() override { m_forceRedraw = true; if(m_looper) ALooper_wake(m_looper); }
#endif

    // utility methods for unified window handling (EGLWindow -> GLWindow)
    void setKeyCallback(std::function<void(EGLContext, int, int, int, int)> f) {}
    void setCharCallback(std::function<void(EGLContext, unsigned int)> f) {}
    void setMouseButtonCallback(std::function<void(EGLContext, int, int, int)> f) {}
    void setCursorPosCallback(std::function<void(EGLContext, double, double)> f) {}
    void setWindowSizeCallback(std::function<void(EGLContext, int, int)> f) {}
    void setWindowCloseCallback(std::function<void(EGLContext)> f) {}
    void setWindowMaximizeCallback(std::function<void(EGLContext, int)> f) {}
    void setWindowIconifyCallback(std::function<void(EGLContext, int)> f) {}
    void setWindowFocusCallback(std::function<void(EGLContext, int)> f) {}
    void setWindowPosCallback(std::function<void(EGLContext, int, int)> f) {}
    void setScrollCallback(std::function<void(EGLContext, double, double)> f) {}
    void setWindowRefreshCallback(std::function<void(EGLContext)> f) {}
    void setOnCloseCb(std::function<void()> f) { m_onCloseCb = std::move(f); }

    static void waitEvents();
    static void pollEvents() {}
    static void postEmptyEvent();
    static void setErrorCallback(std::function<void(int, const char*)> f) {}
    static void initLibrary() {}
    static void terminateLibrary() {}
    static void focusWin(EGLContext ctx) {}
    static void makeNoneCurrent() {
        if (m_display && !eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT))
            LOGE << "EGLWindow, could not make none context current";
    }

    static void error_callback(int error, const char* description) {
        printf("EGL ERROR: %s \n", description);
        fputs(description, stderr);
    }

    static std::unique_ptr<GLWindowBase> m_osWin;
    static EGLDisplay                    m_display;  // EGL display connection

protected:
    EGLint     m_eglWindowOpacity;   ///< 0-255 window alpha value
    glm::ivec4 m_initialClearColor;  ///< 0-255 window alpha value

    ESContext m_esContext;

    EGLSurface  m_surface         = nullptr;
    EGLConfig   m_eglConfig       = nullptr;
    EGLint      m_eglVersionMajor = -1;
    EGLint      m_eglVersionMinor = -1;
    EGLint      m_format;
    orientation m_orientation = orientation::default_ori;
    EGLint      m_num_config;

    void* m_userData = nullptr;

    std::chrono::system_clock::time_point m_lastTime;
    std::chrono::system_clock::time_point m_startTime;

    float                 m_androidDefaultDpi = 160.f;
    GLBase*               m_glbase            = nullptr;
    std::function<void()> m_onCloseCb;

#ifdef __ANDROID__
    ALooper* m_looper = nullptr;
#endif
};

}  // namespace ara

#endif
