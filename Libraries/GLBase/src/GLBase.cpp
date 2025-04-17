#include "GLBase.h"

#include <Utils/Texture.h>
#include <Utils/TextureCollector.h>

#include "GeoPrimitives/Quad.h"
#include "Res/ResInstance.h"
#include "Utils/TypoGlyphMap.h"
#include "WindowManagement/GLWindow.h"

using namespace glm;
using namespace std;

// NOTE: FBOs and VAOs can't be shared since they don't contain actual data

namespace ara {

GLBase::GLBase() {
    g_textureCollector.init(this);
    g_winMan = std::make_unique<WindowManager>(this);
}

/// <summary>
/// init a GL context with standard resources
/// </summary>
bool GLBase::m_init(bool doInitResources, void *winHnd) {
    if (g_inited) return true;

    g_mainThreadId = this_thread::get_id();

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    g_winMan->setMainThreadId(g_mainThreadId);
    if (m_selfManagedCtx) {
        glWinPar gp;
        gp.createHidden = true;
        gp.doInit       = true;
        gp.hidInput     = false;
        gp.width        = 5;
        gp.height       = 5;
        gp.shiftX       = 0;
        gp.shiftY       = 0;
        gp.shareCont    = winHnd;
        g_win           = g_winMan->addWin(&gp);
    }
#elif _WIN32
    // create an invisible window with a valid gl context which will contain the
    // resources and be shared to all context that are created afterwards
    m_createCtx();
#endif
    initToThisCtx();
    checkCapabilities();
    if (doInitResources) {
        initResources();
    }
#ifdef ARA_USE_GLFW
    GLWindow::makeNoneCurrent();
#elif _WIN32
    wglMakeCurrent(nullptr, nullptr);
#endif

    g_inited = true;
    return g_win ? true : false;
}

/// <summary>
/// check for required version, if not available set fallback switch ->
/// OpenGL 3.2 compatibilty
/// </summary>
void GLBase::checkCapabilities() {
    if (m_checkedCaps) return;

    // Compute Shaders -> core since 4.3 (Surface Generator)
    // shaderbuffer storage -> 4.3
    // TFO -> 4.0
    // Tesselation Shader -> 4.0
    // VAO -> core since 3.0
    // instanced array -> core since 3.3

    glGetIntegerv(GL_MAJOR_VERSION, &g_caps.major_vers);
    glGetIntegerv(GL_MINOR_VERSION, &g_caps.minor_vers);

#if defined(ARA_USE_EGL) || defined(ARA_USE_GLES31)
    LOG << " Using GLES Version " << g_caps.major_vers << "." << g_caps.minor_vers;
#endif

    char se[111] = {0};
#if defined(ARA_USE_EGL) || defined(ARA_USE_GLES31)
    // note: the precision qualifiers are necessary for GLES!!!
    sprintf(se,
            "#version %d%d%d es\n#extension GL_EXT_shader_io_blocks : enable\nprecision highp float;\nprecision highp "
            "sampler3D;\n",
            g_caps.major_vers, g_caps.minor_vers > 10 ? g_caps.minor_vers / 10 : g_caps.minor_vers,
            g_caps.minor_vers > 10 ? g_caps.minor_vers % 10 : 0);
#else
    sprintf(se, "#version %d%d%d\n", g_caps.major_vers,
            g_caps.minor_vers > 10 ? g_caps.minor_vers / 10 : g_caps.minor_vers,
            g_caps.minor_vers > 10 ? g_caps.minor_vers % 10 : 0);
#endif
    setShaderHeader(se);

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &g_caps.max_tex_units);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &g_caps.max_tex_size);
#ifndef ARA_USE_GLES31
    glGetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS, &g_caps.max_framebuffer_layers);
    glGetIntegerv(GL_MAX_GEOMETRY_SHADER_INVOCATIONS, &g_caps.max_shader_invoc);
    glGetIntegerv(GL_MULTISAMPLE,
                  &g_caps.multisample);  // always 1 with qtquickview?
