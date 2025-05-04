#pragma once

#ifdef ARA_USE_EGL
#include <GlbCommon/GlbCommon.h>

namespace ara {
/// esCreateWindow flag - RGB color m_buffer
#define ES_WINDOW_RGB 0
/// esCreateWindow flag - ALPHA color m_buffer
#define ES_WINDOW_ALPHA 1
/// esCreateWindow flag - depth m_buffer
#define ES_WINDOW_DEPTH 2
/// esCreateWindow flag - stencil m_buffer
#define ES_WINDOW_STENCIL 4
/// esCreateWindow flat - multi-sample m_buffer
#define ES_WINDOW_MULTISAMPLE 8

class ESContext {
public:
    void *platformData = nullptr;
    void *m_userData   = nullptr;
    GLint width        = 0;  /// Window width
    GLint height       = 0;  /// Window height

    void registerDrawFunc(std::function<void(ESContext *)> f) { m_drawFunc = f; }
    void registerShutdownFunc(std::function<void(ESContext *)> f) { m_shutdownFunc = f; }
    void registerUpdateFunc(std::function<void(ESContext *, float)> f) { m_updateFunc = f; }
    void registerKeyFunc(std::function<void(ESContext *, unsigned char, int, int)> f) { m_keyFunc = f; }

#ifndef __APPLE__
    EGLNativeDisplayType eglNativeDisplay = 0;        /// Display handle
    EGLNativeWindowType  eglNativeWindow  = 0;        /// Window handle
    EGLDisplay           eglDisplay       = nullptr;  /// EGL display
    EGLContext           eglContext       = nullptr;  /// EGL context
    EGLSurface           eglSurface       = nullptr;  /// EGL surface
#endif

    /// Callbacks
    std::function<void(ESContext *)>                          m_drawFunc;
    std::function<void(ESContext *)>                          m_shutdownFunc;
    std::function<void(ESContext *, unsigned char, int, int)> m_keyFunc;
    std::function<void(ESContext *, float)>                   m_updateFunc;
};

/// \brief Create a window with the specified parameters
/// \param esContext Application context
/// \param title Name for title bar of window
/// \param width Width in pixels of window to create
/// \param height Height in pixels of window to create
/// \param flags Bitfield for the window creation flags
///         ES_WINDOW_RGB     - specifies that the color m_buffer should have
///         R,G,B channels ES_WINDOW_ALPHA   - specifies that the color m_buffer
///         should have alpha ES_WINDOW_DEPTH   - specifies that a depth
///         m_buffer should be created ES_WINDOW_STENCIL - specifies that a
///         stencil m_buffer should be created ES_WINDOW_MULTISAMPLE - specifies
///         that a multi-sample m_buffer should be created
/// \return GL_TRUE if window creation is succesful, GL_FALSE otherwise
GLboolean esCreateWindow(ESContext *esContext, const char *title, GLint width, GLint height, GLuint flags);

/// \brief Log a message to the debug output for the platform
/// \param formatStr Format string for error log.
void       esLogMessage(const char *formatStr, ...);
EGLBoolean WinCreate(ESContext *esContext, const char *title);
GLboolean  userInterrupt(ESContext *esContext);

}  // namespace ara

#endif
