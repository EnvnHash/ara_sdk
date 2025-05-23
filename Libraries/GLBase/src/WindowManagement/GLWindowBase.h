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

#include <Conditional.h>
#include <WindowManagement/GLWindowCommon.h>

namespace ara {

using winHidCb = std::variant<
    std::function<void(unsigned int)>,
    std::function<void(int, int, int, int)>,
    std::function<void(int, int, int)>,
    std::function<void(int, int)>,
    std::function<void(int)>,
    std::function<void(double, double)>,
    std::function<void()>>;

class GLWindowBase {
public:
    GLWindowBase() = default;
    virtual ~GLWindowBase() = default;
    virtual bool  create(const glWinPar &gp)            = 0;
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
    virtual void forceRedraw() { m_forceRedraw = true; }
    void setDrawFunc(const std::function<bool(double, double, int)>& f) { m_drawFunc = f; }
    void setGlCb(const std::function<void()>& cb) {
        m_glCb = cb;
        m_iterate.notify();
    }

    template <typename... Args>
    void visitCallbacks(const winCb& key, const winHidCb& varFunc, const std::function<void()>& optFunc, Args&&... args) {
        std::visit([&] (auto&& func) {
            if constexpr (std::is_invocable_v<std::decay_t<decltype(func)>, Args...>) {
                func(args...);
                if (optFunc) {
                    optFunc();
                }
            } else {
                LOGE << "Error: Argument mismatch for winCb type \"" << winCbNames[key] << "\", num of args: " << sizeof...(args);
            }
        }, varFunc);
    }

    template <typename... Args>
    void parseGlobalHidCb(const winCb& key, Args&&... args) {
        if (m_globalHidCallbacks.contains(key)) {
            for (const auto& [id, varFuncPtr] : m_globalHidCallbacks[key]) {
                visitCallbacks(key, *varFuncPtr.get(), nullptr, args...);
            }
        }
    }

    template <typename... Args>
    void onWinHid(const winCb& key, Args&&... args) {
        if (m_winHidCallbacks.contains(key)) {
            visitCallbacks(key, m_winHidCallbacks[key], [&]{ parseGlobalHidCb(key, args...); }, args...);
        } else {
            LOGE << "Error: No callback found for winCb \"" << winCbNames[key] << "\"";
        }
    }

    virtual void onFrameBufferSize(int width, int height) {}
    static void onMouseEnter(bool val) {}
    static void onDrop(int count, const char **paths) {}

    virtual void setWinCallback(winCb tp, const winHidCb& f) { m_winHidCallbacks.insert_or_assign(tp, f); }
    virtual void addGlobalHidCallback(winCb tp, void* id, const std::shared_ptr<winHidCb>& f) { m_globalHidCallbacks[tp].emplace(id, f); }
    virtual void removeGlobalHidCallback(winCb tp, void* id) {
        if (m_globalHidCallbacks.contains(tp)) {
            std::erase_if(m_globalHidCallbacks[tp], [id](auto &it) { return it.first == id; });
        }
    }

    std::function<bool(double, double, int)> &getDrawFunc() { return m_drawFunc; }

