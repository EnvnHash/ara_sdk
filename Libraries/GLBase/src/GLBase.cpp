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

#include "GLBase.h"

#include <string_utils.h>
#include <Utils/Texture.h>
#include <Utils/TextureCollector.h>

#include "GeoPrimitives/Quad.h"
#include "Asset/AssetManager.h"
#include "Utils/Typo/TypoGlyphMap.h"
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
bool GLBase::init(bool doInitResources, void *winHnd) {
    if (g_inited) {
        return true;
    }
    g_mainThreadId = this_thread::get_id();

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    g_winMan->setMainThreadId(g_mainThreadId);
    if (m_selfManagedCtx) {
        g_win = g_winMan->addWin(glWinPar{
            .createHidden = true,
            .hidInput     = false,
            .size         = { 5, 5 },
            .shareCont    = winHnd,
#ifdef __ANDROID__
            .contScale   = { g_androidDensity , g_androidDensity }
#endif
        });
    }
#elif _WIN32
    // create an invisible window with a valid gl context which will contain the resources and be shared to all context
    // that are created afterwards
    createCtx();
#endif

    initToThisCtx();
    checkCapabilities();
    if (doInitResources) {
        initResources();
    }

#if defined(ARA_USE_GLFW) || defined(__ANDROID__)
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
    if (m_checkedCaps) {
        return;
    }

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

    std::stringstream ss;
#if defined(ARA_USE_EGL) || defined(ARA_USE_GLES31)
    // note: the precision qualifiers are necessary for GLES!!!
    ss << "#version "
        << std::to_string(g_caps.major_vers)
        << std::to_string(g_caps.minor_vers > 10 ? g_caps.minor_vers / 10 : g_caps.minor_vers)
        << std::to_string(g_caps.minor_vers > 10 ? g_caps.minor_vers % 10 : 0)
        << " es\n#extension GL_EXT_shader_io_blocks : enable\nprecision highp float;\nprecision highp sampler3D;\n";
#else
    ss << "#version "
        << std::to_string(g_caps.major_vers)
        << std::to_string(g_caps.minor_vers > 10 ? g_caps.minor_vers / 10 : g_caps.minor_vers)
        << std::to_string(g_caps.minor_vers > 10 ? g_caps.minor_vers % 10 : 0)
        << "\n";
#endif
    setShaderHeader(ss.str());

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &g_caps.max_tex_units);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &g_caps.max_tex_size);
#ifndef ARA_USE_GLES31
#ifndef __APPLE__
    glGetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS, &g_caps.max_framebuffer_layers);
    glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &g_caps.max_compute_shader_block);
#endif
    glGetIntegerv(GL_MAX_GEOMETRY_SHADER_INVOCATIONS, &g_caps.max_shader_invoc);
    glGetIntegerv(GL_MULTISAMPLE, &g_caps.multisample);
#else
    glGetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS_EXT, &g_caps.max_framebuffer_layers);
    glGetIntegerv(GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT, &g_caps.max_shader_invoc);