#else
    glGetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS_EXT, &g_caps.max_framebuffer_layers);
    glGetIntegerv(GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT, &g_caps.max_shader_invoc);
#endif
    glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &g_caps.max_compute_shader_block);
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &g_caps.max_nr_layers);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &g_caps.max_nr_attachments);
    glGetIntegerv(GL_MAX_SAMPLES, &g_caps.max_nr_samples);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &g_caps.max_nr_drawbuffers);

#ifdef __APPLE__
    // on osx 10.15 intel HD 4000 there is a problem with mimap generation ...
    // simply doesnt work so disable it by force
    std::string renderer((char *)glGetString(GL_RENDERER));
    std::size_t found = renderer.find("Intel");
    if (found != std::string::npos) {
        g_isIntelRenderer    = true;
        g_caps.max_tex_level = 1;
    }
#endif
    /*
        LOG << "GL_MAX_TEXTURE_IMAGE_UNITS " << g_caps.max_tex_units;
        LOG << "GL_MAX_TEXTURE_SIZE " << g_caps.max_tex_size;
        LOG << "GL_TEXTURE_MAX_LEVEL " << g_caps.max_tex_level;
        LOG << "GL_MAX_FRAMEBUFFER_LAYERS " << g_caps.max_framebuffer_layers;
        LOG << "GL_MAX_GEOMETRY_SHADER_INVOCATIONS " << g_caps.max_shader_invoc;
        LOG << "GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS " <<
       g_caps.max_compute_shader_block; LOG << "GL_MAX_ARRAY_TEXTURE_LAYERS " <<
       g_caps.max_nr_layers; LOG << "GL_MAX_COLOR_ATTACHMENTS " <<
       g_caps.max_nr_attachments; LOG << "GL_MULTISAMPLE " <<
       g_caps.multisample; LOG << "GL_MAX_SAMPLES " << g_caps.max_nr_samples;
        LOG << "GL_MAX_DRAW_BUFFERS " << g_caps.max_nr_drawbuffers;
    */
    m_checkedCaps = true;
}

/// <summary>
/// init standard resources in existing context
/// </summary>
void GLBase::initToThisCtx() {
    initGLEW();

    checkCapabilities();

    g_shaderCollector.getStdCol();  // init std col shader
    g_shaderCollector.getStdTex();  // init std tex shader
    g_nativeCtx = getGLCtx();
    glGenVertexArrays(1, &g_nullVao);

    glFinish();
}

/// <summary>
/// load common resources form disk (fonts, icons, etc.) Done file by file in
/// debug mode and from res_comp in Release mode
/// </summary>
void GLBase::initResources() {
    if (!g_resInstance) {
        g_resInstance = make_unique<Instance>(g_resRootPath, "res_comp", this);

        if (g_resInstance->Load(g_resFile, false)) {
            LOG << "[OK] GLBase Resource file " << g_resRootPath + "/" + g_resFile << " loaded. Using compilation file "
                << (g_resInstance->usingComp() ? "true" : "false");
        } else {
            for (ResNode::e_error &err : g_resInstance->getRoot()->errList)
                LOGE << "Line " << err.lineIndex + 1 << " err:" << err.errorString;
        }

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
#if defined(__ANDROID__) && !defined(ARA_ANDROID_PURE_NATIVE_APP)
#else
        getWinMan()->setRes(g_resInstance.get());
#endif
#ifdef ARA_USE_GLFW
        getWinMan()->loadMouseCursors();
#endif

#ifndef __ANDROID__
        if (!g_resInstance->usingComp()) {
            m_resUpdtRun = true;

            // start resources files update loop
            m_resUpdt = std::thread([this] {
                while (m_resUpdtRun) {
                    checkResourceChanges();
                    m_resUpdtSema.wait(800);
                }

                clearGlCbQueue();
                m_resUpdtExited.notify();
            });

            m_resUpdt.detach();
        }
#endif
#endif
    }
}

