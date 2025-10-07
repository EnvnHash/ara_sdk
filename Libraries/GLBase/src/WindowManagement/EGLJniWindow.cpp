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

#if defined(ARA_USE_EGL) && !defined(ARA_USE_GLFW)

#include "WindowManagement/EGLJniWindow.h"
#include "GLBase.h"

using namespace std;
using namespace glm;
using namespace std::chrono;

namespace ara {

int EGLJniWindow::init(const glWinPar& gp) {
    m_esContext.eglContext = gp.extWinHandle;

    // Initialize GL state.
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_startTime     = system_clock::now();
    m_inited        = true;
    m_initSema.notify();
    m_isOpen        = true;

    return GL_TRUE;
}

void EGLJniWindow::runLoop(const std::function<bool(double, double, int)>& f, bool eventBased, bool destroyWinOnExit) {
    m_drawFunc       = f;
    m_run            = true;
    m_eventBasedLoop = eventBased;

    // Loop until the user closes the window
    while (m_run) {
        if (eventBased && m_initSignaled && !m_forceRedraw) {
            m_iterate.wait(0);
        }

        m_forceRedraw = false;
        pollEvents();
        draw();
        execGlCb();

        if (eventBased && !m_initSignaled) {
            m_initSignaled = true;
            m_glInitedSema.notify();
        }
    }
    m_run = false;

    if (destroyWinOnExit) {
        destroy();
    }

    m_exitSignal.notify();
}

void EGLJniWindow::pollEvents() {
    /*
    if (!m_eventQueue.empty()) {
        unique_lock l(m_eventQueueMtx);

        bool funcSuccess = false;
        for (auto it = m_eventQueue.begin(); it != m_eventQueue.end();) {
            funcSuccess = (*it)();
            if (funcSuccess) {
                it = m_eventQueue.erase(it);
            } else {
                ++it;
            }
        }
    }*/
}

void EGLJniWindow::startDrawThread(const std::function<bool(double, double, int)>& f) {
    if (!isRunning()) {
        m_drawThread = std::thread(&EGLJniWindow::runLoop, this, f, true, true);
        m_drawThread.detach();
    }
}

void EGLJniWindow::stopDrawThread() {
    if (m_run) {
        m_run = false;
        close();
        m_iterate.notify();
        m_exitSignal.wait(0);
    }
}

void EGLJniWindow::draw() {
    m_lastTime   = system_clock::now();
    auto actDifF = std::chrono::duration<double, std::milli>(m_lastTime - m_startTime).count();
    glViewport(0, 0, m_realSize.x, m_realSize.y);

    if (m_drawFunc(actDifF * 1000.0, 0, 0)) {
        swap();
    }
}

void EGLJniWindow::swap() {
    if (EGL_FALSE == eglSwapBuffers(m_esContext.eglDisplay, m_esContext.eglSurface)) {
        LOG << "NativeEngine: eglSwapBuffers failed, EGL error " << eglGetError();
    }
}

void EGLJniWindow::makeCurrent() {
    if (!eglMakeCurrent(m_esContext.eglDisplay, m_esContext.eglSurface, m_esContext.eglSurface, m_esContext.eglContext)) {
        LOGE << "EGLWindow::makeCurrent() failed";
    }
}

void EGLJniWindow::onWindowSize(int width, int height) {
    m_virtSize.x  = width;
    m_virtSize.y = height;
    if (m_windowSizeCb) {
        m_windowSizeCb(width, height);
    }
}

void EGLJniWindow::checkSize() {
    int w, h;

    eglQuerySurface(m_esContext.eglDisplay, m_esContext.eglSurface, EGL_WIDTH, &w);
    eglQuerySurface(m_esContext.eglDisplay, m_esContext.eglSurface, EGL_HEIGHT, &h);
    m_virtSize.x = static_cast<int32_t>(std::round(static_cast<float>(w) / m_contentScale.x));
    m_virtSize.y = static_cast<int32_t>(std::round(static_cast<float>(h) / m_contentScale.y));
    m_realSize.x = static_cast<int32_t>(w);
    m_realSize.y = static_cast<int32_t>(h);

    if (m_windowSizeCb) {
        m_windowSizeCb(static_cast<int32_t>(m_virtSize.x), static_cast<int32_t>(m_virtSize.y));
    }
}

void EGLJniWindow::waitEvents() {
#if defined(__linux__) && !defined(__ANDROID__)
    if (m_osWin) {
        auto osWin = static_cast<X11Window*>(m_osWin.get());
        osWin->waitEvents();
    }
#endif
}

//    Check whether EGL_KHR_create_context extension is supported.  If so,
//    return EGL_OPENGL_ES3_BIT_KHR instead of EGL_OPENGL_ES2_BIT
EGLint EGLJniWindow::GetContextRenderableType(EGLDisplay eglDisplay) {
#ifdef EGL_KHR_create_context
    const char* extensions = eglQueryString(eglDisplay, EGL_EXTENSIONS);

    // check whether EGL_KHR_create_context is in the extension string
    if (extensions && strstr(extensions, "EGL_KHR_create_context")) {
        return EGL_OPENGL_ES3_BIT_KHR;
    }
#endif
    return EGL_OPENGL_ES2_BIT;
}

}  // namespace ara

#endif
