//
// Created by sven on 6/28/21.
//

#if defined(__linux__) && !defined(__ANDROID__)

#include "X11Window.h"

namespace ara {

bool X11Window::create(glWinPar &gp) {
    return create((char *)"", gp.width, gp.height, gp.shiftX, gp.shiftY, gp.bits, gp.fullScreen, gp.createHidden,
                  gp.decorated, gp.resizeable, gp.floating, gp.transparent, gp.shareCont);
}

bool X11Window::create(char *title, uint32_t width, uint32_t height, uint32_t m_posX, uint32_t posY, uint32_t bits,
                       uint32_t nrSamples, bool fullscreenflag, bool hidden, bool decorated, bool resizable,
                       bool floating, bool transparent, void *shareCtx) {
    XrmInitialize();

    m_display = XOpenDisplay(nullptr);
    if (!m_display) return false;

    createKeyTables();
    initAtoms();

    m_root   = DefaultRootWindow(m_display);
    m_parent = m_root;

    XSetWindowAttributes swa;
    swa.event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask | PointerMotionMask | ButtonPressMask |
                     ButtonReleaseMask | ExposureMask | FocusChangeMask | VisibilityChangeMask | EnterWindowMask |
                     LeaveWindowMask | PropertyChangeMask;
    m_win = XCreateWindow(m_display, m_parent, 0, 0, width, height, 0, CopyFromParent, InputOutput, CopyFromParent,
                          CWEventMask, &swa);
    s_wmDeleteMessage = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(m_display, m_win, &s_wmDeleteMessage, 1);

    XSetWindowAttributes xattr;
    xattr.override_redirect = false;
    XChangeWindowAttributes(m_display, m_win, CWOverrideRedirect, &xattr);

    XWMHints hints;
    hints.input = true;
    hints.flags = InputHint;
    XSetWMHints(m_display, m_win, &hints);

    // make the window visible on the screen
    XMapWindow(m_display, m_win);
    XStoreName(m_display, m_win, title);

    // Create a colormap based on the visual used by the current context
    // m_colormap = XCreateColormap(m_display, root, visual, AllocNone);

    XSetWindowAttributes wa = {0};

    if (XSupportsLocale()) {
        XSetLocaleModifiers("");

        m_im = XOpenIM(m_display, 0, nullptr, nullptr);
        if (m_im) {
            if (!hasUsableInputMethodStyle()) {
                XCloseIM(m_im);
                m_im = nullptr;
            }
        }
    }

    if (m_im) {
        m_ic = XCreateIC(m_im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, m_win, XNFocusWindow,
                         m_win, nullptr);
    }

    if (m_ic) {
        unsigned long filter = 0;
        if (XGetICValues(m_ic, XNFilterEvents, &filter, nullptr) == nullptr)
            XSelectInput(m_display, m_win, wa.event_mask | filter);
    }

    // get identifiers for the provided atom name strings
    Atom wm_state = XInternAtom(m_display, "_NET_WM_STATE", false);

    XEvent xev = {0};
    // memset ( &xev, 0, sizeof(xev) );
    xev.type                 = ClientMessage;
    xev.xclient.window       = m_win;
    xev.xclient.message_type = wm_state;
    xev.xclient.format       = 32;
    xev.xclient.data.l[0]    = 1;
    xev.xclient.data.l[1]    = false;
    XSendEvent(m_display, DefaultRootWindow(m_display), false, SubstructureNotifyMask, &xev);

    return true;
}

void X11Window::initAtoms() {
    // String m_format atoms
    m_NULL_       = XInternAtom(m_display, "NULL", False);
    m_UTF8_STRING = XInternAtom(m_display, "UTF8_STRING", False);
    m_ATOM_PAIR   = XInternAtom(m_display, "ATOM_PAIR", False);

    // Custom selection property atom
    m_GLFW_SELECTION = XInternAtom(m_display, "GLFW_SELECTION", False);

    // ICCCM standard clipboard atoms
    m_TARGETS   = XInternAtom(m_display, "TARGETS", False);
    m_MULTIPLE  = XInternAtom(m_display, "MULTIPLE", False);
    m_PRIMARY   = XInternAtom(m_display, "PRIMARY", False);
    m_INCR      = XInternAtom(m_display, "INCR", False);
    m_CLIPBOARD = XInternAtom(m_display, "CLIPBOARD", False);

    // Clipboard manager atoms
    m_CLIPBOARD_MANAGER = XInternAtom(m_display, "CLIPBOARD_MANAGER", False);
    m_SAVE_TARGETS      = XInternAtom(m_display, "SAVE_TARGETS", False);

    // Xdnd (drag and drop) atoms
    m_XdndAware      = XInternAtom(m_display, "XdndAware", False);
    m_XdndEnter      = XInternAtom(m_display, "XdndEnter", False);
    m_XdndPosition   = XInternAtom(m_display, "XdndPosition", False);
    m_XdndStatus     = XInternAtom(m_display, "XdndStatus", False);
    m_XdndActionCopy = XInternAtom(m_display, "XdndActionCopy", False);
    m_XdndDrop       = XInternAtom(m_display, "XdndDrop", False);
    m_XdndFinished   = XInternAtom(m_display, "XdndFinished", False);
    m_XdndSelection  = XInternAtom(m_display, "XdndSelection", False);
    m_XdndTypeList   = XInternAtom(m_display, "XdndTypeList", False);
    m_text_uri_list  = XInternAtom(m_display, "text/uri-list", False);

    // ICCCM, EWMH and Motif window property atoms
    // These can be set safely even without WM support
    // The EWMH atoms that require WM support are handled in detectEWMH
    m_WM_PROTOCOLS             = XInternAtom(m_display, "WM_PROTOCOLS", False);
    m_WM_STATE                 = XInternAtom(m_display, "WM_STATE", False);
    m_WM_DELETE_WINDOW         = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
    m_NET_SUPPORTED            = XInternAtom(m_display, "_NET_SUPPORTED", False);
    m_NET_SUPPORTING_WM_CHECK  = XInternAtom(m_display, "_NET_SUPPORTING_WM_CHECK", False);
    m_NET_WM_ICON              = XInternAtom(m_display, "_NET_WM_ICON", False);
    m_NET_WM_PING              = XInternAtom(m_display, "_NET_WM_PING", False);
    m_NET_WM_PID               = XInternAtom(m_display, "_NET_WM_PID", False);
    m_NET_WM_NAME              = XInternAtom(m_display, "_NET_WM_NAME", False);
    m_NET_WM_ICON_NAME         = XInternAtom(m_display, "_NET_WM_ICON_NAME", False);
    m_NET_WM_BYPASS_COMPOSITOR = XInternAtom(m_display, "_NET_WM_BYPASS_COMPOSITOR", False);
    m_NET_WM_WINDOW_OPACITY    = XInternAtom(m_display, "_NET_WM_WINDOW_OPACITY", False);
    m_MOTIF_WM_HINTS           = XInternAtom(m_display, "_MOTIF_WM_HINTS", False);
}

// Check whether the IM has a usable style
//
bool X11Window::hasUsableInputMethodStyle() {
    bool       found  = false;
    XIMStyles *styles = nullptr;

    if (XGetIMValues(m_im, XNQueryInputStyle, &styles, nullptr) != nullptr) return false;

    for (unsigned int i = 0; i < styles->count_styles; i++) {
        if (styles->supported_styles[i] == (XIMPreeditNothing | XIMStatusNothing)) {
            found = true;
            break;
        }
    }

    XFree(styles);
    return found;
}

void X11Window::waitEvents() {
    while (!XPending(m_display)) waitForEvent(nullptr);

    pollEvents();
}

// Wait for data to arrive using select
// This avoids blocking other threads via the per-display Xlib lock that also
// covers GLX functions
bool X11Window::waitForEvent(double *timeout) {
    fd_set    fds   = {0};
    const int fd    = ConnectionNumber(m_display);
    int       count = fd + 1;
    FD_SET(fd, &fds);

    for (;;) {
        if (select(count, &fds, nullptr, nullptr, nullptr) != -1 || errno != EINTR) return true;
    }
}

void X11Window::pollEvents() {
    XPending(m_display);

    while (XQLength(m_display)) {
        bool filtered = False;
        int  keycode  = 0;

        XEvent event;
        XNextEvent(m_display, &event);

        // HACK: Save scancode as some IMs clear the field in XFilterEvent
        if (event.type == KeyPress || event.type == KeyRelease) keycode = event.xkey.keycode;

        if (m_im) filtered = XFilterEvent(&event, None);

        if (m_xkb.available) {
            if (event.type == m_xkb.eventBase + XkbEventCode) {
                if (((XkbEvent *)&event)->any.xkb_type == XkbStateNotify &&
                    (((XkbEvent *)&event)->state.changed & XkbGroupStateMask)) {
                    m_xkb.group = ((XkbEvent *)&event)->state.group;
                }

                return;
            }
        }

        if (event.type == GenericEvent) {
            //            if (m_xi.available)
            //            {
            //                X11Window* window = m_disabledCursorWindow;
            //
            //                if (window &&
            //                    m_rawMouseMotion &&
            //                    event.xcookie.extension == m_xi.majorOpcode &&
            //                    XGetEventData(m_display, &event.xcookie) &&
            //                    event.xcookie.evtype == XI_RawMotion)
            //                {
            //                    XIRawEvent* re =
            //                    (XIRawEvent*)event.xcookie.data; if
            //                    (re->valuators.mask_len)
            //                    {
            //                        const double* values = re->raw_values;
            //                        double xpos = m_virtualCursorPosX;
            //                        double ypos = m_virtualCursorPosY;
            //
            //                        if (XIMaskIsSet(re->valuators.mask, 0))
            //                        {
            //                            xpos += *values;
            //                            values++;
            //                        }
            //
            //                        if (XIMaskIsSet(re->valuators.mask, 1))
            //                            ypos += *values;
            //
            //                        onMouseCursor(xpos, ypos);
            //                    }
            //                }
            //
            //                XFreeEventData(m_display, &event.xcookie);
            //            }
            return;
        }

        if (event.type == SelectionClear) {
            handleSelectionClear(&event);
            return;
        } else if (event.type == SelectionRequest) {
            handleSelectionRequest(&event);
            return;
        }

        switch (event.type) {
            case ReparentNotify: {
                m_parent = event.xreparent.parent;
                return;
            }

            case KeyPress: {
                const int key   = translateKey(keycode);
                const int mods  = translateState(event.xkey.state);
                const int plain = !(mods & (GLSG_MOD_CONTROL | GLSG_MOD_ALT));

                if (m_ic) {
                    // HACK: Do not report the key press events duplicated by
                    // XIM
                    //       Duplicate key releases are filtered out implicitly
                    //       by the GLFW key repeat logic in _glfwInputKey A
                    //       timestamp per key is used to handle simultaneous
                    //       keys
                    // NOTE: Always allow the first event for each key through
                    //       (the server never sends a timestamp of zero)
                    // NOTE: Timestamp difference is compared to handle
                    // wrap-around
                    Time diff = event.xkey.time - m_keyPressTimes[keycode];
                    if (diff == event.xkey.time || (diff > 0 && diff < (1 << 31))) {
                        if (keycode) onKey(key, keycode, GLSG_PRESS, mods);

                        m_keyPressTimes[keycode] = event.xkey.time;
                    }

                    if (!filtered) {
                        int    count;
                        Status status;
#if defined(X_HAVE_UTF8_STRING)
                        char  buffer[100];
                        char *chars = buffer;

                        count = Xutf8LookupString(m_ic, &event.xkey, buffer, sizeof(buffer) - 1, nullptr, &status);

                        if (status == XBufferOverflow) {
                            chars = (char *)calloc(count + 1, 1);
                            count = Xutf8LookupString(m_ic, &event.xkey, chars, count, nullptr, &status);
                        }

                        if (status == XLookupChars || status == XLookupBoth) {
                            const char *c = chars;
                            chars[count]  = '\0';
                            while (c - chars < count) onChar(decodeUTF8(&c));
                            //  _glfwInputChar(window, decodeUTF8(&c), mods,
                            //  plain);
                        }
#else  /*X_HAVE_UTF8_STRING*/
                        wchar_t  m_buffer[16];
                        wchar_t *chars = m_buffer;

                        count = XwcLookupString(m_ic, &event.xkey, m_buffer, sizeof(m_buffer) / sizeof(wchar_t),
                                                nullptr, &status);

                        if (status == XBufferOverflow) {
                            chars = calloc(count, sizeof(wchar_t));
                            count = XwcLookupString(m_ic, &event.xkey, chars, count, nullptr, &status);
                        }

                        if (status == XLookupChars || status == XLookupBoth) {
                            int i;
                            for (i = 0; i < count; i++) _glfwInputChar(window, chars[i], mods, plain);
                        }
#endif /*X_HAVE_UTF8_STRING*/

                        if (chars != buffer) free(chars);
                    }
                } else {
                    KeySym keysym;
                    XLookupString(&event.xkey, nullptr, 0, &keysym, nullptr);

                    //_glfwInputKey(window, key, keycode, GLSG_PRESS, mods);

                    const long character = keySym2Unicode(keysym);
                    // if (character != -1)
                    //   _glfwInputChar(window, character, mods, plain);
                }

                return;
            }

            case KeyRelease: {
                const int key  = translateKey(keycode);
                const int mods = translateState(event.xkey.state);

                if (!m_xkb.detectable) {
                    // HACK: Key repeat events will arrive as
                    // KeyRelease/KeyPress
                    //       pairs with similar or identical time stamps
                    //       The key repeat logic in _glfwInputKey expects only
                    //       key presses to repeat, so detect and discard
                    //       release events
                    if (XEventsQueued(m_display, QueuedAfterReading)) {
                        XEvent next;
                        XPeekEvent(m_display, &next);

                        if (next.type == KeyPress && next.xkey.window == event.xkey.window &&
                            next.xkey.keycode == keycode) {
                            // HACK: The time of repeat events sometimes doesn't
                            //       match that of the press event, so add an
                            //       epsilon
                            //       Toshiyuki Takahashi can press a button
                            //       16 times per second so it's fairly safe to
                            //       assume that no human is pressing the key 50
                            //       times per second (value is ms)
                            if ((next.xkey.time - event.xkey.time) < 20) {
                                // This is very likely a server-generated key
                                // repeat event, so ignore it
                                return;
                            }
                        }
                    }
                }

                onKey(key, keycode, GLSG_RELEASE, mods);
                return;
            }

            case ButtonPress: {
                const int mods = translateState(event.xbutton.state);

                if (event.xbutton.button == Button1)
                    onMouseButton(GLSG_MOUSE_BUTTON_LEFT, GLSG_PRESS, mods);
                else if (event.xbutton.button == Button2)
                    onMouseButton(GLSG_MOUSE_BUTTON_MIDDLE, GLSG_PRESS, mods);
                else if (event.xbutton.button == Button3)
                    onMouseButton(GLSG_MOUSE_BUTTON_RIGHT, GLSG_PRESS, mods);

                // Modern X provides scroll events as mouse button presses
                else if (event.xbutton.button == Button4)
                    onScroll(0.0, 1.0);
                else if (event.xbutton.button == Button5)
                    onScroll(0.0, -1.0);
                //                else if (event.xbutton.button == Button6)
                //                    onScroll(1.0, 0.0);
                //                else if (event.xbutton.button == Button7)
                //                    onScroll(-1.0, 0.0);

                else {
                    // Additional buttons after 7 are treated as regular buttons
                    // We subtract 4 to fill the gap left by scroll input above
                    onMouseButton(event.xbutton.button - Button1 - 4, GLSG_PRESS, mods);
                }

                return;
            }

            case ButtonRelease: {
                const int mods = translateState(event.xbutton.state);

                if (event.xbutton.button == Button1) {
                    onMouseButton(GLSG_MOUSE_BUTTON_LEFT, GLSG_RELEASE, mods);
                } else if (event.xbutton.button == Button2) {
                    onMouseButton(GLSG_MOUSE_BUTTON_MIDDLE, GLSG_RELEASE, mods);
                } else if (event.xbutton.button == Button3) {
                    onMouseButton(GLSG_MOUSE_BUTTON_RIGHT, GLSG_RELEASE, mods);
                }
                //                else if (event.xbutton.button > Button7)
                //                {
                //                    // Additional buttons after 7 are treated
                //                    as regular buttons
                //                    // We subtract 4 to fill the gap left by
                //                    scroll input above
                //                    onMouseButton(event.xbutton.button -
                //                    Button1 - 4, GLSG_RELEASE, mods);
                //                }

                return;
            }

            case EnterNotify: {
                // XEnterWindowEvent is XCrossingEvent
                const int x = event.xcrossing.x;
                const int y = event.xcrossing.y;

                // HACK: This is a workaround for WMs (KWM, Fluxbox) that
                // otherwise
                //       ignore the defined cursor for hidden cursor mode
                // if (m_cursorMode == GLSG_CURSOR_HIDDEN)
                //    updateCursorImage(window);

                onMouseEnter(true);
                onMouseCursor(x, y);

                m_lastCursorPosX = x;
                m_lastCursorPosY = y;
                return;
            }

            case LeaveNotify: {
                onMouseEnter(false);
                return;
            }

            case MotionNotify: {
                const int x = event.xmotion.x;
                const int y = event.xmotion.y;

                if (x != m_warpCursorPosX || y != m_warpCursorPosY) {
                    // The cursor was moved by something other than GLFW

                    if (m_cursorMode == GLSG_CURSOR_DISABLED) {
                        //                        if (m_disabledCursorWindow !=
                        //                        window)
                        //                            return;

                        if (m_rawMouseMotion) return;

                        const int dx = x - m_lastCursorPosX;
                        const int dy = y - m_lastCursorPosY;

                        onMouseCursor(m_virtualCursorPosX + dx, m_virtualCursorPosY + dy);
                    } else
                        onMouseCursor(x, y);
                }

                m_lastCursorPosX = x;
                m_lastCursorPosY = y;
                return;
            }

            case ConfigureNotify: {
                if (event.xconfigure.width != m_widthVirt || event.xconfigure.height != m_heightVirt) {
                    onFrameBufferSize(event.xconfigure.width, event.xconfigure.height);
                    onWindowSize(event.xconfigure.width, event.xconfigure.height);

                    m_widthVirt  = event.xconfigure.width;
                    m_heightVirt = event.xconfigure.height;
                }

                int xpos = event.xconfigure.x;
                int ypos = event.xconfigure.y;

                // NOTE: ConfigureNotify events from the server are in local
                //       coordinates, so if we are reparented we need to
                //       translate the position into root (screen) coordinates
                if (!event.xany.send_event && m_parent != m_root) {
                    grabErrorHandler();

                    Window dummy;
                    XTranslateCoordinates(m_display, m_parent, m_root, xpos, ypos, &xpos, &ypos, &dummy);

                    releaseErrorHandler();
                    if (m_errorCode == BadWindow) return;
                }

                if (xpos != m_offsX || ypos != m_offsY) {
                    onWindowPos(xpos, ypos);
                    m_offsX = xpos;
                    m_offsY = ypos;
                }

                return;
            }

            case ClientMessage: {
                // Custom client message, probably from the window manager

                if (filtered) return;

                if (event.xclient.message_type == None) return;

                if (event.xclient.message_type == m_WM_PROTOCOLS) {
                    const Atom protocol = event.xclient.data.l[0];
                    if (protocol == None) return;

                    if (protocol == m_WM_DELETE_WINDOW) {
                        // The window manager was asked to close the window, for
                        // example by the user pressing a 'close' window
                        // decoration button
                        onWindowClose();
                    } else if (protocol == m_NET_WM_PING) {
                        // The window manager is pinging the application to
                        // ensure it's still responding to events

                        XEvent reply         = event;
                        reply.xclient.window = m_root;

                        XSendEvent(m_display, m_root, False, SubstructureNotifyMask | SubstructureRedirectMask, &reply);
                    }
                } else if (event.xclient.message_type == m_XdndEnter) {
                    // A drag operation has entered the window
                    unsigned long i, count;
                    Atom         *formats = nullptr;
                    const bool    list    = event.xclient.data.l[1] & 1;

                    m_xdnd.source   = event.xclient.data.l[0];
                    m_xdnd.version  = event.xclient.data.l[1] >> 24;
                    m_xdnd.m_format = None;

                    if (m_xdnd.version > _GLSG_XDND_VERSION) return;

                    if (list) {
                        count = getWindowProperty(m_xdnd.source, m_XdndTypeList, XA_ATOM, (unsigned char **)&formats);
                    } else {
                        count   = 3;
                        formats = (Atom *)event.xclient.data.l + 2;
                    }

                    for (i = 0; i < count; i++) {
                        if (formats[i] == m_text_uri_list) {
                            m_xdnd.m_format = m_text_uri_list;
                            break;
                        }
                    }

                    if (list && formats) XFree(formats);
                } else if (event.xclient.message_type == m_XdndDrop) {
                    // The drag operation has finished by dropping on the window
                    Time time = CurrentTime;

                    if (m_xdnd.version > _GLSG_XDND_VERSION) return;

                    if (m_xdnd.m_format) {
                        if (m_xdnd.version >= 1) time = event.xclient.data.l[2];

                        // Request the chosen m_format from the source window
                        XConvertSelection(m_display, m_XdndSelection, m_xdnd.m_format, m_XdndSelection, m_win, time);
                    } else if (m_xdnd.version >= 2) {
                        XEvent reply               = {ClientMessage};
                        reply.xclient.window       = m_xdnd.source;
                        reply.xclient.message_type = m_XdndFinished;
                        reply.xclient.format       = 32;
                        reply.xclient.data.l[0]    = m_win;
                        reply.xclient.data.l[1]    = 0;  // The drag was rejected
                        reply.xclient.data.l[2]    = None;

                        XSendEvent(m_display, m_xdnd.source, False, NoEventMask, &reply);
                        XFlush(m_display);
                    }
                } else if (event.xclient.message_type == m_XdndPosition) {
                    // The drag operation has moved over the window
                    const int xabs = (event.xclient.data.l[2] >> 16) & 0xffff;
                    const int yabs = (event.xclient.data.l[2]) & 0xffff;
                    Window    dummy;
                    int       xpos, ypos;

                    if (m_xdnd.version > _GLSG_XDND_VERSION) return;

                    XTranslateCoordinates(m_display, m_root, m_win, xabs, yabs, &xpos, &ypos, &dummy);

                    onMouseCursor(xpos, ypos);

                    XEvent reply               = {ClientMessage};
                    reply.xclient.window       = m_xdnd.source;
                    reply.xclient.message_type = m_XdndStatus;
                    reply.xclient.format       = 32;
                    reply.xclient.data.l[0]    = m_win;
                    reply.xclient.data.l[2]    = 0;  // Specify an empty rectangle
                    reply.xclient.data.l[3]    = 0;

                    if (m_xdnd.m_format) {
                        // Reply that we are ready to copy the dragged data
                        reply.xclient.data.l[1] = 1;  // Accept with no rectangle
                        if (m_xdnd.version >= 2) reply.xclient.data.l[4] = m_XdndActionCopy;
                    }

                    XSendEvent(m_display, m_xdnd.source, False, NoEventMask, &reply);
                    XFlush(m_display);
                }

                return;
            }

            case SelectionNotify: {
                if (event.xselection.property == m_XdndSelection) {
                    // The converted data from the drag operation has arrived
                    char               *data;
                    const unsigned long result =
                        getWindowProperty(event.xselection.requestor, event.xselection.property,
                                          event.xselection.target, (unsigned char **)&data);

                    if (result) {
                        int    i, count;
                        char **paths = parseUriList(data, &count);

                        onDrop(count, (const char **)paths);

                        for (i = 0; i < count; i++) free(paths[i]);
                        free(paths);
                    }

                    if (data) XFree(data);

                    if (m_xdnd.version >= 2) {
                        XEvent reply               = {ClientMessage};
                        reply.xclient.window       = m_xdnd.source;
                        reply.xclient.message_type = m_XdndFinished;
                        reply.xclient.format       = 32;
                        reply.xclient.data.l[0]    = m_win;
                        reply.xclient.data.l[1]    = result;
                        reply.xclient.data.l[2]    = m_XdndActionCopy;

                        XSendEvent(m_display, m_xdnd.source, False, NoEventMask, &reply);
                        XFlush(m_display);
                    }
                }

                return;
            }

            case FocusIn: {
                if (event.xfocus.mode == NotifyGrab || event.xfocus.mode == NotifyUngrab) {
                    // Ignore focus events from popup indicator windows, window
                    // menu key chords and window dragging
                    return;
                }

                if (m_cursorMode == GLSG_CURSOR_DISABLED) disableCursor();

                if (m_ic) XSetICFocus(m_ic);

                onWindowFocus(1);
                return;
            }

            case FocusOut: {
                if (event.xfocus.mode == NotifyGrab || event.xfocus.mode == NotifyUngrab) {
                    // Ignore focus events from popup indicator windows, window
                    // menu key chords and window dragging
                    return;
                }

                if (m_cursorMode == GLSG_CURSOR_DISABLED) enableCursor();

                if (m_ic) XUnsetICFocus(m_ic);

                if (
                    // m_monitor &&
                    m_autoIconify)
                    onWindowIconify(1);

                onWindowFocus(0);
                return;
            }

            case Expose: {
                onWindowRefresh();
                return;
            }

            case PropertyNotify: {
                if (event.xproperty.state != PropertyNewValue) return;

                if (event.xproperty.atom == m_WM_STATE) {
                    const int state = getWindowState();
                    if (state != IconicState && state != NormalState) return;

                    const bool iconified = (state == IconicState);
                    if (m_iconified != iconified) {
                        // if (window->monitor) {
                        //                            if (iconified)
                        //                                releaseMonitor();
                        //                            else
                        //                                acquireMonitor();
                        // }

                        m_iconified = iconified;
                        onWindowIconify(iconified);
                    }
                }
                //                else if (event.xproperty.atom ==
                //                m_NET_WM_STATE)
                //                {
                //                    const bool maximized =
                //                    _glfwPlatformWindowMaximized(window); if
                //                    (m_maximized != maximized)
                //                    {
                //                        m_maximized = maximized;
                //                        onWindowMaximize(maximized);
                //                    }
                //                }

                return;
            }

            case DestroyNotify: return;

            default: return;
        }
    }

    XFlush(m_display);
}

void X11Window::handleSelectionClear(XEvent *event) {
    if (event->xselectionclear.selection == m_PRIMARY) {
        delete m_primarySelectionString;
        m_primarySelectionString = nullptr;
    } else {
        delete m_clipboardString;
        m_clipboardString = nullptr;
    }
}

void X11Window::handleSelectionRequest(XEvent *event) {
    const XSelectionRequestEvent *request = &event->xselectionrequest;

    XEvent reply               = {SelectionNotify};
    reply.xselection.property  = writeTargetToProperty(request);
    reply.xselection.display   = request->display;
    reply.xselection.requestor = request->requestor;
    reply.xselection.selection = request->selection;
    reply.xselection.target    = request->target;
    reply.xselection.time      = request->time;

    XSendEvent(m_display, request->requestor, False, 0, &reply);
}

// Set the specified property to the selection converted to the requested target
Atom X11Window::writeTargetToProperty(const XSelectionRequestEvent *request) {
    int        i;
    char      *selectionString = NULL;
    const Atom formats[]       = {m_UTF8_STRING, XA_STRING};
    const int  formatCount     = sizeof(formats) / sizeof(formats[0]);

    if (request->selection == m_PRIMARY)
        selectionString = m_primarySelectionString;
    else
        selectionString = m_clipboardString;

    if (request->property == None) {
        // The requester is a legacy client (ICCCM section 2.2)
        // We don't support legacy clients, so fail here
        return None;
    }

    if (request->target == m_TARGETS) {
        // The list of supported targets was requested

        const Atom targets[] = {m_TARGETS, m_MULTIPLE, m_UTF8_STRING, XA_STRING};

        XChangeProperty(m_display, request->requestor, request->property, XA_ATOM, 32, PropModeReplace,
                        (unsigned char *)targets, sizeof(targets) / sizeof(targets[0]));

        return request->property;
    }

    if (request->target == m_MULTIPLE) {
        // Multiple conversions were requested

        Atom         *targets;
        unsigned long i, count;

        count = getWindowProperty(request->requestor, request->property, m_ATOM_PAIR, (unsigned char **)&targets);

        for (i = 0; i < count; i += 2) {
            int j;

            for (j = 0; j < formatCount; j++) {
                if (targets[i] == formats[j]) break;
            }

            if (j < formatCount) {
                XChangeProperty(m_display, request->requestor, targets[i + 1], targets[i], 8, PropModeReplace,
                                (unsigned char *)selectionString, strlen(selectionString));
            } else
                targets[i + 1] = None;
        }

        XChangeProperty(m_display, request->requestor, request->property, m_ATOM_PAIR, 32, PropModeReplace,
                        (unsigned char *)targets, count);

        XFree(targets);

        return request->property;
    }

    if (request->target == m_SAVE_TARGETS) {
        // The request is a check whether we support SAVE_TARGETS
        // It should be handled as a no-op side effect target

        XChangeProperty(m_display, request->requestor, request->property, m_NULL_, 32, PropModeReplace, NULL, 0);

        return request->property;
    }

    // Conversion to a data target was requested

    for (i = 0; i < formatCount; i++) {
        if (request->target == formats[i]) {
            // The requested target is one we support

            XChangeProperty(m_display, request->requestor, request->property, request->target, 8, PropModeReplace,
                            (unsigned char *)selectionString, strlen(selectionString));

            return request->property;
        }
    }

    // The requested target is not supported

    return None;
}

// Translates an X11 key code to a GLFW key token
int X11Window::translateKey(int scancode) {
    // Use the pre-filled LUT (see createKeyTables() in x11_init.c)
    if (scancode < 0 || scancode > 255) return GLSG_KEY_UNKNOWN;

    return m_keycodes[scancode];
}

// Translate the X11 KeySyms for a key to a GLFW key code
// NOTE: This is only used as a fallback, in case the XKB method fails
//       It is layout-dependent and will fail partially on most non-US layouts
int X11Window::translateKeySyms(const KeySym *keysyms, int width) {
    if (width > 1) {
        switch (keysyms[1]) {
            case XK_KP_0: return GLSG_KEY_KP_0;
            case XK_KP_1: return GLSG_KEY_KP_1;
            case XK_KP_2: return GLSG_KEY_KP_2;
            case XK_KP_3: return GLSG_KEY_KP_3;
            case XK_KP_4: return GLSG_KEY_KP_4;
            case XK_KP_5: return GLSG_KEY_KP_5;
            case XK_KP_6: return GLSG_KEY_KP_6;
            case XK_KP_7: return GLSG_KEY_KP_7;
            case XK_KP_8: return GLSG_KEY_KP_8;
            case XK_KP_9: return GLSG_KEY_KP_9;
            case XK_KP_Separator:
            case XK_KP_Decimal: return GLSG_KEY_KP_DECIMAL;
            case XK_KP_Equal: return GLSG_KEY_KP_EQUAL;
            case XK_KP_Enter: return GLSG_KEY_KP_ENTER;
            default: break;
        }
    }

    switch (keysyms[0]) {
        case XK_Escape: return GLSG_KEY_ESCAPE;
        case XK_Tab: return GLSG_KEY_TAB;
        case XK_Shift_L: return GLSG_KEY_LEFT_SHIFT;
        case XK_Shift_R: return GLSG_KEY_RIGHT_SHIFT;
        case XK_Control_L: return GLSG_KEY_LEFT_CONTROL;
        case XK_Control_R: return GLSG_KEY_RIGHT_CONTROL;
        case XK_Meta_L:
        case XK_Alt_L: return GLSG_KEY_LEFT_ALT;
        case XK_Mode_switch:       // Mapped to Alt_R on many keyboards
        case XK_ISO_Level3_Shift:  // AltGr on at least some machines
        case XK_Meta_R:
        case XK_Alt_R: return GLSG_KEY_RIGHT_ALT;
        case XK_Super_L: return GLSG_KEY_LEFT_SUPER;
        case XK_Super_R: return GLSG_KEY_RIGHT_SUPER;
        case XK_Menu: return GLSG_KEY_MENU;
        case XK_Num_Lock: return GLSG_KEY_NUM_LOCK;
        case XK_Caps_Lock: return GLSG_KEY_CAPS_LOCK;
        case XK_Print: return GLSG_KEY_PRINT_SCREEN;
        case XK_Scroll_Lock: return GLSG_KEY_SCROLL_LOCK;
        case XK_Pause: return GLSG_KEY_PAUSE;
        case XK_Delete: return GLSG_KEY_DELETE;
        case XK_BackSpace: return GLSG_KEY_BACKSPACE;
        case XK_Return: return GLSG_KEY_ENTER;
        case XK_Home: return GLSG_KEY_HOME;
        case XK_End: return GLSG_KEY_END;
        case XK_Page_Up: return GLSG_KEY_PAGE_UP;
        case XK_Page_Down: return GLSG_KEY_PAGE_DOWN;
        case XK_Insert: return GLSG_KEY_INSERT;
        case XK_Left: return GLSG_KEY_LEFT;
        case XK_Right: return GLSG_KEY_RIGHT;
        case XK_Down: return GLSG_KEY_DOWN;
        case XK_Up: return GLSG_KEY_UP;
        case XK_F1: return GLSG_KEY_F1;
        case XK_F2: return GLSG_KEY_F2;
        case XK_F3: return GLSG_KEY_F3;
        case XK_F4: return GLSG_KEY_F4;
        case XK_F5: return GLSG_KEY_F5;
        case XK_F6: return GLSG_KEY_F6;
        case XK_F7: return GLSG_KEY_F7;
        case XK_F8: return GLSG_KEY_F8;
        case XK_F9: return GLSG_KEY_F9;
        case XK_F10: return GLSG_KEY_F10;
        case XK_F11: return GLSG_KEY_F11;
        case XK_F12: return GLSG_KEY_F12;
        case XK_F13: return GLSG_KEY_F13;
        case XK_F14: return GLSG_KEY_F14;
        case XK_F15: return GLSG_KEY_F15;
        case XK_F16: return GLSG_KEY_F16;
        case XK_F17: return GLSG_KEY_F17;
        case XK_F18: return GLSG_KEY_F18;
        case XK_F19: return GLSG_KEY_F19;
        case XK_F20: return GLSG_KEY_F20;
        case XK_F21: return GLSG_KEY_F21;
        case XK_F22: return GLSG_KEY_F22;
        case XK_F23: return GLSG_KEY_F23;
        case XK_F24: return GLSG_KEY_F24;
        case XK_F25:
            return GLSG_KEY_F25;

            // Numeric keypad
        case XK_KP_Divide: return GLSG_KEY_KP_DIVIDE;
        case XK_KP_Multiply: return GLSG_KEY_KP_MULTIPLY;
        case XK_KP_Subtract: return GLSG_KEY_KP_SUBTRACT;
        case XK_KP_Add:
            return GLSG_KEY_KP_ADD;

            // These should have been detected in secondary keysym test above!
        case XK_KP_Insert: return GLSG_KEY_KP_0;
        case XK_KP_End: return GLSG_KEY_KP_1;
        case XK_KP_Down: return GLSG_KEY_KP_2;
        case XK_KP_Page_Down: return GLSG_KEY_KP_3;
        case XK_KP_Left: return GLSG_KEY_KP_4;
        case XK_KP_Right: return GLSG_KEY_KP_6;
        case XK_KP_Home: return GLSG_KEY_KP_7;
        case XK_KP_Up: return GLSG_KEY_KP_8;
        case XK_KP_Page_Up: return GLSG_KEY_KP_9;
        case XK_KP_Delete: return GLSG_KEY_KP_DECIMAL;
        case XK_KP_Equal: return GLSG_KEY_KP_EQUAL;
        case XK_KP_Enter:
            return GLSG_KEY_KP_ENTER;

            // Last resort: Check for printable keys (should not happen if the
            // XKB extension is available). This will give a layout dependent
            // mapping (which is wrong, and we may miss some keys, especially on
            // non-US keyboards), but it's better than nothing...
        case XK_a: return GLSG_KEY_A;
        case XK_b: return GLSG_KEY_B;
        case XK_c: return GLSG_KEY_C;
        case XK_d: return GLSG_KEY_D;
        case XK_e: return GLSG_KEY_E;
        case XK_f: return GLSG_KEY_F;
        case XK_g: return GLSG_KEY_G;
        case XK_h: return GLSG_KEY_H;
        case XK_i: return GLSG_KEY_I;
        case XK_j: return GLSG_KEY_J;
        case XK_k: return GLSG_KEY_K;
        case XK_l: return GLSG_KEY_L;
        case XK_m: return GLSG_KEY_M;
        case XK_n: return GLSG_KEY_N;
        case XK_o: return GLSG_KEY_O;
        case XK_p: return GLSG_KEY_P;
        case XK_q: return GLSG_KEY_Q;
        case XK_r: return GLSG_KEY_R;
        case XK_s: return GLSG_KEY_S;
        case XK_t: return GLSG_KEY_T;
        case XK_u: return GLSG_KEY_U;
        case XK_v: return GLSG_KEY_V;
        case XK_w: return GLSG_KEY_W;
        case XK_x: return GLSG_KEY_X;
        case XK_y: return GLSG_KEY_Y;
        case XK_z: return GLSG_KEY_Z;
        case XK_1: return GLSG_KEY_1;
        case XK_2: return GLSG_KEY_2;
        case XK_3: return GLSG_KEY_3;
        case XK_4: return GLSG_KEY_4;
        case XK_5: return GLSG_KEY_5;
        case XK_6: return GLSG_KEY_6;
        case XK_7: return GLSG_KEY_7;
        case XK_8: return GLSG_KEY_8;
        case XK_9: return GLSG_KEY_9;
        case XK_0: return GLSG_KEY_0;
        case XK_space: return GLSG_KEY_SPACE;
        case XK_minus: return GLSG_KEY_MINUS;
        case XK_equal: return GLSG_KEY_EQUAL;
        case XK_bracketleft: return GLSG_KEY_LEFT_BRACKET;
        case XK_bracketright: return GLSG_KEY_RIGHT_BRACKET;
        case XK_backslash: return GLSG_KEY_BACKSLASH;
        case XK_semicolon: return GLSG_KEY_SEMICOLON;
        case XK_apostrophe: return GLSG_KEY_APOSTROPHE;
        case XK_grave: return GLSG_KEY_GRAVE_ACCENT;
        case XK_comma: return GLSG_KEY_COMMA;
        case XK_period: return GLSG_KEY_PERIOD;
        case XK_slash: return GLSG_KEY_SLASH;
        case XK_less: return GLSG_KEY_WORLD_1;  // At least in some layouts...
        default: break;
    }

    // No matching translation was found
    return GLSG_KEY_UNKNOWN;
}

// Translates an X event modifier state mask
int X11Window::translateState(int state) {
    int mods = 0;

    if (state & ShiftMask) mods |= GLSG_MOD_SHIFT;
    if (state & ControlMask) mods |= GLSG_MOD_CONTROL;
    if (state & Mod1Mask) mods |= GLSG_MOD_ALT;
    if (state & Mod4Mask) mods |= GLSG_MOD_SUPER;
    if (state & LockMask) mods |= GLSG_MOD_CAPS_LOCK;
    if (state & Mod2Mask) mods |= GLSG_MOD_NUM_LOCK;

    return mods;
}

// Create key code translation tables
void X11Window::createKeyTables() {
    int scancode, scancodeMin, scancodeMax;

    memset(m_keycodes, -1, sizeof(m_keycodes));
    memset(m_scancodes, -1, sizeof(m_scancodes));
    m_xkb.major     = 1;
    m_xkb.minor     = 0;
    m_xkb.available = XkbQueryExtension(m_display, &m_xkb.majorOpcode, &m_xkb.eventBase, &m_xkb.errorBase, &m_xkb.major,
                                        &m_xkb.minor);

    if (m_xkb.available) {
        // Use XKB to determine physical key locations independently of the
        // current keyboard layout

        XkbDescPtr desc = XkbGetMap(m_display, 0, XkbUseCoreKbd);
        XkbGetNames(m_display, XkbKeyNamesMask | XkbKeyAliasesMask, desc);

        scancodeMin = desc->min_key_code;
        scancodeMax = desc->max_key_code;

        const struct {
            int   key;
            char *name;
        } keymap[] = {{GLSG_KEY_GRAVE_ACCENT, (char *)"TLDE"},
                      {GLSG_KEY_1, (char *)"AE01"},
                      {GLSG_KEY_2, (char *)"AE02"},
                      {GLSG_KEY_3, (char *)"AE03"},
                      {GLSG_KEY_4, (char *)"AE04"},
                      {GLSG_KEY_5, (char *)"AE05"},
                      {GLSG_KEY_6, (char *)"AE06"},
                      {GLSG_KEY_7, (char *)"AE07"},
                      {GLSG_KEY_8, (char *)"AE08"},
                      {GLSG_KEY_9, (char *)"AE09"},
                      {GLSG_KEY_0, (char *)"AE10"},
                      {GLSG_KEY_MINUS, (char *)"AE11"},
                      {GLSG_KEY_EQUAL, (char *)"AE12"},
                      {GLSG_KEY_Q, (char *)"AD01"},
                      {GLSG_KEY_W, (char *)"AD02"},
                      {GLSG_KEY_E, (char *)"AD03"},
                      {GLSG_KEY_R, (char *)"AD04"},
                      {GLSG_KEY_T, (char *)"AD05"},
                      {GLSG_KEY_Y, (char *)"AD06"},
                      {GLSG_KEY_U, (char *)"AD07"},
                      {GLSG_KEY_I, (char *)"AD08"},
                      {GLSG_KEY_O, (char *)"AD09"},
                      {GLSG_KEY_P, (char *)"AD10"},
                      {GLSG_KEY_LEFT_BRACKET, (char *)"AD11"},
                      {GLSG_KEY_RIGHT_BRACKET, (char *)"AD12"},
                      {GLSG_KEY_A, (char *)"AC01"},
                      {GLSG_KEY_S, (char *)"AC02"},
                      {GLSG_KEY_D, (char *)"AC03"},
                      {GLSG_KEY_F, (char *)"AC04"},
                      {GLSG_KEY_G, (char *)"AC05"},
                      {GLSG_KEY_H, (char *)"AC06"},
                      {GLSG_KEY_J, (char *)"AC07"},
                      {GLSG_KEY_K, (char *)"AC08"},
                      {GLSG_KEY_L, (char *)"AC09"},
                      {GLSG_KEY_SEMICOLON, (char *)"AC10"},
                      {GLSG_KEY_APOSTROPHE, (char *)"AC11"},
                      {GLSG_KEY_Z, (char *)"AB01"},
                      {GLSG_KEY_X, (char *)"AB02"},
                      {GLSG_KEY_C, (char *)"AB03"},
                      {GLSG_KEY_V, (char *)"AB04"},
                      {GLSG_KEY_B, (char *)"AB05"},
                      {GLSG_KEY_N, (char *)"AB06"},
                      {GLSG_KEY_M, (char *)"AB07"},
                      {GLSG_KEY_COMMA, (char *)"AB08"},
                      {GLSG_KEY_PERIOD, (char *)"AB09"},
                      {GLSG_KEY_SLASH, (char *)"AB10"},
                      {GLSG_KEY_BACKSLASH, (char *)"BKSL"},
                      {GLSG_KEY_WORLD_1, (char *)"LSGT"},
                      {GLSG_KEY_SPACE, (char *)"SPCE"},
                      {GLSG_KEY_ESCAPE, (char *)"ESC"},
                      {GLSG_KEY_ENTER, (char *)"RTRN"},
                      {GLSG_KEY_TAB, (char *)"TAB"},
                      {GLSG_KEY_BACKSPACE, (char *)"BKSP"},
                      {GLSG_KEY_INSERT, (char *)"INS"},
                      {GLSG_KEY_DELETE, (char *)"DELE"},
                      {GLSG_KEY_RIGHT, (char *)"RGHT"},
                      {GLSG_KEY_LEFT, (char *)"LEFT"},
                      {GLSG_KEY_DOWN, (char *)"DOWN"},
                      {GLSG_KEY_UP, (char *)"UP"},
                      {GLSG_KEY_PAGE_UP, (char *)"PGUP"},
                      {GLSG_KEY_PAGE_DOWN, (char *)"PGDN"},
                      {GLSG_KEY_HOME, (char *)"HOME"},
                      {GLSG_KEY_END, (char *)"END"},
                      {GLSG_KEY_CAPS_LOCK, (char *)"CAPS"},
                      {GLSG_KEY_SCROLL_LOCK, (char *)"SCLK"},
                      {GLSG_KEY_NUM_LOCK, (char *)"NMLK"},
                      {GLSG_KEY_PRINT_SCREEN, (char *)"PRSC"},
                      {GLSG_KEY_PAUSE, (char *)"PAUS"},
                      {GLSG_KEY_F1, (char *)"FK01"},
                      {GLSG_KEY_F2, (char *)"FK02"},
                      {GLSG_KEY_F3, (char *)"FK03"},
                      {GLSG_KEY_F4, (char *)"FK04"},
                      {GLSG_KEY_F5, (char *)"FK05"},
                      {GLSG_KEY_F6, (char *)"FK06"},
                      {GLSG_KEY_F7, (char *)"FK07"},
                      {GLSG_KEY_F8, (char *)"FK08"},
                      {GLSG_KEY_F9, (char *)"FK09"},
                      {GLSG_KEY_F10, (char *)"FK10"},
                      {GLSG_KEY_F11, (char *)"FK11"},
                      {GLSG_KEY_F12, (char *)"FK12"},
                      {GLSG_KEY_F13, (char *)"FK13"},
                      {GLSG_KEY_F14, (char *)"FK14"},
                      {GLSG_KEY_F15, (char *)"FK15"},
                      {GLSG_KEY_F16, (char *)"FK16"},
                      {GLSG_KEY_F17, (char *)"FK17"},
                      {GLSG_KEY_F18, (char *)"FK18"},
                      {GLSG_KEY_F19, (char *)"FK19"},
                      {GLSG_KEY_F20, (char *)"FK20"},
                      {GLSG_KEY_F21, (char *)"FK21"},
                      {GLSG_KEY_F22, (char *)"FK22"},
                      {GLSG_KEY_F23, (char *)"FK23"},
                      {GLSG_KEY_F24, (char *)"FK24"},
                      {GLSG_KEY_F25, (char *)"FK25"},
                      {GLSG_KEY_KP_0, (char *)"KP0"},
                      {GLSG_KEY_KP_1, (char *)"KP1"},
                      {GLSG_KEY_KP_2, (char *)"KP2"},
                      {GLSG_KEY_KP_3, (char *)"KP3"},
                      {GLSG_KEY_KP_4, (char *)"KP4"},
                      {GLSG_KEY_KP_5, (char *)"KP5"},
                      {GLSG_KEY_KP_6, (char *)"KP6"},
                      {GLSG_KEY_KP_7, (char *)"KP7"},
                      {GLSG_KEY_KP_8, (char *)"KP8"},
                      {GLSG_KEY_KP_9, (char *)"KP9"},
                      {GLSG_KEY_KP_DECIMAL, (char *)"KPDL"},
                      {GLSG_KEY_KP_DIVIDE, (char *)"KPDV"},
                      {GLSG_KEY_KP_MULTIPLY, (char *)"KPMU"},
                      {GLSG_KEY_KP_SUBTRACT, (char *)"KPSU"},
                      {GLSG_KEY_KP_ADD, (char *)"KPAD"},
                      {GLSG_KEY_KP_ENTER, (char *)"KPEN"},
                      {GLSG_KEY_KP_EQUAL, (char *)"KPEQ"},
                      {GLSG_KEY_LEFT_SHIFT, (char *)"LFSH"},
                      {GLSG_KEY_LEFT_CONTROL, (char *)"LCTL"},
                      {GLSG_KEY_LEFT_ALT, (char *)"LALT"},
                      {GLSG_KEY_LEFT_SUPER, (char *)"LWIN"},
                      {GLSG_KEY_RIGHT_SHIFT, (char *)"RTSH"},
                      {GLSG_KEY_RIGHT_CONTROL, (char *)"RCTL"},
                      {GLSG_KEY_RIGHT_ALT, (char *)"RALT"},
                      {GLSG_KEY_RIGHT_ALT, (char *)"LVL3"},
                      {GLSG_KEY_RIGHT_ALT, (char *)"MDSW"},
                      {GLSG_KEY_RIGHT_SUPER, (char *)"RWIN"},
                      {GLSG_KEY_MENU, (char *)"MENU"}};

        // Find the X11 key code -> GLFW key code mapping
        for (scancode = scancodeMin; scancode <= scancodeMax; scancode++) {
            int key = GLSG_KEY_UNKNOWN;

            // Map the key name to a GLFW key code. Note: We use the US
            // keyboard layout. Because function keys aren't mapped correctly
            // when using traditional KeySym translations, they are mapped
            // here instead.
            for (int i = 0; i < sizeof(keymap) / sizeof(keymap[0]); i++) {
                if (strncmp(desc->names->keys[scancode].name, keymap[i].name, XkbKeyNameLength) == 0) {
                    key = keymap[i].key;
                    break;
                }
            }

            // Fall back to key aliases in case the key name did not match
            for (int i = 0; i < desc->names->num_key_aliases; i++) {
                if (key != GLSG_KEY_UNKNOWN) break;

                if (strncmp(desc->names->key_aliases[i].real, desc->names->keys[scancode].name, XkbKeyNameLength) !=
                    0) {
                    continue;
                }

                for (int j = 0; j < sizeof(keymap) / sizeof(keymap[0]); j++) {
                    if (strncmp(desc->names->key_aliases[i].alias, keymap[j].name, XkbKeyNameLength) == 0) {
                        key = keymap[j].key;
                        break;
                    }
                }
            }

            m_keycodes[scancode] = key;
        }

        XkbFreeNames(desc, XkbKeyNamesMask, True);
        XkbFreeKeyboard(desc, 0, True);
    } else
        XDisplayKeycodes(m_display, &scancodeMin, &scancodeMax);

    int     width;
    KeySym *keysyms = XGetKeyboardMapping(m_display, scancodeMin, scancodeMax - scancodeMin + 1, &width);

    for (scancode = scancodeMin; scancode <= scancodeMax; scancode++) {
        // Translate the un-translated key codes using traditional X11 KeySym
        // lookups
        if (m_keycodes[scancode] < 0) {
            const size_t base    = (scancode - scancodeMin) * width;
            m_keycodes[scancode] = translateKeySyms(&keysyms[base], width);
        }

        // Store the reverse translation for faster key name lookup
        if (m_keycodes[scancode] > 0) m_scancodes[m_keycodes[scancode]] = scancode;
    }

    XFree(keysyms);
}

// Decode a Unicode code point from a UTF-8 stream
// Based on cutef8 by Jeff Bezanson (Public Domain)
#if defined(X_HAVE_UTF8_STRING)
unsigned int X11Window::decodeUTF8(const char **s) {
    unsigned int       ch = 0, count = 0;
    const unsigned int offsets[] = {0x00000000u, 0x00003080u, 0x000e2080u, 0x03c82080u, 0xfa082080u, 0x82082080u};

    do {
        ch = (ch << 6) + (unsigned char)**s;
        (*s)++;
        count++;
    } while ((**s & 0xc0) == 0x80);

    assert(count <= 6);
    return ch - offsets[count - 1];
}
#endif /*X_HAVE_UTF8_STRING*/

// Convert XKB KeySym to Unicode
long X11Window::keySym2Unicode(unsigned int keysym) {
    int min = 0;
    int max = sizeof(keysymtab) / sizeof(struct codepair) - 1;
    int mid;

    // First check for Latin-1 characters (1:1 mapping)
    if ((keysym >= 0x0020 && keysym <= 0x007e) || (keysym >= 0x00a0 && keysym <= 0x00ff)) {
        return keysym;
    }

    // Also check for directly encoded 24-bit UCS characters
    if ((keysym & 0xff000000) == 0x01000000) return keysym & 0x00ffffff;

    // Binary search in table
    while (max >= min) {
        mid = (min + max) / 2;
        if (keysymtab[mid].keysym < keysym)
            min = mid + 1;
        else if (keysymtab[mid].keysym > keysym)
            max = mid - 1;
        else
            return keysymtab[mid].ucs;
    }

    // No matching Unicode value found
    return -1;
}

void X11Window::grabErrorHandler() {
    m_errorCode = Success;
    XSetErrorHandler(errorHandler);
}

void X11Window::releaseErrorHandler() {
    // Synchronize to make sure all commands are processed
    XSync(m_display, false);
    XSetErrorHandler(nullptr);
}

int X11Window::errorHandler(Display *display, XErrorEvent *event) {
    //    if (m_display != display)
    //        return 0;

    m_errorCode = event->error_code;
    return 0;
}

// Retrieve a single window property of the specified type
// Inspired by fghGetWindowProperty from freeglut
unsigned long X11Window::getWindowProperty(Window window, Atom property, Atom type, unsigned char **value) {
    Atom          actualType;
    int           actualFormat;
    unsigned long itemCount, bytesAfter;

    XGetWindowProperty(m_display, window, property, 0, LONG_MAX, False, type, &actualType, &actualFormat, &itemCount,
                       &bytesAfter, value);

    return itemCount;
}

// Splits and translates a text/uri-list into separate file paths
// NOTE: This function destroys the provided string
char **X11Window::parseUriList(char *text, int *count) {
    std::string prefix = "file://";
    char      **paths  = NULL;
    char       *line;

    *count = 0;

    while ((line = strtok(text, "\r\n"))) {
        text = NULL;

        if (line[0] == '#') {
            continue;
        }

        if (std::string(line).substr(0, prefix.length()) == prefix) {
            line += prefix.length;
            while (*line != '/') {
                ++line;
            }
        }

        ++(*count);

        char *path        = (char *)calloc(strlen(line) + 1, 1);
        paths             = (char **)realloc(paths, *count * sizeof(char *));
        paths[*count - 1] = path;

        while (*line) {
            if (line[0] == '%' && line[1] && line[2]) {
                const char digits[3] = {line[1], line[2], '\0'};
                *path                = strtol(digits, NULL, 16);
                line += 2;
            } else {
                *path = *line;
            }

            ++path;
            ++line;
        }
    }

    return paths;
}

// Exit disabled cursor mode for the specified window
void X11Window::enableCursor() {
    if (m_rawMouseMotion) disableRawMouseMotion();

    m_disabledCursorWindow = nullptr;
    XUngrabPointer(m_display, CurrentTime);
    onMouseCursor(m_restoreCursorPosX, m_restoreCursorPosY);
    // updateCursorImage(window);
}

// Apply disabled cursor mode to a focused window
void X11Window::disableCursor() {
    if (m_rawMouseMotion) enableRawMouseMotion();

    //    m_disabledCursorWindow = this;
    //    _glfwPlatformGetCursorPos(window,
    //                              m_restoreCursorPosX,
    //                              m_restoreCursorPosY);
    //    //updateCursorImage();
    //    //_glfwCenterCursorInContentArea(window);
    //    XGrabPointer(m_display, m_win, True,
    //                 ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
    //                 GrabModeAsync, GrabModeAsync,
    //                 m_win,
    //                 m_hiddenCursorHandle,
    //                 CurrentTime);
}

// Enable XI2 raw mouse motion events
void X11Window::enableRawMouseMotion() {
    XIEventMask   em;
    unsigned char mask[XIMaskLen(XI_RawMotion)] = {0};

    em.deviceid = XIAllMasterDevices;
    em.mask_len = sizeof(mask);
    em.mask     = mask;
    XISetMask(mask, XI_RawMotion);

    XISelectEvents(m_display, m_root, &em, 1);
}

// Returns whether the window is iconified
int X11Window::getWindowState() {
    int result = WithdrawnState;
    struct {
        CARD32 state;
        Window icon;
    } *state = NULL;

    if (getWindowProperty(m_win, m_WM_STATE, m_WM_STATE, (unsigned char **)&state) >= 2) {
        result = state->state;
    }

    if (state) XFree(state);

    return result;
}

// Disable XI2 raw mouse motion events
void X11Window::disableRawMouseMotion() {
    XIEventMask   em;
    unsigned char mask[] = {0};

    em.deviceid = XIAllMasterDevices;
    em.mask_len = sizeof(mask);
    em.mask     = mask;

    XISelectEvents(m_display, m_root, &em, 1);
}

void X11Window::close() {
    if (m_im) {
        XCloseIM(m_im);
        m_im = nullptr;
    }

    if (m_display) {
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
}

X11Window::~X11Window() {}
}  // namespace ara

#endif