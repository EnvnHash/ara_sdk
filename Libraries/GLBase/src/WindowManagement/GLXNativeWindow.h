//
// Created by sven on 9/5/20.
//

#if defined(__linux__) && !defined(__ANDROID__)
#ifndef ARA_USE_GLES31

#pragma once

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GlbCommon/GlbCommon.h>
#include <unistd.h>
#include "GLWindowCommon.h"

#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display *, GLXFBConfig, GLXContext, Bool, const int *);

namespace ara {

class GLXNativeWindow {
public:
    virtual ~GLXNativeWindow();

    static bool isExtensionSupported(const char *extList, const char *extension) {
        const char *start;
        const char *where, *terminator;

        // Extension names should not have spaces.
        where = strchr(extension, ' ');
        if (where || *extension == '\0') {
          return false;
        }

        // It takes a bit of care to be fool-proof about parsing the
        // OpenGL extensions string. Don't be fooled by sub-strings,  etc.
        for (start = extList;;) {
            where = strstr(start, extension);

            if (!where) {
              break;
            }

            terminator = where + std::string(extension).length();

            if (where == start || *(where - 1) == ' ') {
                if (*terminator == ' ' || *terminator == '\0') {
                  return true;
                }
            }

            start = terminator;
        }

        return false;
    }

    static int ctxErrorHandler(Display *dpy, XErrorEvent *ev) {
        ctxErrorOccurred = true;
        return 0;
    }

    bool create(const glWinPar&);

    bool init();

    void open() {
        if (display) {
            XMapWindow(display, win);
            waitForVisibilityNotify();
            m_isOpen      = true;
            m_requestOpen = false;
        }
    }

    void close() {
        if (display) {
            XUnmapWindow(display, win);
            XFlush(display);
            m_isOpen       = false;
            m_requestClose = false;
        }
    }

    void swap() { glXSwapBuffers(display, win); }
    void makeCurrent() { glXMakeCurrent(display, win, ctx); }
    void destroy();
    void resize(GLsizei width, GLsizei height);

    void       pollEvents() {}
    GLXContext getGlxCtx() { return ctx; }
    Window    *getWin() { return &win; }
    uint32_t   getWidth() { return m_widthVirt; }
    uint32_t   getHeight() { return m_heightVirt; }
    uint32_t   getPosX() { return m_offsX; }
    uint32_t   getPosY() { return m_offsY; }
    bool       isOpen() { return m_isOpen; }
    bool       getRequestOpen() { return m_requestOpen; }
    bool       getRequestClose() { return m_requestClose; }

    void     requestOpen(bool val) { m_requestOpen = val; }
    void     requestClose(bool val) { m_requestClose = val; }
    uint64_t getTimerFrequency() { return 1000000; }

    uint64_t getTimerValue();
    bool     waitForEvent(double *timeout);
    bool     waitForVisibilityNotify();

    inline static bool ctxErrorOccurred = true;

private:
    Display   *display = nullptr;
    Window     win;
    GLXContext ctx;
    Colormap   cmap;

    static bool m_active;                // Window Active Flag Set To TRUE By Default
    bool        m_fullscreen   = false;  // Fullscreen Flag Set To Fullscreen Mode By Default
    bool        m_isOpen       = false;
    bool        m_run          = false;
    bool        m_transparent  = false;
    bool        m_requestOpen  = false;
    bool        m_requestClose = false;
    uint32_t    m_widthVirt    = 0;
    uint32_t    m_heightVirt   = 0;
    uint32_t    m_offsX        = 0;
    uint32_t    m_offsY        = 0;
};

}  // namespace ara

#endif
#endif