void GLBase::checkResourceChanges() {
    bool changed = g_resInstance->checkForResSourceChange();
    changed      = changed || g_resInstance->checkForChangesInFolderFiles();

    if (changed) {
#ifdef ARA_USE_GLFW
        // style updating must be sync with the gl loop
        // get the actually focused window and push the update to its glqueue
        GLFWWindow *win = getWinMan()->getFocusedWin();
        if (win) {
            win->setGlCb([&] {
                // Resources reside inside GLBase to not consume more memory
                // than necessary threaded access to the resources is provided
                // via an internal mutex
                g_resInstance->CallResSourceChange();
                g_resInstance->CallForChangesInFolderFiles();

                // update all
                if (g_updtResCb) g_updtResCb();
            });
        }
#else
        if (g_updtResCb) g_updtResCb();
#endif
    }
}

/// <summary>
/// init standard resources in existing context
/// </summary>
void GLBase::destroy(bool terminateGLFW) {
    if (!g_inited) return;

    if (g_loopRunning) m_stopRenderLoop();
    g_openGlCbs.clear();

    // gl render loop for sure is stopped at this point, that means also the gl
    // context is unbound
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    if (m_selfManagedCtx && g_win) g_win->makeCurrent();
#endif
    // remove all gl resources, must be done on a valid gl context
    // g_stdQuad.reset();
    g_shaderCollector.clear();
    glDeleteVertexArrays(1, &g_nullVao);

    if (m_resUpdtRun) {
        m_resUpdtRun = false;
        m_resUpdtSema.notify();
        m_resUpdtExited.wait();
        g_resInstance.reset();
        g_resInstance = nullptr;
    }

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    if (m_selfManagedCtx && g_win) {
        GLWindow::makeNoneCurrent();
        g_winMan->removeWin(g_win, terminateGLFW);
    }
#elif _WIN32
    m_destroyCtx();
#endif
    g_inited      = false;
    m_checkedCaps = false;
}

/// <summary>
/// create an individual context with local basic resources
/// </summary>
#ifdef ARA_USE_GLFW
unique_ptr<GLWindow> GLBase::createOpenGLCtx(bool initGLFW) {
    // unique_lock<mutex> lock(g_mtx);

    auto     gwin = make_unique<GLWindow>();
    glWinPar gp;
    gp.createHidden = true;
    gp.doInit       = initGLFW;
    if (!gwin->init(gp)) gwin.reset();

    // init GLEW
    if (!initGLEW()) {
        gwin.reset();
    }

    glGetError();  // delete glew standard error (bug in glew)
    return gwin;   // in c++17 implicitly calls move construction
}
#endif

void GLBase::m_startRenderLoop() {
    if (!g_loopRunning) {
        // start a separate thread for processing
        g_renderLoop = std::thread([this] { renderLoop(); });
        g_renderLoop.detach();

        // wait for loop to be running
        g_loopRunningSem.wait();

        g_loopRunningSem.reset();
    }
}

void GLBase::renderLoop() {
#ifdef ARA_USE_GLFW
    // initially make the GLBase context current
    if (!g_win) return;
    g_win->makeCurrent();
#elif _WIN32
    if (!m_hdc || !m_hRC) {
        LOGE << "m_glbase.renderLoop Error, context not valid";
        return;
    }

    if (!wglMakeCurrent(m_hdc, m_hRC)) LOGE << "m_glbase.renderLoop Error, could not make current context " << m_hRC;
#endif

    g_loopRunningSem.notify();  // wait until another thread is waiting for this notify
    g_loopRunning = true;

    while (g_loopRunning) {
        g_sema.wait(0);  // wait infinitely

        g_mtx.lock();
        // process the callbacks
        for (auto it = g_openGlCbs.begin(); it != g_openGlCbs.end();) {
            if (it->first()) {
                if (it->second) it->second->notify();
                it = g_openGlCbs.erase(it);
            } else
                it++;
        }
        g_mtx.unlock();

        glFinish();
    }

    g_loopRunning = false;
#ifdef ARA_USE_GLFW
    glfwMakeContextCurrent(nullptr);
#elif _WIN32
    wglMakeCurrent(nullptr, nullptr);
#endif

    g_loopExit.notify();
}