#endif
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
/// load common resources form disk (fonts, icons, etc.) Done file by file in debug mode and from res_comp in Release mode
/// </summary>
void GLBase::initResources() {
    if (!g_assetManager) {
        g_assetManager = make_unique<AssetManager>(g_resRootPath, "res_comp", this);

        if (g_assetManager->Load(g_resFile)) {
            LOG << "[OK] GLBase Resource file " << g_resRootPath + "/" + g_resFile << " loaded. "
              << (g_assetManager->usingComp() ? " Used compiled binary asset file" : "");
        } else {
            for (ResNode::e_error &err : g_assetManager->getRoot()->errList) {
                LOGE << "Line " << err.lineIndex + 1 << " err:" << err.errorString;
            }
        }

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
#if defined(__ANDROID__) && !defined(ARA_ANDROID_PURE_NATIVE_APP)
#else
        getWinMan()->setAssetManager(g_assetManager.get());
#endif
#ifdef ARA_USE_GLFW
        getWinMan()->loadMouseCursors();
#endif

#ifndef __ANDROID__
        if (!g_assetManager->usingComp()) {
            m_resUpdtRun = true;

            // start resources files update loop
            m_resUpdt = std::thread([this] {
                while (m_resUpdtRun) {
                    checkResourceChanges();
                    std::this_thread::sleep_for(std::chrono::milliseconds(800));
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
    bool changed = g_assetManager->checkForChangesInFolderFiles();

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
                g_assetManager->callResSourceChange();
                g_assetManager->callForChangesInFolderFiles();

                // update all
                if (g_updtResCb) {
                    g_updtResCb();
                }
            });
        }
#else
        if (g_updtResCb) {
            g_updtResCb();
        }
#endif
    }
}

/// <summary>
/// init standard resources in existing context
/// </summary>
void GLBase::destroy(bool terminateGLFW) {
    if (!g_inited) {
        return;
    }

    if (g_glCallbackLoopRunning) {
        stopProcCallbackLoop();
    }
    g_glCallbacks.clear();

    // gl render loop for sure is stopped at this point, that means also the gl context is unbound
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    if (m_selfManagedCtx && g_win) {
        g_win->makeCurrent();
    }
#endif
    // remove all gl resources, must be done on a valid gl context/ g_stdQuad.reset();
    g_shaderCollector.clear();
    glDeleteVertexArrays(1, &g_nullVao);

    if (m_resUpdtRun) {
        m_resUpdtRun = false;
        m_resUpdtExited.wait();
    }

    g_assetManager.reset();
    g_assetManager = nullptr;

    g_textureCollector.clear();

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    if (m_selfManagedCtx && g_win) {
        GLWindow::makeNoneCurrent();
        g_winMan->removeWin(g_win, terminateGLFW);
    }
#elif _WIN32
    destroyCtx();
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
    if (!gwin->init(gp)) {
        gwin.reset();
    }

    // init GLEW
    if (!initGLEW()) {
        gwin.reset();
    }

    glGetError();  // delete glew standard error (bug in glew)
    return gwin;   // in c++17 implicitly calls move construction
}
#endif

void GLBase::startGlCallbackProcLoop() {
    if (!g_glCallbackLoopRunning) {
        // start a separate thread for processing
        g_glCallbackLoop = std::thread([this] { glCallbackLoop(); });
        g_glCallbackLoop.detach();

        // wait for loop to be running
        g_glCallbackLoopRunningSem.wait();
        g_glCallbackLoopRunningSem.reset();
    }
}

void GLBase::glCallbackLoop() {
#ifdef ARA_USE_GLFW
    // initially make the GLBase context current
    if (!g_win) {
        return;
    }
    g_win->makeCurrent();
#elif _WIN32
    if (!m_hdc || !m_hRC) {
        LOGE << "m_glbase.renderLoop Error, context not valid";
        return;
    }

    if (!wglMakeCurrent(m_hdc, m_hRC)) LOGE << "m_glbase.renderLoop Error, could not make current context " << m_hRC;
#endif

    g_glCallbackLoopRunningSem.notify();  // wait until another thread is waiting for this notify
    g_glCallbackLoopRunning = true;

    while (g_glCallbackLoopRunning) {
        g_sema.wait(0);  // wait infinitely
        g_mtx.lock();
        iterateGlCallback();
        g_mtx.unlock();
        glFinish();
    }

    g_glCallbackLoopRunning = false;
#ifdef ARA_USE_GLFW
    glfwMakeContextCurrent(nullptr);
#elif _WIN32
    wglMakeCurrent(nullptr, nullptr);
#endif

    g_loopExit.notify();
}

void GLBase::iterateGlCallback() {
    for (auto it = g_glCallbacks.begin(); it != g_glCallbacks.end();) {
        if (it->first()) {
            if (it->second) {
                it->second->notify();
            }
            it = g_glCallbacks.erase(it);
        } else {
            ++it;
        }
    }
}

void GLBase::stopProcCallbackLoop() {
    if (g_glCallbackLoopRunning) {
        g_glCallbackLoopRunning = false;
        g_sema.notify();  // notify in case the loop is waiting;
        g_loopExit.wait(0);
    }
}

void GLBase::createCtx() {
#if !defined(ARA_USE_GLFW) && defined(_WIN32)
    // in case any context is current, make none current
    wglMakeCurrent(nullptr, nullptr);
    HINSTANCE hInstance = (HINSTANCE)::GetModuleHandle(nullptr);

    // register a window class for subsequent use in calls to the CreateWindow
    // or CreateWindowEx function.
    m_wglClassName     = "OpenGL_" + std::to_string((uint64_t)this);
    wcex =  WNDCLASSEX{
        .cbSize        = sizeof(WNDCLASSEX);
        .style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        .lpfnWndProc   = &m_glbase.WGLMessageHandler;
        .cbWndExtra    = sizeof(GLBase *);  // Reserve space to store the instance pointer
        .cbClsExtra    = 0;
        .hInstance     = hInstance;
        .hIcon         = LoadIcon(hInstance, IDI_WINLOGO);
        .hCursor       = LoadCursor(nullptr, IDC_ARROW);
        .hbrBackground = nullptr;  // No Background Required For GL
        .lpszMenuName  = nullptr;
        .lpszClassName = m_wglClassName.c_str();  // Set The Class Name
        .hIconSm       = LoadIcon(wcex.hInstance, IDI_WINLOGO);
    };

    if (!RegisterClassEx(&wcex)) {
        LOGE << "m_glbase.createCtx Error: could not register Window class";
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
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        16,  // z-buffer depth
        0, 0, 0, 0, 0, 0, 0,
    };

    // Pixel format.
    GLuint m_nPixelFormat = ChoosePixelFormat(m_hdc, &pfd);
    SetPixelFormat(m_hdc, m_nPixelFormat, &pfd);

    m_hRC = wglCreateContext(m_hdc); // Create the OpenGL Rendering Context.
    wglMakeCurrent(m_hdc, m_hRC);

    initGLEW();
    printGLVersion();
#endif
}

void GLBase::addContext(void *ctx) {
    g_contexts.emplace_back(ctx);
}

void GLBase::destroyCtx() {
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

void GLBase::removeContext(void *ctx) {
    g_contexts.remove_if([ctx](const void *lc) { return lc == ctx; });
}

#if !defined(ARA_USE_GLFW) && defined(_WIN32)
LRESULT CALLBACK m_glbase.WGLMessageHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hWnd, message, wParam, lParam);
}
#endif

