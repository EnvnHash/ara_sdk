//
// Created by sven on 9/5/20.
//

#if defined(__linux__) && !defined(__ANDROID__)
#ifndef ARA_USE_GLES31

#include "GLXNativeWindow.h"

#include <sys/time.h>

namespace ara {
bool GLXNativeWindow::create(const glWinPar& wp) {
    display = XOpenDisplay(nullptr);

    if (!display) {
        printf("Failed to open X display\n");
        exit(1);
    }

    // Get a matching FB config
    static int visual_attribs[] = {GLX_X_RENDERABLE, True, GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT, GLX_RENDER_TYPE,
                                   GLX_RGBA_BIT, GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
                                   GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8, GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8,
                                   GLX_DOUBLEBUFFER, True,
                                   // GLX_SAMPLE_BUFFERS  , 1,
                                   // GLX_SAMPLES         , 2,
                                   None};

    int glx_major, glx_minor;

    // FBConfigs were added in GLX version 1.3.
    if (!glXQueryVersion(display, &glx_major, &glx_minor) || ((glx_major == 1) && (glx_minor < 3)) || (glx_major < 1)) {
        printf("Invalid GLX version");
        exit(1);
    }

    int          fbcount;
    GLXFBConfig *fbc = glXChooseFBConfig(display, DefaultScreen(display), visual_attribs, &fbcount);
    if (!fbc) {
        printf("Failed to retrieve a framebuffer config\n");
        exit(1);
    }

    // Pick the FB config/visual with the most samples per pixel
    int best_fbc = -1, worst_fbc = -1, best_num_samp = -1, worst_num_samp = 999;

    int i;
    for (i = 0; i < fbcount; ++i) {
        XVisualInfo *vi = glXGetVisualFromFBConfig(display, fbc[i]);
        if (vi) {
            int samp_buf, samples;
            glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf);
            glXGetFBConfigAttrib(display, fbc[i], GLX_SAMPLES, &samples);

            if (wp.nrSamples == samples) {
                best_fbc = i;
                break;
            }
            //  printf( "  Matching fbconfig %d, visual ID 0x%2x: SAMPLE_BUFFERS
            //  = %d, SAMPLES = %d\n",  i, vi -> visualid, samp_buf, samples );

            /*
              if ( best_fbc < 0 || samp_buf && samples > best_num_samp )
                  best_fbc = i, best_num_samp = samples;
              if ( worst_fbc < 0 || !samp_buf || samples < worst_num_samp )
                  worst_fbc = i, worst_num_samp = samples;
                  */
        }
        XFree(vi);
    }

    GLXFBConfig bestFbc = fbc[best_fbc];

    // Be sure to free the FBConfig list allocated by glXChooseFBConfig()
    XFree(fbc);

    // Get a visual
    XVisualInfo *vi = glXGetVisualFromFBConfig(display, bestFbc);
    // printf( "Chosen visual ID = 0x%x\n", vi->visualid );

    XSetWindowAttributes swa;
    swa.colormap = cmap   = XCreateColormap(display, RootWindow(display, vi->screen), vi->visual, AllocNone);
    swa.background_pixmap = None;
    swa.border_pixel      = 0;
    swa.event_mask        = StructureNotifyMask;

    win = XCreateWindow(display, RootWindow(display, vi->screen), wp.shift.x, wp.shift.y, wp.size.x, wp.size.y, 0, vi->depth,
                        InputOutput, vi->visual, CWBorderPixel | CWColormap | CWEventMask, &swa);
    if (!win) {
        printf("Failed to create window.\n");
        exit(1);
    }

    // Done with the visual info data
    XFree(vi);

    XStoreName(display, win, "GLX Native Win");
    if (!wp.hidden) {
        XMapWindow(display, win);
    }

    // Get the default screen's GLX extension list
    const char *glxExts = glXQueryExtensionsString(display, DefaultScreen(display));

    // NOTE: It is not necessary to create or make current to a context before
    // calling glXGetProcAddressARB
    glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");

    ctx = 0;

    // Install an X error handler so the application won't exit if GL 3.0
    // context allocation fails.
    //
    // Note this error handler is global.  All display connections in all
    // threads of a process use the same error handler, so be sure to guard
    // against other threads issuing X commands while this code is running.
    ctxErrorOccurred                            = false;
    int (*oldHandler)(Display *, XErrorEvent *) = XSetErrorHandler(&ctxErrorHandler);

