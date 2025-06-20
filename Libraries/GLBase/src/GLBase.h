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

    void appmsg(const char *format, ...);
    void appMsgStatic(size_t lineIdx, const char *format, ...);
    void initAppMsg(const char *fontFile, int fontHeight, int screenWidth, int screenHeight);
    void renderAppMsgs();
    void startGlCallbackProcLoop();
    void stopProcCallbackLoop();
    void addEvtCb(const std::function<bool()> &func, bool forcePush = false);
    void addGlCb(const std::function<bool()> &func, Conditional *sema = nullptr);
    void glCallbackLoop();
    void iterateGlCallback();
    void initResources();
    void checkResourceChanges();
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
    WindowManager *getWinMan();
#endif

    void runOnMainThread(const std::function<bool()> &func, bool forcePush = false) { addEvtCb(func, forcePush); }
    void setResFile(std::string str) { g_resFile = std::move(str); }
    void setShaderHeader(std::string hdr) { return g_shaderCollector.setShaderHeader(std::move(hdr)); }
    void setAppMsgNumLines(size_t count) { g_appMessagesNumLines = count; }
    void setUpdtResCb(std::function<void()> f) { g_updtResCb = std::move(f); }
    void useSelfManagedCtx(bool val) { m_selfManagedCtx = val; }

    void    procGlCb() { g_sema.notify(); }
    auto    getNrCtx() { return g_contexts.size(); }
    auto    &shaderCollector() { return g_shaderCollector; }
    auto    &textureCollector() { return g_textureCollector; }
    auto    *nullVao() { return &g_nullVao; }
    auto    getMainThreadId() { return g_mainThreadId; }
    auto    rendererIsIntel() const { return g_isIntelRenderer; }
    auto    maxTexUnits() const { return g_caps.max_tex_units; }
    auto    maxShaderInvocations() const { return g_caps.max_shader_invoc; }
    auto    getNrSamples() const { return static_cast<int32_t>(g_nrSamples); }
    auto    maxNrDrawBuffers() const { return g_caps.max_nr_drawbuffers; }
    auto    maxTexMipMapLevels() const { return g_caps.max_tex_level; }
    auto    getMajorVer() const { return g_caps.major_vers; }
    auto    getMinorVer() const { return g_caps.minor_vers; }
    auto    glMtx() { return &g_mtx; }
    auto    getNativeCtxHndl() const { return g_nativeCtx.ctx; }
    auto    getNativeDeviceHndl() const { return g_nativeCtx.deviceHandle; }
    auto&   getShaderHeader() { return g_shaderCollector.getShaderHeader(); }
    auto    isInited() const { return g_inited; }
    bool    isRunning() { return g_glCallbackLoopRunning; }
    auto    getAssetManager() { return g_assetManager.get(); }
    auto&   getLoopExitSema() { return g_loopExit; }
    auto&   perCtxQuads() { return g_perCtxQuad; }

#if !defined(ARA_USE_GLFW) && defined(_WIN32)
    static LRESULT CALLBACK WGLMessageHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif
    std::string g_resRootPath    = "resdata";
    std::string g_resFile        = "res.txt";
    int32_t     g_hwDpi          = 96;
    float       g_androidDensity = 1.f;
    glm::vec2   g_androidDpi{};

#ifdef __ANDROID__
    void *android_app = nullptr;
#endif

protected:
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    std::unique_ptr<WindowManager> g_winMan;
    GLWindow                      *g_win = nullptr;
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

    GLCaps                          g_caps;
    GLNativeCtxHnd                  g_nativeCtx;
    GLStateManager                  g_stateMan;
    std::shared_ptr<Quad>           g_stdQuad;
    std::unique_ptr<TypoGlyphMap>   g_typoGlyphMap;
    ShaderCollector                 g_shaderCollector;
    TextureCollector                g_textureCollector;
    std::unique_ptr<AssetManager>   g_assetManager;
    std::list<void *>               g_contexts;
    std::vector<std::string>        g_appMessages;
    std::vector<std::string>        g_appMsgStatic;
    glm::vec4                       g_appMsgCol{1.f};
    int                             g_typoFontHeight       = 0;
    GLuint                          g_nullVao              = 0;
    GLuint                          g_nrSamples            = 2;
    size_t                          g_appMessagesNumLines  = 10;
    size_t                          g_appMsgStaticNumLines = 10;
    int32_t                         g_max_tex_units        = 0;

    std::unordered_map<void *, std::shared_ptr<Quad>> g_perCtxQuad;
    std::list<std::pair<std::function<bool()>, Conditional *>> g_glCallbacks;

    bool g_inited           = false;
    bool m_checkedCaps      = false;
    bool g_useFallback      = false;
    bool g_isIntelRenderer  = false;
    bool m_ctx              = false;
    bool m_doResetCtx       = false;
    bool m_selfManagedCtx   = true;
    void *m_enterCtx        = nullptr;

    std::thread::id g_mainThreadId;
    std::thread     g_glCallbackLoop;
    std::thread     m_resUpdt;
    std::mutex      g_mtx;

    std::atomic<bool> g_glCallbackLoopRunning = {false};
    std::atomic<bool> m_resUpdtRun  = {false};

    Conditional              g_sema;
    Conditional              g_glCallbackLoopRunningSem;
    Conditional              g_loopExit;
    Conditional              m_resUpdtExited;
    std::list<Conditional *> m_semaqueue;

    std::function<void()> g_updtResCb;
};

}  // namespace ara