    int                 getKeyScancode(int key)  const { return m_scancodes[key]; }
    virtual uint32_t    getWidth() const { return m_widthVirt; }
    virtual uint32_t    getHeight() const { return m_heightVirt; }
    virtual uint32_t    getWidthReal() const { return m_widthReal; }
    virtual uint32_t    getHeightReal() const { return m_heightReal; }
    virtual uint32_t    getPosX() const { return m_offsX; }
    virtual uint32_t    getPosY() const { return m_offsY; }
    glm::ivec2          getSize() const { return {m_widthVirt, m_heightVirt}; }
    glm::ivec2          getPosition() const { return {m_offsX, m_offsY}; }
    auto                getInitSema() { return &m_initSema; }
    auto                getExitSema() { return &m_exitSema; }
    auto                getGlInitedSema() { return &m_glInitedSema; }
    virtual void*       getWin() { return nullptr; }
    virtual void*       getDisp() { return nullptr; }
    glm::ivec2          getLastMousePos() const { return {m_lastCursorPosX, m_lastCursorPosY}; };
    static int*         getWorkArea() { return nullptr; }
    auto&               getContentScale() { return m_contentScale; }
    auto                isInited() const { return m_initSignaled; }
    virtual bool        isOpen() const { return m_isOpen; }
    auto                isRunning() const { return m_run; }
    virtual bool        getRequestOpen() const { return m_requestOpen; }
    virtual bool        getRequestClose() const { return m_requestClose; }
    virtual bool        getForceRedraw() const { return m_forceRedraw; }
    virtual void        requestOpen(bool val) { m_requestOpen = val; }
    virtual void        requestClose(bool val) { m_requestClose = val; }
    int32_t             virt2RealX(int x) { return static_cast<int>(static_cast<float>(x) * getContentScale().x); }
    int32_t             virt2RealY(int y) { return static_cast<int>(static_cast<float>(y) * getContentScale().y); }

protected:
    std::unordered_map<winCb, winHidCb> m_winHidCallbacks;
    std::unordered_map<winCb, std::unordered_map<void*, std::shared_ptr<winHidCb>>>  m_globalHidCallbacks;

    bool m_active{};               // Window Active Flag Set To TRUE By Default
    bool m_fullscreen           = false;  // Fullscreen Flag Set To Fullscreen Mode By Default
    bool m_isOpen               = false;
    bool m_run                  = false;
    bool m_done                 = false;
    bool m_transparent          = false;
    bool m_decorated            = false;
    bool m_resizable            = true;
    bool m_cursorTracked        = false;
    bool m_frameAction          = false;
    bool m_iconified            = false;
    bool m_maximized            = false;
    bool m_scaleToMonitor       = false;
    bool m_shouldClose          = false;
    bool m_stickyKeys           = false;
    bool m_stickyMouseButtons   = false;
    bool m_lockKeyMods          = false;
    bool m_rawMouseMotion       = false;
    bool m_hidBlocked           = false;
    bool m_requestOpen          = false;
    bool m_requestClose         = false;
    bool m_eventBasedLoop       = false;
    bool m_inited               = false;
    bool m_forceRedraw          = false;
    bool m_initSignaled         = false;

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

    glm::vec2 m_contentScale{1.f, 1.f};

    uint32_t m_widthVirt  = 0;  /// in virtual pixels
    uint32_t m_heightVirt = 0;  /// in virtual pixels
    uint32_t m_widthReal  = 0;  /// in real pixels
    uint32_t m_heightReal = 0;  /// in real pixels
    uint32_t m_offsX      = 0;
    uint32_t m_offsY      = 0;

    std::thread m_msgLoop;  // Windows Message Structure
    std::thread m_drawThread;

    //  WindowCallbacks m_callbacks;
    std::array<short int, GLSG_KEY_LAST + 1>        m_scancodes{-1};
    std::array<short int, 512>                      m_keycodes{-1};
    std::array<char, GLSG_KEY_LAST + 1>             m_keys{};
    std::array<char, GLSG_MOUSE_BUTTON_LAST + 1>    m_mouseButtons{};
    std::array<char, GLSG_KEY_LAST + 1>             m_keynames[5]{};

    std::vector<glVidMode>                   modes;
    std::vector<std::pair<int, int>>         monOffsets;
    std::vector<std::pair<float, float>>     monContScale;
    std::function<bool(double, double, int)> m_drawFunc;
    std::function<void()>                    m_glCb;

    Conditional m_exitSignal;
    Conditional m_exitSema;
    Conditional m_initSema;
    Conditional m_iterate;
    Conditional m_glInitedSema;
};

}  // namespace ara