    // Check for the GLX_ARB_create_context extension string and the function.
    // If either is not present, use GLX 1.3 context creation method.
    if (!isExtensionSupported(glxExts, "GLX_ARB_create_context") || !glXCreateContextAttribsARB) {
        printf("glXCreateContextAttribsARB() not found ... using old-style GLX context\n");
        ctx = glXCreateNewContext(display, bestFbc, GLX_RGBA_TYPE, 0, True);
    }
    // If it does, try to get a GL 3.0 context!
    else {
        int context_attribs[] = {GLX_CONTEXT_MAJOR_VERSION_ARB, 3, GLX_CONTEXT_MINOR_VERSION_ARB, 0,
                                 // GLX_CONTEXT_FLAGS_ARB        ,
                                 // GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
                                 None};

        ctx = glXCreateContextAttribsARB(display, bestFbc, (GLXContext)wp.sharedCtx, True, context_attribs);

        // Sync to ensure any errors generated are processed.
        XSync(display, False);

        if (!(!ctxErrorOccurred && ctx)) {
            // Couldn't create GL 3.0 context.  Fall back to old-style 2.x
            // context. When a context version below 3.0 is requested,
            // implementations will return the newest context version compatible
            // with OpenGL versions less than version 3.0.
            // GLX_CONTEXT_MAJOR_VERSION_ARB = 1
            context_attribs[1] = 1;
            // GLX_CONTEXT_MINOR_VERSION_ARB = 0
            context_attribs[3] = 0;

            ctxErrorOccurred = false;

            printf("Failed to create GL 3.0 context  ... using old-style GLX context\n");
            ctx = glXCreateContextAttribsARB(display, bestFbc, 0, True, context_attribs);
        }
    }

    // This is the butcher's way of removing window decorations
    // Setting the override-redirect attribute on a window makes the
    // window manager ignore the window completely (ICCCM, section 4)
    // The good thing is that this makes undecorated full screen windows
    // easy to do; the bad thing is that we have to do everything
    // manually and some things (like iconify/restore) won't work at
    // all, as those are tasks usually performed by the window manager
    if (!wp.decorated) {
        XSetWindowAttributes attributes;
        attributes.override_redirect = True;
        XChangeWindowAttributes(display, win, CWOverrideRedirect, &attributes);
    }

    // Sync to ensure any errors generated are processed.
    XSync(display, False);

    // Restore the original error handler
    XSetErrorHandler(oldHandler);

    if (ctxErrorOccurred || !ctx) {
        printf("Failed to create an OpenGL context\n");
        exit(1);
    }

    // Verifying that context is a direct context
    if (!glXIsDirect(display, ctx)) printf("Indirect GLX rendering context obtained\n");

    //  glXSwapIntervalEXT(0);
    return true;
}

// Wait for data to arrive using select
// This avoids blocking other threads via the per-display Xlib lock that also
// covers GLX functions
bool GLXNativeWindow::waitForEvent(double *timeout) {
    if (!display) return false;

    fd_set    fds;
    const int fd    = ConnectionNumber(display);
    int       count = fd + 1;

    for (;;) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        if (timeout) {
            const long     seconds      = (long)*timeout;
            const long     microseconds = (long)((*timeout - seconds) * 1e6);
            struct timeval tv           = {seconds, microseconds};
            const uint64_t base         = getTimerValue();

            const int result = select(count, &fds, nullptr, nullptr, &tv);
            const int error  = errno;

            *timeout -= (getTimerValue() - base) / (double)getTimerFrequency();

            if (result > 0) return true;
            if ((result == -1 && error == EINTR) || *timeout <= 0.0) return false;
        } else if (select(count, &fds, nullptr, nullptr, nullptr) != -1 || errno != EINTR)
            return true;
    }
}

uint64_t GLXNativeWindow::getTimerValue(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * (uint64_t)1000000 + (uint64_t)tv.tv_usec;
}

// Waits until a VisibilityNotify event arrives for the specified window or the
// timeout period elapses (ICCCM section 4.2.2)
bool GLXNativeWindow::waitForVisibilityNotify() {
    if (!display) return false;
    XEvent dummy;
    double timeout = 0.1;
    while (!XCheckTypedWindowEvent(display, win, VisibilityNotify, &dummy)) {
        if (!waitForEvent(&timeout)) return false;
    }

    return true;
}

void GLXNativeWindow::destroy() {
    if (!display) return;

    glXMakeCurrent(display, 0, 0);
    glXDestroyContext(display, ctx);

    XDestroyWindow(display, win);
    XFreeColormap(display, cmap);
    XCloseDisplay(display);
    display = nullptr;
}

GLXNativeWindow::~GLXNativeWindow() { destroy(); }
}  // namespace ara

#endif
#endif