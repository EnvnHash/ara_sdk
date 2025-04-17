//
// Created by user on 23.09.2020.
//

#pragma once

#include <Conditional.h>
#include <WindowManagement/GLWindowCommon.h>

namespace ara {

class GLWindowBase {
public:
    GLWindowBase() = default;
    virtual ~GLWindowBase() {}
    virtual bool  create(glWinPar &gp)                  = 0;
    virtual void  open()                                = 0;
    virtual void  close()                               = 0;
    virtual void  swap()                                = 0;
    virtual void  makeCurrent() {};
    virtual void  setVSync(bool val)                    = 0;
    virtual void  destroy()                             = 0;
    virtual void  minimize()                            = 0;
    virtual void  restore()                             = 0;
    virtual void  resize(GLsizei width, GLsizei height) = 0;
    virtual void *getNativeCtx()                        = 0;

    void iterate() { m_iterate.notify(); }
    void forceRedraw() { m_forceRedraw = true; }
    void setDrawFunc(const std::function<bool(double, double, int)>& f) { m_drawFunc = f; }
    void setGlCb(const std::function<void()>& cb) {
        m_glCb = cb;
        m_iterate.notify();
    }

    /** called when a key is pressed or released. Is called directly from GLFW.
     * When the window has been created through GLFWWindowManager, this is
     * called from there. */
    void onKey(int key, int scancode, int action, int mods) {
        if (!m_hidBlocked) {
            for (auto &it : m_keyCb) {
                it(key, scancode, action, mods);
            }
        }
    }

    void onChar(unsigned int codepoint) {
        if (!m_hidBlocked && m_charCb) {
            m_charCb(codepoint);
        }
    }

    void onMouseButton(int button, int action, int mods) {
        if (!m_hidBlocked && m_mouseButtonCb) {
            m_mouseButtonCb(button, action, mods);
        }
    }

    void onMouseCursor(double xpos, double ypos) {
        if (!m_hidBlocked && m_mouseCursorCb) {
            m_mouseCursorCb(xpos, ypos);
        }
    }

    virtual void onWindowSize(int width, int height) = 0;
    virtual void onFrameBufferSize(int width, int height) {}

    void onScroll(double xpos, double ypos) {
        if (!m_hidBlocked && m_scrollCb) {
            m_scrollCb(xpos, ypos);
        }
    }
    void onWindowPos(int xpos, int ypos) {
        if (m_windowPosCb) {
            m_windowPosCb(xpos, ypos);
        }
    }
    void onWindowMaximize(int flag) {
        if (m_windowMaximizeCb) {
            m_windowMaximizeCb(flag);
        }
    }
    void onWindowIconify(int flag) {
        if (m_windowIconfiyCb) {
            m_windowIconfiyCb(flag);
        }
    }
    void onWindowFocus(int flag) {
        if (m_windowFocusCb) {
            m_windowFocusCb(flag);
        }
    }
    void onWindowClose() {
        if (m_closeCb) {
            m_closeCb();
        }
    }
    void onWindowRefresh() {
        if (m_windowRefreshCb) {
            m_windowRefreshCb();
        }
    }
    void onMouseEnter(bool val) {}
    void onDrop(int count, const char **paths) {}