void GLBase::m_stopRenderLoop() {
    if (g_loopRunning) {
        g_loopRunning = false;
        g_sema.notify();  // notify in case the loop is waiting;
        g_loopExit.wait(0);
    }
}

void GLBase::m_createCtx() {
#if !defined(ARA_USE_GLFW) && defined(_WIN32)
    // in case any context is current, make none current
    wglMakeCurrent(nullptr, nullptr);

    HINSTANCE hInstance = (HINSTANCE)::GetModuleHandle(NULL);

    // register a window class for subsequent use in calls to the CreateWindow
    // or CreateWindowEx function.
    wcex.cbSize        = sizeof(WNDCLASSEX);
    wcex.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc   = &m_glbase.WGLMessageHandler;
    wcex.cbWndExtra    = sizeof(GLBase *);  // Reserve space to store the instance pointer
    wcex.cbClsExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon(hInstance, IDI_WINLOGO);
    wcex.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = NULL;  // No Background Required For GL
    wcex.lpszMenuName  = NULL;

    m_wglClassName     = "OpenGL_" + std::to_string((uint64_t)this);
    wcex.lpszClassName = m_wglClassName.c_str();  // Set The Class Name
    wcex.hIconSm       = LoadIcon(wcex.hInstance, IDI_WINLOGO);

    if (!RegisterClassEx(&wcex)) {
        LOGE << "m_glbase.m_createCtx Error: could not register Window class";
        return;
    }

    // create an message window for receiving the windows messages from the core
    // (WS_VISIBLE not set and no ShowWindow() call)
    string name = "GLBaseWin";
    m_hWnd      = CreateWindowA(m_wglClassName.c_str(), name.c_str(), WS_POPUPWINDOW, 0, 0, 10,
                                10,  // ..anything, the window is invisible
                                nullptr, nullptr, hInstance, nullptr);

    if (!m_hWnd) throw std::exception("CreateWindow failed");

    // Store instance pointer
    SetWindowLongPtrW(m_hWnd, 0, reinterpret_cast<LONG_PTR>(this));

    // Get device context only once.
    m_hdc = GetDC(m_hWnd);

    static PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,  // bit depth
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        16,  // z-buffer depth
        0,
        0,
        0,
        0,
        0,
        0,
        0,
    };

    // Pixel format.
    GLuint m_nPixelFormat = ChoosePixelFormat(m_hdc, &pfd);
    SetPixelFormat(m_hdc, m_nPixelFormat, &pfd);

    // Create the OpenGL Rendering Context.
    m_hRC = wglCreateContext(m_hdc);

    wglMakeCurrent(m_hdc, m_hRC);

    // init GLEW
    glewExperimental = GL_TRUE;
    GLenum err       = glewInit();
    if (err != GLEW_OK) {
        LOGE << "GLEW ERROR: " << glewGetErrorString(err);
        return;
    }

    err = glGetError();  // remove glew standard error

    LOG << "Vendor:   " << glGetString(GL_VENDOR);
    LOG << "Renderer: " << glGetString(GL_RENDERER);
    LOG << "Version:  " << glGetString(GL_VERSION);
    LOG << "GLSL:     " << glGetString(GL_SHADING_LANGUAGE_VERSION);

    // glViewport(0, 0, 10, 10);	// Reset The Current Viewport
    // SwapBuffers(m_hdc);

    /*
        m_msgLoop = std::thread([this](){
            m_msgLoopSp.notify();
            m_msgLoopRunning = true;

            MSG msg = { 0 };
            BOOL bRet;

            while ((bRet = GetMessage(&msg, m_hWnd, 0, 0)) != 0)
            {
                LOG << "GLBAse GetMessage ";
                if (bRet == -1)
                {
                    // handle the error and possibly exit
                }
                else
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
            m_msgLoopEnd.notify();
        });

        m_msgLoop.detach();
        m_msgLoopSp.wait(0);
        */
