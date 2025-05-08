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

#include "EGLWindow.h"
#include "GLBase.h"

using namespace std;
using namespace glm;
using namespace std::chrono;

namespace ara {

std::unique_ptr<GLWindowBase> EGLWindow::m_osWin;        // EGL display connection
EGLDisplay                    EGLWindow::m_display = nullptr;  // EGL display connection

int EGLWindow::init(const glWinPar& gp) {
    m_widthVirt   = gp.size.x;
    m_heightVirt  = gp.size.y;
    m_widthReal   = gp.size.x;
    m_heightReal  = gp.size.y;
    m_orientation = orientation::default_ori;
    m_glbase      = static_cast<GLBase*>(gp.glbase);

#ifndef __APPLE__
    EGLint majorVersion = 0;
    EGLint minorVersion = 0;

#ifdef __ANDROID__
    m_esContext.eglNativeWindow = static_cast<ANativeWindow*>(gp.extWinHandle);

    // For Android, get the width/height from the window rather than what the application requested.
    m_esContext.width  = ANativeWindow_getWidth(m_esContext.eglNativeWindow);
    m_esContext.height = ANativeWindow_getHeight(m_esContext.eglNativeWindow);

    // calculate a scaling factor for the devices dpi setting (is set in UIApplication::android_handle_cmd)
    m_contentScale = vec2{m_glbase->g_androidDensity, m_glbase->g_androidDensity};
#else
    m_esContext.width  = m_widthVirt;
    m_esContext.height = m_heightVirt;
#endif

#ifdef __ANDROID__
    if ((m_esContext.eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        LOGE << "eglGetDisplay() returned error " << eglGetError();
        return false;
    }
#elif __linux__
    m_osWin = make_unique<X11Window>();
    m_osWin->create(gp);

    m_esContext.eglNativeWindow  = *static_cast<EGLNativeWindowType*>(m_osWin->getWin());
    m_esContext.eglNativeDisplay = *static_cast<EGLNativeDisplayType*>(m_osWin->getDisp());
    m_esContext.eglDisplay       = eglGetDisplay(m_esContext.eglNativeDisplay);
#endif
    m_display = m_esContext.eglDisplay;

    // Initialize EGL
    if (!eglInitialize(m_esContext.eglDisplay, &majorVersion, &minorVersion)) {
        return false;
    }
    LOG << "USING EGL " << majorVersion << "." << minorVersion;

    EGLConfig config = nullptr;
    auto      flags  = ES_WINDOW_RGB | ES_WINDOW_DEPTH | ES_WINDOW_STENCIL;

    {
        EGLint numConfigs   = 0;
        EGLint attribList[] = {EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE,
                               (flags & ES_WINDOW_ALPHA) ? 8 : EGL_DONT_CARE, EGL_DEPTH_SIZE,
                               (flags & ES_WINDOW_DEPTH) ? 8 : EGL_DONT_CARE, EGL_STENCIL_SIZE,
                               (flags & ES_WINDOW_STENCIL) ? 8 : EGL_DONT_CARE, EGL_SAMPLE_BUFFERS,
                               (flags & ES_WINDOW_MULTISAMPLE) ? 1 : 0,
                               // if EGL_KHR_create_context extension is supported, then we will
                               // use EGL_OPENGL_ES3_BIT_KHR instead of EGL_OPENGL_ES2_BIT in the
                               // attribute list
                               EGL_RENDERABLE_TYPE, GetContextRenderableType(m_esContext.eglDisplay), EGL_NONE};

        // Choose config
        eglChooseConfig(m_esContext.eglDisplay, attribList, nullptr, 0, &numConfigs);
        std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
        assert(supportedConfigs);
        eglChooseConfig(m_esContext.eglDisplay, attribList, supportedConfigs.get(), numConfigs, &numConfigs);
        assert(numConfigs);

        auto i = 0;
        for (; i < numConfigs; i++) {
            auto&  cfg = supportedConfigs[i];
            EGLint r, m_g, b, d;
            if (eglGetConfigAttrib(m_esContext.eglDisplay, cfg, EGL_RED_SIZE, &r) &&
                eglGetConfigAttrib(m_esContext.eglDisplay, cfg, EGL_GREEN_SIZE, &m_g) &&
                eglGetConfigAttrib(m_esContext.eglDisplay, cfg, EGL_BLUE_SIZE, &b) &&
                eglGetConfigAttrib(m_esContext.eglDisplay, cfg, EGL_DEPTH_SIZE, &d) && r == 8 && m_g == 8 && b == 8 &&
                d == 0) {
                config = supportedConfigs[i];
                break;
            }
        }

        if (i == numConfigs) {
            config = supportedConfigs[0];
        }

        if (config == nullptr) {
            LOGE << "Unable to initialize EGLConfig";
            return -1;
        }
    }

#ifdef __ANDROID__
    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    {
        EGLint format = 0;
        eglGetConfigAttrib(m_esContext.eglDisplay, config, EGL_NATIVE_VISUAL_ID, &format);
    }
#endif

    // Create a surface
    m_esContext.eglSurface = eglCreateWindowSurface(m_esContext.eglDisplay, config, m_esContext.eglNativeWindow, nullptr);
    if (m_esContext.eglSurface == EGL_NO_SURFACE) {
        LOGE << "EGLWindow Error eglCreateWindowSurface failed";
        return GL_FALSE;
    }

    // Create a GL context
    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    m_esContext.eglContext  = eglCreateContext(m_esContext.eglDisplay, config, gp.shareCont, contextAttribs);
    if (m_esContext.eglContext == EGL_NO_CONTEXT) {
        LOGE << "EGLWindow Error eglCreateContext failed";
        return GL_FALSE;
    }

    // Make the context current
    if (!eglMakeCurrent(m_esContext.eglDisplay, m_esContext.eglSurface, m_esContext.eglSurface,
                        m_esContext.eglContext)) {
        LOGE << "EGLWindow Error could not make context current";
        return GL_FALSE;
    }

    int w, h;
    eglQuerySurface(m_esContext.eglDisplay, m_esContext.eglSurface, EGL_WIDTH, &w);
    eglQuerySurface(m_esContext.eglDisplay, m_esContext.eglSurface, EGL_HEIGHT, &h);
    m_widthVirt  = static_cast<uint32_t>(std::round(static_cast<float>(w) / m_contentScale.x));
    m_heightVirt = static_cast<uint32_t>(std::round(static_cast<float>(h) / m_contentScale.y));
    m_widthReal  = w;
    m_heightReal = h;
    LOG << " EGLWindow virtual size: " << m_widthVirt << "x" << m_heightVirt << " hw size: " << m_widthReal << "x"
        << m_heightReal;

    // Check openGL on the system
    auto opengl_info = {GL_VENDOR, GL_RENDERER, GL_VERSION, GL_EXTENSIONS};
    for (auto name : opengl_info) {
        auto info = glGetString(name);
        LOG << "OpenGL Info: " << info;
    }

    // Initialize GL state.
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    swap();

#endif  // #ifndef __APPLE__

    m_startTime = system_clock::now();
    m_inited    = true;
    m_initSema.notify();
    m_run          = true;
    m_initSignaled = true;

    return GL_TRUE;
}

void EGLWindow::runLoop(std::function<bool(double, double, int)> f, bool eventBased, bool destroyWinOnExit) {
    bool unlock      = false;
    m_drawFunc       = f;
    m_run            = true;
    m_eventBasedLoop = eventBased;

    if (!eglMakeCurrent(m_esContext.eglDisplay, m_esContext.eglSurface, m_esContext.eglSurface, m_esContext.eglContext)) {
        LOGE << "EGLWindow::runLoop could not make context current";
    }

    // Loop until the user closes the window
    while (m_run) {
        if (eventBased && m_initSignaled && !m_forceRedraw) {
            m_iterate.wait(0);
        }

        m_forceRedraw = false;
        draw();

        if (m_run && m_glCb) {
            m_glCb();
            m_glCb = nullptr;
        }
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

void EGLWindow::startDrawThread(std::function<bool(double, double, int)> f) {
    if (!isRunning()) {
        m_drawThread = std::thread(&EGLWindow::runLoop, this, f, true, false);
        m_drawThread.detach();
    }
}

void EGLWindow::stopDrawThread() {
    if (m_run) {
        m_run = false;
        close();
        m_iterate.notify();
        m_exitSignal.wait(0);
    }
}

void EGLWindow::draw() {
    m_lastTime   = system_clock::now();
    auto actDifF = std::chrono::duration<double, std::milli>(m_lastTime - m_startTime).count();
    glViewport(0, 0, m_widthReal, m_heightReal);

    if (m_drawFunc(actDifF * 1000.0, 0, 0)) {
        swap();
    }
}

void EGLWindow::open() {}

void EGLWindow::close() {}

void EGLWindow::swap() {
    if (EGL_FALSE == eglSwapBuffers(m_esContext.eglDisplay, m_esContext.eglSurface)) {
        LOG << "NativeEngine: eglSwapBuffers failed, EGL error " << eglGetError();
    }
}

void EGLWindow::hide() {}

void EGLWindow::makeCurrent() {
    if (!eglMakeCurrent(m_esContext.eglDisplay, m_esContext.eglSurface, m_esContext.eglSurface, m_esContext.eglContext)) {
        LOGE << "EGLWindow::makeCurrent() failed";
    }
}

void EGLWindow::setVSync(bool val) {}

void EGLWindow::destroy(bool val) {
    eglMakeCurrent(m_esContext.eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(m_esContext.eglDisplay, m_esContext.eglContext);
    eglDestroySurface(m_esContext.eglDisplay, m_esContext.eglSurface);
    eglTerminate(m_esContext.eglDisplay);

    m_esContext.eglDisplay = EGL_NO_DISPLAY;
    m_esContext.eglSurface = EGL_NO_SURFACE;
    m_esContext.eglContext = EGL_NO_CONTEXT;
}

void EGLWindow::onWindowSize(int width, int height) {
    m_widthVirt  = width;
    m_heightVirt = height;
    if (m_windowSizeCb) {
        m_windowSizeCb(width, height);
    }
}

void EGLWindow::resize(GLsizei width, GLsizei height) {}

void EGLWindow::checkSize() {
    int w, h;

    eglQuerySurface(m_esContext.eglDisplay, m_esContext.eglSurface, EGL_WIDTH, &w);
    eglQuerySurface(m_esContext.eglDisplay, m_esContext.eglSurface, EGL_HEIGHT, &h);
    m_widthVirt  = static_cast<uint32_t>(std::round(static_cast<float>(w) / m_contentScale.x));
    m_heightVirt = static_cast<uint32_t>(std::round(static_cast<float>(h) / m_contentScale.y));
    m_widthReal  = static_cast<uint32_t>(w);
    m_heightReal = static_cast<uint32_t>(h);

    if (m_windowSizeCb) {
        m_windowSizeCb(static_cast<int32_t>(m_widthVirt), static_cast<int32_t>(m_heightVirt));
    }
}

void* EGLWindow::getNativeCtx() {
    return nullptr;
}

void EGLWindow::waitEvents() {
#if defined(__linux__) && !defined(__ANDROID__)
    if (m_osWin) {
        auto osWin = static_cast<X11Window*>(m_osWin.get());
        osWin->waitEvents();
    }
#endif
}

void EGLWindow::postEmptyEvent() {
#if defined(__linux__) && !defined(__ANDROID__)
    auto display = (Display*)static_cast<X11Window*>(m_osWin.get())->getDisp();
    auto window  = (Window) static_cast<X11Window*>(m_osWin.get())->getWin();

    XEvent event           = {ClientMessage};
    event.xclient.window   = window;
    event.xclient.m_format = 32;  // Data is 32-bit longs

    Atom nullMsg               = XInternAtom(display, "NULL", False);
    event.xclient.message_type = nullMsg;

    XSendEvent(display, window, False, 0, &event);
    XFlush(display);
#endif
}

#ifndef __APPLE__
//    Check whether EGL_KHR_create_context extension is supported.  If so,
//    return EGL_OPENGL_ES3_BIT_KHR instead of EGL_OPENGL_ES2_BIT
EGLint EGLWindow::GetContextRenderableType(EGLDisplay eglDisplay) {
#ifdef EGL_KHR_create_context
    const char* extensions = eglQueryString(eglDisplay, EGL_EXTENSIONS);

    // check whether EGL_KHR_create_context is in the extension string
    if (extensions && strstr(extensions, "EGL_KHR_create_context")) {
        return EGL_OPENGL_ES3_BIT_KHR;
    }
#endif
    return EGL_OPENGL_ES2_BIT;
}
#endif

}  // namespace ara

#endif