    /** setters for HID callbacks */
    void addKeyCb(const std::function<void(int, int, int, int)>& f) { m_keyCb.push_back(f); }
    void setCharCb(const std::function<void(int)>& f) { m_charCb = f; }
    void setMouseButtonCb(const std::function<void(int, int, int)>& f) { m_mouseButtonCb = f; }
    void setMouseCursorCb(const std::function<void(double, double)>& f) { m_mouseCursorCb = f; }
    void setWindowSizeCb(const std::function<void(int, int)>& f) { m_windowSizeCb = f; }
    void setScrollCb(const std::function<void(double, double)>& f) { m_scrollCb = f; }
    void setWindowPosCb(const std::function<void(int, int)>& f) { m_windowPosCb = f; }
    void setWindowMaximizeCb(const std::function<void(int)>& f) { m_windowMaximizeCb = f; }
    void setWindowIconfifyCb(const std::function<void(int)>& f) { m_windowIconfiyCb = f; }
    void setWindowFocusCb(const std::function<void(int)>& f) { m_windowFocusCb = f; }
    void setCloseCb(const std::function<void()>& f) { m_closeCb = f; }
    void setWindowRefreshCb(const std::function<void()>& f) { m_windowRefreshCb = f; }
    std::function<bool(double, double, int)> &getDrawFunc() { return m_drawFunc; }

int                  getKeyScancode(int key) { return m_scancodes[key]; }
    uint32_t         getWidth() { return m_widthVirt; }
    uint32_t         getHeight() { return m_heightVirt; }
    uint32_t         getWidthReal() { return m_widthReal; }
    uint32_t         getHeightReal() { return m_heightReal; }
    uint32_t         getPosX() { return m_offsX; }
    uint32_t         getPosY() { return m_offsY; }
    glm::ivec2       getSize() { return {m_widthVirt, m_heightVirt}; }
    glm::ivec2       getPosition() { return {m_offsX, m_offsY}; }
    Conditional     *getInitSema() { return &m_initSema; }
    Conditional     *getExitSema() { return &m_exitSema; }
    Conditional     *getGlInitedSema() { return &m_glInitedSema; }
    virtual void    *getWin() { return nullptr; }
    virtual void    *getDisp() { return nullptr; }
    glm::ivec2       getLastMousePos() { return {m_lastCursorPosX, m_lastCursorPosY}; };
    int             *getWorkArea() { return nullptr; }
    glm::vec2       &getContentScale() { return m_contentScale; }
    bool             isInited() { return m_initSignaled; }
    bool             isOpen() { return m_isOpen; }
    bool             isRunning() { return m_run; }
    bool             getRequestOpen() { return m_requestOpen; }
    bool             getRequestClose() { return m_requestClose; }
    void             requestOpen(bool val) { m_requestOpen = val; }
    void             requestClose(bool val) { m_requestClose = val; }

protected:
    bool m_active;               // Window Active Flag Set To TRUE By Default
    bool m_fullscreen  = false;  // Fullscreen Flag Set To Fullscreen Mode By Default
    bool m_isOpen      = false;
    bool m_run         = false;
    bool m_done        = false;
    bool m_transparent = false;
    bool m_decorated   = false;
    bool m_resizable   = true;
    bool m_cursorTracked;
    bool m_frameAction;
    bool m_iconified;
    bool m_maximized;
    bool m_scaleToMonitor = false;
    bool m_shouldClose    = false;
    bool m_stickyKeys;
    bool m_stickyMouseButtons;
    bool m_lockKeyMods;
    bool m_rawMouseMotion = false;
    bool m_hidBlocked     = false;
    bool m_requestOpen    = false;
    bool m_requestClose   = false;
    bool m_eventBasedLoop = false;
    bool m_inited         = false;
    bool m_forceRedraw    = false;
    bool m_initSignaled   = false;

    // The last received cursor position, regardless of source
    int m_lastCursorPosX = 0;
    int m_lastCursorPosY = 0;
    int m_cursorMode     = 0;
    int m_numer          = 0;
    int m_denom          = 0;
    int m_minwidth = 0, m_minheight = 0;
    int m_maxwidth = 0, m_maxheight = 0;

    double m_restoreCursorPosX = 0;
    double m_restoreCursorPosY = 0;
    double m_virtualCursorPosX = 0;
    double m_virtualCursorPosY = 0;

    glm::vec2 m_contentScale = glm::vec2{1.f, 1.f};

    uint32_t m_widthVirt  = 0;  /// in virtual pixels
    uint32_t m_heightVirt = 0;  /// in virtual pixels
    uint32_t m_widthReal  = 0;  /// in real pixels
    uint32_t m_heightReal = 0;  /// in real pixels
    uint32_t m_offsX      = 0;
    uint32_t m_offsY      = 0;

    std::thread m_msgLoop;  // Windows Message Structure
    std::thread m_drawThread;

    WindowCallbacks m_callbacks;
    short int       m_scancodes[GLSG_KEY_LAST + 1];
    short int       m_keycodes[512];
    char            m_keys[GLSG_KEY_LAST + 1];
    char            m_mouseButtons[GLSG_MOUSE_BUTTON_LAST + 1];
    char            m_keynames[GLSG_KEY_LAST + 1][5];

    std::vector<glVidMode>                   modes;
    std::vector<std::pair<int, int>>         monOffsets;
    std::vector<std::pair<float, float>>     monContScale;
    std::function<bool(double, double, int)> m_drawFunc;
    std::function<void()>                    m_glCb;

    std::list<std::function<void(int, int, int, int)>> m_keyCb;
    std::function<void(unsigned int)>                  m_charCb;
    std::function<void(int, int, int)>                 m_mouseButtonCb;
    std::function<void(double, double)>                m_mouseCursorCb;
    std::function<void(int, int)>                      m_windowSizeCb;
    std::function<void(double, double)>                m_scrollCb;
    std::function<void(int, int)>                      m_windowPosCb;
    std::function<void(int)>                           m_windowMaximizeCb;
    std::function<void(int)>                           m_windowIconfiyCb;
    std::function<void(int)>                           m_windowFocusCb;
    std::function<void()>                              m_closeCb;
    std::function<void()>                              m_windowRefreshCb;

    Conditional m_exitSignal;
    Conditional m_exitSema;
    Conditional m_initSema;
    Conditional m_iterate;
    Conditional m_glInitedSema;
};

}  // namespace ara
