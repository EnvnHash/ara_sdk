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


#if defined(__linux__) && !defined(__ANDROID__)

#pragma once

#include <X11/Xlib.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
// #include <X11/Xmd.h>
#include <WindowManagement/GLWindowBase.h>
#include <WindowManagement/X11Window_unicode.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/XI2.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBstr.h>
#include <GlbCommon/GlbCommon.h>

#define _GLSG_XDND_VERSION 5
#define CARD32 int32_t

namespace ara {

class xkb {
public:
    bool         available   = false;
    bool         detectable  = false;
    int          majorOpcode = 0;
    int          eventBase   = 0;
    int          errorBase   = 0;
    int          major       = 0;
    int          minor       = 0;
    unsigned int group       = 0;
};

class xdnd {
public:
    int    version;
    Window source;
    Atom   m_format;
};

class X11Window : public GLWindowBase {
public:
    X11Window() : GLWindowBase() {}

    ~X11Window() override;

    bool        create(const glWinPar &gp) override;
    void        open() override {}
    void        close() override;
    void        swap() override {}
    void        makeCurrent() override {}
    void        setVSync(bool val) override {}
    void        destroy() override {}
    void        resize(GLsizei width, GLsizei height) override {}
    void       *getNativeCtx() override { return nullptr; }
    void        onWindowSize(int width, int height) override {}
    void        minimize() override {}
    void        restore() override {}
    static bool isMinimized() { return false; }

    void initAtoms();
    bool hasUsableInputMethodStyle();
    void waitEvents();
    bool waitForEvent(double *timeout);
    void pollEvents();
    Atom writeTargetToProperty(const XSelectionRequestEvent *request);
    void handleSelectionClear(XEvent *event);
    void handleSelectionRequest(XEvent *event);
    int  translateKey(int scancode);
    int  translateState(int state);
    int  translateKeySyms(const KeySym *keysyms, int width);
    void createKeyTables();

#if defined(X_HAVE_UTF8_STRING)
    unsigned int decodeUTF8(const char **s);
#endif

    long keySym2Unicode(unsigned int keysym);
    void grabErrorHandler();
    void releaseErrorHandler();

    static int errorHandler(Display *display, XErrorEvent *event);

    unsigned long getWindowProperty(Window window, Atom property, Atom type, unsigned char **value);

    char **parseUriList(char *text, int *count);

    void enableCursor();
    void disableCursor();
    void enableRawMouseMotion();
    void disableRawMouseMotion();
    int  getWindowState();

    void *getWin() override { return &m_win; }
    void *getDisp() override { return m_display; }
    Atom &getWMDelMsg() { return s_wmDeleteMessage; }
    int  *getWorkArea() { return m_workArea; }

protected:
    Display *m_display = nullptr;
    Window   m_win;
    Window   m_root;
    Window   m_parent;
    Colormap m_colormap;

    Atom s_wmDeleteMessage{};
    Atom m_WM_PROTOCOLS{};
    Atom m_WM_STATE{};
    Atom m_WM_DELETE_WINDOW{};
    Atom m_NET_SUPPORTED{};
    Atom m_NET_SUPPORTING_WM_CHECK{};
    Atom m_NET_WM_ICON{};
    Atom m_NET_WM_PING{};
    Atom m_NET_WM_PID{};
    Atom m_NET_WM_NAME{};
    Atom m_NET_WM_ICON_NAME{};
    Atom m_NET_WM_BYPASS_COMPOSITOR{};
    Atom m_NET_WM_WINDOW_OPACITY{};
    Atom m_MOTIF_WM_HINTS{};

    // Xdnd (drag and drop) atoms
    Atom m_XdndAware{};
    Atom m_XdndEnter{};
    Atom m_XdndPosition{};
    Atom m_XdndStatus{};
    Atom m_XdndActionCopy{};
    Atom m_XdndDrop{};
    Atom m_XdndFinished{};
    Atom m_XdndSelection{};
    Atom m_XdndTypeList{};
    Atom m_text_uri_list{};

    // Selection (clipboard) atoms
    Atom m_TARGETS{};
    Atom m_MULTIPLE{};
    Atom m_INCR{};
    Atom m_CLIPBOARD{};
    Atom m_PRIMARY{};
    Atom m_CLIPBOARD_MANAGER{};
    Atom m_SAVE_TARGETS{};
    Atom m_NULL_{};
    Atom m_UTF8_STRING{};
    Atom m_COMPOUND_STRING{};
    Atom m_ATOM_PAIR{};
    Atom m_GLFW_SELECTION{};

    bool              m_autoIconify       = false;
    bool              m_maximized         = false;
    bool              m_iconified         = false;
    double            m_virtualCursorPosX = 0.0;
    double            m_virtualCursorPosY = 0.0;
    int               m_warpCursorPosX    = 0;
    int               m_warpCursorPosY    = 0;
    inline static int m_errorCode         = 0;
    int               m_workArea[4]       = {0};

    XIM       m_im{};  // XIM input method
    XIC       m_ic{};
    short int m_keycodes[256];
    Time      m_keyPressTimes[256];  // The time of the last KeyPress event per
                                     // keycode, for discarding  duplicate key events
                                     // generated for some keys by ibus
    xkb  m_xkb{};
    xdnd m_xdnd{};

    char *m_clipboardString        = nullptr;
    char *m_primarySelectionString = nullptr;

    X11Window *m_disabledCursorWindow = nullptr;
};

}  // namespace ara

#endif