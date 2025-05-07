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


#ifdef WGLWINDOW

#pragma once

#ifdef _WIN32

#include <WindowManagement/GLWindow.h>
#include <Windows.h>
#include <dwmapi.h>

// ntdll.dll function pointer typedefs
typedef LONG(WINAPI* PFN_RtlVerifyVersionInfo)(OSVERSIONINFOEXW*, ULONG, ULONGLONG);

#ifndef RtlVerifyVersionInfo
#define RtlVerifyVersionInfo ntdll.RtlVerifyVersionInfo_
#endif

namespace ara {
class WGLWindow : public GLWindow {
public:
    struct {
        HINSTANCE                instance;
        PFN_RtlVerifyVersionInfo RtlVerifyVersionInfo_;
    } ntdll;

    bool create(const glWinPar& wp);
    bool init();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void open() override {
        if (m_hWnd) {
            ShowWindow(m_hWnd, SW_SHOW);
            m_isOpen      = true;
            m_requestOpen = false;
        }
    }
    void close() override {
        if (m_hWnd) {
            ShowWindow(m_hWnd, SW_HIDE);
            m_isOpen       = false;
            m_requestClose = false;
        }
    }
    void swap() override {
        if (m_hDC) {
            SwapBuffers(m_hDC);
        }
    }

    void makeCurrent() override {
        if (!m_hDC || !m_hRC || !wglMakeCurrent(m_hDC, m_hRC)) {
            LOG << "WGLWindow could not make context current m_hDC: " << m_hDC << " m_hRC: " << m_hRC;
        }
    }

    void destroy() override;
    void resize(GLsizei width, GLsizei height) override;
    static void pollEvents() {}
    void applyAspectRatio(int edge, RECT* area) const;
    int getDpi() const;
    bool isLeftOrRightEdge(int edge) const;
    bool isTopOrBottomEdge(int edge) const;
    void enableRawMouseMotion() const;
    static void disableRawMouseMotion();
    void enableCursor();
    void disableCursor();
    void getCursorPos(double* xpos, double* ypos) const;
    void setCursorPos(double xpos, double ypos);
    void centerCursorInContentArea();
    bool cursorInContentArea() const;
    void updateCursorImage() const;
    void updateClipRect() const;
    void updateKeyNamesWin32();
    void updateFramebufferTransparency() const;

    // Notifies shared code of a cursor enter/leave event
    void inputCursorEnter(bool entered) const {
        if (m_callbacks.cursorEnter) {
            m_callbacks.cursorEnter(entered);
        }
    }

    // Notifies shared code of files or directories dropped on a window
    void inputDrop(int count, const char** paths) const {
        if (m_callbacks.drop) {
            m_callbacks.drop(count, paths);
        }
    }

    // Notifies shared code that a window framebuffer has been resized. The size
    // is specified in pixels
    void inputFramebufferSize(int width, int height) const {
        if (m_callbacks.fbsize) {
            m_callbacks.fbsize(width, height);
        }
    }

    // Notifies shared code of a scroll event
    void inputScroll(double xoffset, double yoffset) const {
        if (m_callbacks.scroll) {
            m_callbacks.scroll(xoffset, yoffset);
        }
    }

    // Notifies shared code that a window has been maximized or restored
    void inputWindowMaximize(bool maximized) const {
        if (m_callbacks.maximize) {
            m_callbacks.maximize(maximized);
        }
    }

    // Notifies shared code that a window content scale has changed
    // The scale is specified as the ratio between the current and default DPI
    void inputWindowContentScale(float xscale, float yscale) const {
        if (m_callbacks.scale) {
            m_callbacks.scale(xscale, yscale);
        }
    }

    // Notifies shared code that the user wishes to close a window
    void inputWindowCloseRequest() {
        m_shouldClose = true;
        if (m_callbacks.close) {
            m_callbacks.close();
        }
    }

    // Notifies shared code that the window contents needs updating
    void inputWindowDamage() const {
        if (m_callbacks.refresh) {
            m_callbacks.refresh();
        }
    }

    // Notifies shared code that a window has been iconified or restored
    void inputWindowIconify(bool iconified) const {
        if (m_callbacks.iconify) {
            m_callbacks.iconify(iconified);
        }
    }

    // Notifies shared code that a window has moved. The position is specified
    // in content area relative screen coordinates
    void inputWindowPos(int x, int y) const {
        if (m_callbacks.pos) {
            m_callbacks.pos(x, y);
        }
    }

    // Notifies shared code that a window has been resized. The size is
    // specified in screen coordinates
    void inputWindowSize(int width, int height) const {
        if (m_callbacks.size) {
            m_callbacks.size(width, height);
        }
    }

    void  inputWindowFocus(bool focused);
    void  inputKey(int key, int scancode, int action, int mods);
    void  inputMouseClick(int button, int action, int mods);
    void  inputChar(unsigned int codepoint, int mods, bool plain) const;
    void  inputCursorPos(double xpos, double ypos);
    void  setInputMode(int mode, int value);
    void  setCursorMode(int mode);
    void  setRawMouseMotion(bool enabled) const;
    BOOL  isWindows10BuildOrGreaterWin32(WORD build) const;
    BOOL  isWindowsVersionOrGreaterWin32(WORD major, WORD minor, WORD sp) const;
    static DWORD getWindowExStyle() ;
    DWORD getWindowStyle() const;
    static int   getKeyMods();
    void  getWindowSize(int* width, int* height) const;
    void  getFullWindowSize(DWORD style, DWORD exStyle, int contentWidth, int contentHeight, int* fullWidth,
                            int* fullHeight, UINT dpi) const;
    void  windowFocus(bool focused);
    void  createKeyTables();
    bool  loadLibraries();
    void  freeLibraries() const;

    static bool rawMouseMotionSupported() { return true; }
    int         windowFocused() const { return m_hWnd ? m_hWnd == GetActiveWindow() : 0; }
    HDC         getHdc() const { return m_hDC; }
    HGLRC       getHglrc() const { return m_hRC; }
    HWND        getHwnd() const { return m_hWnd; }
    HINSTANCE   getHInstance() const { return m_hInstance; }
    void*       getNativeCtx() override { return m_hRC; }
    void        setVSync(bool val) override { wglSwapIntervalEXT(static_cast<int>(val)); }

private:
    HDC       m_hDC       = nullptr;  // Private GDI Device Context
    HGLRC     m_hRC       = nullptr;  // Permanent Rendering Context
    HGLRC     m_sharedCtx = nullptr;
    HWND      m_hWnd      = nullptr;  // Holds Our Window Handle
    HINSTANCE m_hInstance = nullptr;  // Holds The Instance Of The Application
    HCURSOR   m_cursorHandle{};
    DWORD     m_dwExStyle{};  // Window Extended Style
    DWORD     m_dwStyle{};    // Window Style
    HICON     m_bigIcon{};
    HICON     m_smallIcon{};
    GLuint    m_pixelFormat{};  // Holds The Results After Searching For A Match
    RAWINPUT* m_rawInput{};
    int       m_rawInputSize{};

    std::string m_thisClassName;  // Windows Message Structure
};
}  // namespace ara

#endif

#endif