void GLBase::shareCtx() {
    if (!g_contexts.empty()) {
        auto c = getGLCtx();
#ifdef _WIN32
        wglShareLists((HGLRC)g_contexts.front(), (HGLRC)c.ctx);
#endif
    }
}

void GLBase::addEvtCb(const std::function<bool()> &func, bool forcePush) {
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

void GLBase::addGlCb(const std::function<bool()> &func, Conditional *sema) {
    std::unique_lock<std::mutex> lock(g_mtx);
    g_glCallbacks.emplace_back(func, sema);
    if (sema) {
        g_sema.notify();  // in the worst case, is sent before g_sema is waiting, true flag wait until another thread
                          // calls wait()
    }
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
    glCtx.ctx          = static_cast<void*>(wglGetCurrentContext());
    glCtx.deviceHandle = static_cast<void*>(wglGetCurrentDC());
#elif defined(__linux__) && !defined(ARA_USE_GLES31)
    glCtx.ctx          = static_cast<void*>(glXGetCurrentContext());
    glCtx.deviceHandle = static_cast<void*>(glXGetCurrentDisplay());
    glCtx.drawable     = static_cast<uint32_t>(glXGetCurrentDrawable());
#elif __ANDROID__
    glCtx.ctx = static_cast<void *>(eglGetCurrentContext());
#endif
    return glCtx;
}

void GLBase::switchCtx(GLNativeCtxHnd &ctx) {
#ifdef _WIN32
    if (!ctx.ctx) {
        return;
    }

    if (!wglMakeCurrent(static_cast<HDC>(ctx.deviceHandle), static_cast<HGLRC>(ctx.ctx))) {
        LOGE << " couldn't change  ctx!!!! ";
        GetLastError();
        std::cerr << "Error " << GetLastError() << std::endl;
    }
#elif defined(__linux__) && !defined(ARA_USE_GLES31)
    if (!ctx.ctx) {
        return;
    }
    glXMakeContextCurrent((Display *)ctx.deviceHandle, static_cast<GLXDrawable>(ctx.drawable),
                          static_cast<GLXDrawable>(ctx.drawable), (GLXContext)ctx.ctx);
#endif
}

void GLBase::initAppMsg(const char *fontFile, int fontHeight, int screenWidth, int screenHeight) {
    g_typoGlyphMap = make_unique<TypoGlyphMap>(screenWidth, screenHeight);
    g_typoGlyphMap->loadFont(fontFile, &g_shaderCollector);
    g_typoFontHeight = fontHeight;
}

void GLBase::clearGlCbQueue() {
    unique_lock<std::mutex> lock(g_mtx);
    g_glCallbacks.clear();
}

void GLBase::setResRootPath(const std::string &str) {
#ifndef ARA_USE_CMRC
    g_resRootPath = (filesystem::current_path() / str).string();
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
