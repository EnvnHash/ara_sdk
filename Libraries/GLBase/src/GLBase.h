#pragma once

#include <Conditional.h>
#include <Utils/GLStateManager.h>
#include <Utils/TextureCollector.h>
#include <Shaders/ShaderCollector.h>
#include <WindowManagement/WindowManager.h>

// NOTE: FBOs and VAOs can't be shared since they don't contain actual data

namespace ara {

class Quad;
class TypoGlyphMap;

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

    bool m_init(bool doInitResources = true, void *winHnd = nullptr);
    void m_createCtx();
    void m_addContext(void *ctx);
    void m_destroyCtx();
    void m_removeContext(void *ctx);
    void m_shareCtx();
    void checkCapabilities();
    void initToThisCtx();
    void destroy(bool terminateGLFW = true);
    static void m_switchCtx(GLNativeCtxHnd &ctx);

#ifdef ARA_USE_GLFW
    static std::unique_ptr<GLWindow> createOpenGLCtx(bool initGLFW = true);
    // static std::unique_ptr<GLWindow> createOpenGLCtx(bool initGLFW = true) {
    // return m_createOpenGLCtx(initGLFW); }
#endif

    void appmsg(const char *format, ...);
    void appMsgStatic(size_t lineIdx, const char *format, ...);
    void initAppMsg(const char *fontFile, int fontHeight, int screenWidth, int screenHeight);
    void renderAppMsgs();
    void m_startRenderLoop();
    void m_stopRenderLoop();
    void m_addEvtCb(const std::function<bool()> &func, bool forcePush = false);
    void m_addGlCb(const std::function<bool()> &func, Conditional *sema = nullptr);
    void renderLoop();
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

    bool        init(bool doInitResources = true, void *winHnd = nullptr) { return m_init(doInitResources, winHnd); }
    void        runOnMainThread(const std::function<bool()> &func, bool forcePush = false) { m_addEvtCb(func, forcePush); }
    void        addGlCb(const std::function<bool()> &func, Conditional *sema = nullptr) { m_addGlCb(func, sema); }
    void        addContext(void *ctx) { m_addContext(ctx); }
    void        removeContext(void *ctx) { m_removeContext(ctx); }
    void        setResFile(std::string str) { g_resFile = std::move(str); }
    static void switchCtx(GLNativeCtxHnd &ctx) { m_switchCtx(ctx); }
    void        setShaderHeader(std::string hdr) { return g_shaderCollector.setShaderHeader(std::move(hdr)); }
    void        setAppMsgNumLines(size_t count) { g_appMessagesNumLines = count; }
    void        setUpdtResCb(std::function<void()> f) { g_updtResCb = std::move(f); }
    void        useSelfManagedCtx(bool val) { m_selfManagedCtx = val; }

    void    procGlCb() { m_procGlCb(); }
    void    m_procGlCb() { g_sema.notify(); }
    void    startRenderLoop() { m_startRenderLoop(); }
    void    stopRenderLoop() { m_stopRenderLoop(); }
    void    shareCtx() { m_shareCtx(); }

    size_t              getNrCtx() { return g_contexts.size(); }
    ShaderCollector     &shaderCollector() { return g_shaderCollector; }
    TextureCollector    &textureCollector() { return g_textureCollector; }
    GLuint              *nullVao() { return &g_nullVao; }
    std::thread::id     getMainThreadId() { return g_mainThreadId; }
    bool                rendererIsIntel() const { return g_isIntelRenderer; }
    int32_t             maxTexUnits() const { return g_caps.max_tex_units; }
    int32_t             maxShaderInvocations() const { return g_caps.max_shader_invoc; }
    int32_t             getNrSamples() const { return static_cast<int32_t>(g_nrSamples); }
    int32_t             maxNrDrawBuffers() const { return g_caps.max_nr_drawbuffers; }
    int32_t             maxTexMipMapLevels() const { return g_caps.max_tex_level; }
    int32_t             getMajorVer() const { return g_caps.major_vers; }
    int32_t             getMinorVer() const { return g_caps.minor_vers; }
    std::mutex          *glMtx() { return &g_mtx; }
    void                *getNativeCtxHndl() const { return g_nativeCtx.ctx; }
    void                *getNativeDeviceHndl() const { return g_nativeCtx.deviceHandle; }
    std::string         getShaderHeader() { return g_shaderCollector.getShaderHeader(); }
    bool                isInited() const { return g_inited; }
    bool                isRunning() { return g_loopRunning; }
    Instance            *getResInstance() { return g_resInstance.get(); }
    Conditional         &getLoopExitSema() { return g_loopExit; }

    std::unordered_map<void *, std::shared_ptr<Quad>>   &perCtxQuads() { return g_perCtxQuad; }

#if !defined(ARA_USE_GLFW) && defined(_WIN32)
    static LRESULT CALLBACK WGLMessageHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#endif
    std::string g_resRootPath = "resdata";  // note: was private with inline getter and setters, but
                                            // assigning didn't work on android
    std::string g_resFile        = "res.txt";
    int32_t     g_hwDpi          = 96;
    float       g_androidDensity = 1.f;

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
    std::unique_ptr<Instance>       g_resInstance;
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
    std::list<std::pair<std::function<bool()>, Conditional *>> g_openGlCbs;

    bool g_inited          = false;
    bool m_checkedCaps     = false;
    bool g_useFallback     = false;
    bool g_isIntelRenderer = false;
    bool m_ctx             = false;
    bool m_doResetCtx      = false;
#ifdef ARA_USE_EGL
    bool m_selfManagedCtx = false;
#else
    bool m_selfManagedCtx = true;
#endif
    void *m_enterCtx = nullptr;

    std::thread::id g_mainThreadId;

    std::thread g_renderLoop;
    std::thread m_resUpdt;

    std::mutex g_mtx;

    std::atomic<bool> g_loopRunning = {false};
    std::atomic<bool> m_resUpdtRun  = {false};

    Conditional              g_sema;
    Conditional              g_loopRunningSem;
    Conditional              g_loopExit;
    Conditional              m_resUpdtExited;
    Conditional              m_resUpdtSema;
    std::list<Conditional *> m_semaqueue;

    std::function<void()> g_updtResCb;
};

}  // namespace ara