#endif
}

void GLBase::m_addContext(void *ctx) {
    g_contexts.emplace_back(ctx);
}

void GLBase::m_destroyCtx() {
#if !defined(ARA_USE_GLFW) && defined(_WIN32)
    if (m_hRC)  // Do We Have A Rendering Context?
    {
        if (!wglDeleteContext(m_hRC))  // Are We Able To Delete The RC?
            LOGE << "Release Rendering Context Failed.";

        m_hRC = nullptr;  // Set RC To NULL
    }

    if (m_hWnd && !DestroyWindow(m_hWnd))  // Are We Able To Destroy The Window?
    {
        LOGE << "Could Not Release hWnd..";
        m_hWnd = nullptr;  // Set hWnd To NULL
    }

    if (!UnregisterClass(wcex.lpszClassName,
                         wcex.hInstance))  // Are We Able To Unregister Class
    {
        LOGE << "Could Not Unregister Class.";
        wcex.hInstance = nullptr;  // Set m_hInstance To NULL
    }
#endif
}

void GLBase::m_removeContext(void *ctx) {
    g_contexts.remove_if([ctx](const void *lc) { return lc == ctx; });
}

#if !defined(ARA_USE_GLFW) && defined(_WIN32)
LRESULT CALLBACK m_glbase.WGLMessageHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hWnd, message, wParam, lParam);
}
#endif

void GLBase::m_shareCtx() {
    if (!g_contexts.empty()) {
        auto c = getGLCtx();
#ifdef _WIN32
        wglShareLists((HGLRC)g_contexts.front(), (HGLRC)c.ctx);
#endif
    }
}

void GLBase::m_addEvtCb(const std::function<bool()> &func, bool forcePush) {
#ifdef __ANDROID__
    func();
#else
    if (std::this_thread::get_id() == g_mainThreadId && !forcePush) {
        func();
    } else {
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
        getWinMan()->addEvtLoopCb(func);
#ifdef ARA_USE_GLFW
        glfwPostEmptyEvent();
#endif
#endif
    }
#endif
}

void GLBase::m_addGlCb(const std::function<bool()> &func, Conditional *sema) {
    std::unique_lock<std::mutex> lock(g_mtx);
    g_openGlCbs.emplace_back(func, sema);
    if (sema)
        g_sema.notify();  // in the worst case, is sent before g_sema is waiting, true flag wait until another thread
                          // calls wait()
}

void GLBase::addGlCbSync(const std::function<bool()> &f) {
    if (!m_selfManagedCtx) {
        f();
    } else {
        Conditional sema;
        addGlCb(f, &sema);
        sema.wait(0);
    }
}

GLNativeCtxHnd GLBase::getGLCtx() {
    GLNativeCtxHnd glCtx;
#ifdef _WIN32
    glCtx.ctx          = (void *)wglGetCurrentContext();
    glCtx.deviceHandle = (void *)wglGetCurrentDC();
#elif defined(__linux__) && !defined(ARA_USE_GLES31)
    glCtx.ctx          = (void *)glXGetCurrentContext();
    glCtx.deviceHandle = (void *)glXGetCurrentDisplay();
    glCtx.drawable     = (uint32_t)glXGetCurrentDrawable();
#elif __ANDROID__
    glCtx.ctx = (void *)eglGetCurrentContext();
#endif
    return glCtx;
}

void GLBase::m_switchCtx(GLNativeCtxHnd &ctx) {
#ifdef _WIN32
    if (!ctx.ctx) return;
    bool res = wglMakeCurrent((HDC)ctx.deviceHandle, (HGLRC)ctx.ctx);
    if (!res) {
        LOGE << " couldn't change  ctx!!!! ";
        GetLastError();
        std::cerr << "Error " << GetLastError() << std::endl;
    }
#elif defined(__linux__) && !defined(ARA_USE_GLES31)
    //    if (ctx == g_nativeCtx.ctx)
    //       g_win->makeCurrent();
    //  else {
    if (!ctx.ctx) return;
    bool res = glXMakeContextCurrent((Display *)ctx.deviceHandle, static_cast<GLXDrawable>(ctx.drawable),
                                     static_cast<GLXDrawable>(ctx.drawable), (GLXContext)ctx.ctx);
    // if (!res) LOGE << "GLBase glXMakeContextCurrent failed!!!";
    //}
#endif
}

