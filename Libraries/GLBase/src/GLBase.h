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
#include <Utils/GLStateManager.h>
#include <Utils/TextureCollector.h>
#include <Utils/Typo/TypoGlyphMap.h>
#include <Shaders/ShaderCollector.h>
#include <WindowManagement/WindowManager.h>
#include <Asset/AssetManager.h>

// NOTE: FBOs and VAOs can't be shared since they don't contain actual data

namespace ara {

class Quad;

class GLNativeCtxHnd {
public:
    void    *ctx          = nullptr;
    void    *deviceHandle = nullptr;
    uint32_t drawable     = 0;
    uint32_t readDrawable = 0;
#if defined(__linux__) && !defined(ARA_USE_GLES31)
    GLXContextID ctxIdExt = 0;
#endif
};

class GLCaps {
public:
    GLint max_tex_units            = 0;
    GLint max_tex_size             = 0;
    GLint max_tex_level            = 64;
    GLint max_framebuffer_layers   = 0;
    GLint max_compute_shader_block = 0;
    GLint max_nr_layers            = 0;
    GLint max_nr_attachments       = 0;
    GLint max_nr_samples           = 0;
    GLint max_shader_invoc         = 0;
    GLint multisample              = 0;
    GLint max_nr_drawbuffers       = 0;
    GLint major_vers               = 0;
    GLint minor_vers               = 0;
};

class GLBase {
public:
    GLBase();

    bool init(bool doInitResources = true, void *winHnd = nullptr);
    void createCtx();
    void addContext(void *ctx);
    void destroyCtx();
    void removeContext(void *ctx);
    void shareCtx() const;
    void checkCapabilities();
    void initToThisCtx();
    void destroy(bool terminateGLFW = true);
    static void switchCtx(GLNativeCtxHnd &ctx);

#ifdef ARA_USE_GLFW
    static std::unique_ptr<GLWindow> createOpenGLCtx(bool initGLFW = true);
#endif

    void initAppMsg(const char *fontFile, int fontHeight, int screenWidth, int screenHeight);
    void startGlCallbackProcLoop();
    void stopProcCallbackLoop();
    void addEvtCb(const std::function<bool()> &func, bool forcePush = false) const;
    void addGlCb(const std::function<bool()> &func, Conditional *sema = nullptr);
    void glCallbackLoop();
    void iterateGlCallback();
    void initResources();
    void startContinousCheck();
    void checkResourceChanges() const;
    void clearGlCbQueue();
    void addGlCbSync(const std::function<bool()> &f);
    void setResRootPath(const std::string &str); // relative to application
    GLStateManager &stateMan();
    void makeCurrent();
    Quad *stdQuad();
    bool getUseFallback();
    int32_t maxNrAttachments();
    void setAppMsgStaticInfoNumLines(size_t count);
    static GLNativeCtxHnd getGLCtx();

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    GLWindow *getWin();
#ifdef ARA_USE_GLFW
    GLContext getGlfwHnd();
#else
    GLContext getGlfwHnd();
#endif
    WindowManager *getWinMan() const;
#endif

    void runOnMainThread(const std::function<bool()> &func, bool forcePush = false) { addEvtCb(func, forcePush); }
    void setResFile(const std::string& str) { m_resFile = str; }
    void setShaderHeader(const std::string& hdr) { return m_shaderCollector.setShaderHeader(hdr); }
    void setAppMsgNumLines(const size_t count) { m_appMessagesNumLines = count; }
    void addUpdtResCb(const std::function<void()>& f) { m_updtResCb.emplace_back(f); }
    void useSelfManagedCtx(const bool val) { m_selfManagedCtx = val; }
    void setLoadMouseCursorIcons(const bool val) { m_loadMouseCursorIcons = val; }
    void setContinousChangeCheck(const bool val) { m_continousChangeCheck = val; }