void GLBase::initAppMsg(const char *fontFile, int fontHeight, int screenWidth, int screenHeight) {
    g_typoGlyphMap = make_unique<TypoGlyphMap>(screenWidth, screenHeight);
    g_typoGlyphMap->loadFont(fontFile, &g_shaderCollector);
    g_typoFontHeight = fontHeight;
}

void GLBase::appmsg(const char *format, ...) {
    char    buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 255, format, args);

    if (g_appMessages.size() >= g_appMessagesNumLines) g_appMessages.erase(g_appMessages.begin());

    g_appMessages.emplace_back(buffer);
    LOG << buffer;
}

void GLBase::appMsgStatic(size_t lineIdx, const char *format, ...) {
    char    buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 255, format, args);

    if (g_appMsgStatic.size() > lineIdx) g_appMsgStatic[lineIdx] = string(buffer);
}

void GLBase::renderAppMsgs() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);  // JUST FOR DEMO: should be done only when needed

    float lineHeight = g_typoGlyphMap->getRelativeLineHeight() * 1.5f;  // add a bit of space

    for (size_t i = 0; i < g_appMsgStatic.size(); i++)
        if (!g_appMsgStatic[i].empty())
            g_typoGlyphMap->print(-0.95f, 0.95f - (i + 1) * lineHeight, g_appMsgStatic[i], g_typoFontHeight,
                                  &g_appMsgCol[0]);

    for (size_t i = 0; i < g_appMessages.size(); i++)
        if (!g_appMessages[i].empty())
            g_typoGlyphMap->print(-0.95f, 0.95f - (i + 1 + g_appMsgStaticNumLines) * lineHeight, g_appMessages[i],
                                  g_typoFontHeight, &g_appMsgCol[0]);
};

void GLBase::clearGlCbQueue() {
    std::unique_lock<std::mutex> lock(g_mtx);
    g_openGlCbs.clear();
}

void GLBase::setResRootPath(const std::string &str) {
#ifndef ARA_USE_CMRC
    g_resRootPath = (std::filesystem::current_path() / str).string();
#else
    g_resRootPath = str;
#endif
}

GLStateManager &GLBase::stateMan() {
    if (!g_inited) {
        initToThisCtx();
    }
    return g_stateMan;
}

void GLBase::makeCurrent() {
    if (!g_inited) {
        initToThisCtx();
    }
    g_win->makeCurrent();
}

Quad *GLBase::stdQuad() {
    if (!g_inited) {
        initToThisCtx();
    }
    return g_stdQuad.get();
}

bool GLBase::getUseFallback() {
    if (!g_inited) {
        init();
    }
    return g_useFallback;
}

int32_t GLBase::maxNrAttachments() {
    if (!g_inited) {
        initToThisCtx();
    }
    return g_caps.max_nr_attachments;
}

void GLBase::setAppMsgStaticInfoNumLines(size_t count) {
    g_appMsgStaticNumLines = count;
    g_appMsgStatic.resize(count);
}

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
GLWindow *GLBase::getWin() {
    if (!g_inited) {
        init();
    }
    return g_win;
}
#ifdef ARA_USE_GLFW
GLContext GLBase::getGlfwHnd() {
    if (!g_inited) {
        init();
    }
    return g_win->getCtx();
}
#else
GLContext GLBase::getGlfwHnd() {
    return nullptr;
}
#endif
WindowManager *GLBase::getWinMan() {
    return g_winMan.get();
}
#endif

}  // namespace ara