    void    procGlCb() { m_sema.notify(); }
    auto    getNrCtx() const { return m_contexts.size(); }
    auto    &shaderCollector() { return m_shaderCollector; }
    auto    &textureCollector() { return m_textureCollector; }
    auto    *nullVao() { return &m_nullVao; }
    auto    getMainThreadId() const { return m_mainThreadId; }
    auto    rendererIsIntel() const { return m_isIntelRenderer; }
    auto    maxTexUnits() const { return m_caps.max_tex_units; }
    auto    maxShaderInvocations() const { return m_caps.max_shader_invoc; }
    auto    getNrSamples() const { return static_cast<int32_t>(m_nrSamples); }
    auto    maxNrDrawBuffers() const { return m_caps.max_nr_drawbuffers; }
    auto    maxTexMipMapLevels() const { return m_caps.max_tex_level; }
    auto    getMajorVer() const { return m_caps.major_vers; }
    auto    getMinorVer() const { return m_caps.minor_vers; }
    auto    glMtx() { return &m_mtx; }
    auto    getNativeCtxHandle() const { return m_nativeCtx.ctx; }
    auto    getNativeDeviceHandle() const { return m_nativeCtx.deviceHandle; }
    auto&   getShaderHeader() const { return m_shaderCollector.getShaderHeader(); }
    auto    isInited() const { return m_inited; }
    bool    isRunning() { return m_glCallbackLoopRunning; }
    auto    getAssetManager() const { return m_assetManager.get(); }
    auto&   getLoopExitSema() { return m_loopExit; }
    auto&   perCtxQuads() { return m_perCtxQuad; }

    std::string m_resRootPath; // must be an absolute path
    std::string m_resFile        = "res.txt";
    int32_t     m_hwDpi          = 96;
    float       m_androidDensity = 1.f;
    glm::vec2   m_androidDpi{};

#if !defined(ARA_USE_GLFW) && defined(_WIN32)
    static LRESULT CALLBACK WGLMessageHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif
#ifdef __ANDROID__
    void *android_app = nullptr;
#endif

protected:
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    std::unique_ptr<WindowManager> m_winMan;
    GLWindow                      *m_win = nullptr;
#elif _WIN32
    WNDCLASSEX         wcex;
    static inline HWND m_hWnd = nullptr;
    HDC                m_hdc  = nullptr;  // Private GDI Device Context
    HGLRC              m_hRC  = nullptr;  // Permanent Rendering Context
    void              *g_win  = nullptr;
    std::string        m_wglClassName;
    std::thread        m_msgLoop;
    Conditional        m_msgLoopSp;
    Conditional        m_msgLoopEnd;
    bool               m_msgLoopRunning = false;
#endif
    GLCaps                          m_caps;
    GLNativeCtxHnd                  m_nativeCtx;
    GLStateManager                  m_stateMan;
    std::shared_ptr<Quad>           m_stdQuad;
    std::unique_ptr<TypoGlyphMap>   m_typoGlyphMap;
    ShaderCollector                 m_shaderCollector;
    TextureCollector                m_textureCollector;
    std::unique_ptr<AssetManager>   m_assetManager;
    std::list<void *>               m_contexts;
    std::vector<std::string>        m_appMessages;
    std::vector<std::string>        m_appMsgStatic;
    glm::vec4                       m_appMsgCol{1.f};
    int                             m_typoFontHeight       = 0;
    GLuint                          m_nullVao              = 0;
    GLuint                          m_nrSamples            = 2;
    size_t                          m_appMessagesNumLines  = 10;
    size_t                          m_appMsgStaticNumLines = 10;
    int32_t                         m_max_tex_units        = 0;

    std::unordered_map<void *, std::shared_ptr<Quad>>           m_perCtxQuad;
    std::list<std::pair<std::function<bool()>, Conditional *>>  m_glCallbacks;

    bool m_continousChangeCheck = true;
    bool m_loadMouseCursorIcons = true;
    bool m_inited               = false;
    bool m_checkedCaps          = false;
    bool m_useFallback          = false;
    bool m_isIntelRenderer      = false;
    bool m_ctx                  = false;
    bool m_doResetCtx           = false;
    bool m_selfManagedCtx       = true;
    void *m_enterCtx            = nullptr;

    std::thread::id m_mainThreadId;
    std::thread     m_glCallbackLoop;
    std::jthread    m_resUpdt;
    std::mutex      m_mtx;

    std::atomic<bool> m_glCallbackLoopRunning = {false};

    Conditional                         m_sema;
    Conditional                         m_glCallbackLoopRunningSem;
    Conditional                         m_loopExit;
    Conditional                         m_resUpdtExited;
    std::list<Conditional*>             m_semaQueue;
    std::list<std::function<void()>>    m_updtResCb;
};

}  // namespace ara