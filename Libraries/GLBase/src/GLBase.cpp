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
#include <Utils/TextureCollector.h>
#include "Asset/AssetManager.h"
#include "Utils/Typo/TypoGlyphMap.h"
#include "WindowManagement/GLWindow.h"

using namespace glm;
using namespace std;

// NOTE: FBOs and VAOs can't be shared since they don't contain actual data

namespace ara {

GLBase::GLBase() {
    m_textureCollector.init(this);
    m_winMan = std::make_unique<WindowManager>(this);
}

/// <summary>
/// init a GL context with standard resources
/// </summary>
bool GLBase::init(const bool doInitResources, void *winHnd) {
    if (m_inited) {
        return true;
    }
    m_mainThreadId = this_thread::get_id();

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    m_winMan->setMainThreadId(m_mainThreadId);
    if (m_selfManagedCtx) {
        m_win = m_winMan->addWin(glWinPar{
            .createHidden = true,
            .hidInput     = false,
            .size         = { 5, 5 },
            .shareCont    = winHnd,
#ifdef __ANDROID__
            .contScale   = { m_androidDensity , m_androidDensity }
#endif
        });
    }
#elif _WIN32
    // create an invisible window with a valid gl context which will contain the resources and be shared to all context
    // that are created afterward
    createCtx();
#endif

    initToThisCtx();
    checkCapabilities();
    if (doInitResources) {
        initResources();
    }

    GLWindow::makeNoneCurrent();

    m_inited = true;
    return m_win != nullptr;
}

/// <summary>
/// check for required version, if not available set fallback switch -> OpenGL 3.2 compatibility
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

    glGetIntegerv(GL_MAJOR_VERSION, &m_caps.major_vers);
    glGetIntegerv(GL_MINOR_VERSION, &m_caps.minor_vers);

#if defined(ARA_USE_EGL) || defined(ARA_USE_GLES31)
    LOG << " Using GLES Version " << m_caps.major_vers << "." << m_caps.minor_vers;
#endif

    std::stringstream ss;
#if defined(ARA_USE_EGL) || defined(ARA_USE_GLES31)
    // note: the precision qualifiers are necessary for GLES!!!
    ss << "#version "
        << std::to_string(m_caps.major_vers)
        << std::to_string(m_caps.minor_vers > 10 ? m_caps.minor_vers / 10 : m_caps.minor_vers)
        << std::to_string(m_caps.minor_vers > 10 ? m_caps.minor_vers % 10 : 0)
        << " es\n#extension GL_EXT_shader_io_blocks : enable\nprecision highp float;\nprecision highp sampler3D;\n";
#else
    ss << "#version "
        << std::to_string(m_caps.major_vers)
        << std::to_string(m_caps.minor_vers > 10 ? m_caps.minor_vers / 10 : m_caps.minor_vers)
        << std::to_string(m_caps.minor_vers > 10 ? m_caps.minor_vers % 10 : 0)
        << "\n";
#endif
    setShaderHeader(ss.str());

    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &m_caps.max_tex_units);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_caps.max_tex_size);
#ifndef ARA_USE_GLES31
#ifndef __APPLE__
    glGetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS, &m_caps.max_framebuffer_layers);
    glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &m_caps.max_compute_shader_block);
#endif
    glGetIntegerv(GL_MAX_GEOMETRY_SHADER_INVOCATIONS, &m_caps.max_shader_invoc);
    glGetIntegerv(GL_MULTISAMPLE, &m_caps.multisample);
#else
    glGetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS_EXT, &m_caps.max_framebuffer_layers);
    glGetIntegerv(GL_MAX_GEOMETRY_SHADER_INVOCATIONS_EXT, &m_caps.max_shader_invoc);
#endif
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &m_caps.max_nr_layers);
    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &m_caps.max_nr_attachments);
    glGetIntegerv(GL_MAX_SAMPLES, &m_caps.max_nr_samples);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &m_caps.max_nr_drawbuffers);

#ifdef __APPLE__
    // on osx 10.15 intel HD 4000 there is a problem with mimap generation ...
    // simply doesn't work so disable it by force
    std::string renderer((char *)glGetString(GL_RENDERER));
    std::size_t found = renderer.find("Intel");
    if (found != std::string::npos) {
        m_isIntelRenderer    = true;
        m_caps.max_tex_level = 1;
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

    m_shaderCollector.getStdCol();  // init std col shader
    m_shaderCollector.getStdTex();  // init std tex shader
    m_nativeCtx = getGLCtx();
    glGenVertexArrays(1, &m_nullVao);

    glFinish();
}

/// <summary>
/// load common resources form disk (fonts, icons, etc.) Done file by file in debug mode and from res_comp in Release mode
/// </summary>
void GLBase::initResources() {
    if (!m_assetManager) {
        if (m_resRootPath.empty()) {
            setResRootPath("resdata"); // default path
        }
        m_assetManager = make_unique<AssetManager>(m_resRootPath, "res_comp", this);
        if (m_assetManager->load(m_resFile)) {
                LOG << "[OK] GLBase Resource file " << m_resRootPath + "/" + m_resFile << " loaded. "
              << (AssetManager::usingComp() ? " Used compiled binary asset file" : "");
        } else {
            for (auto &[lineIndex, errorString] : m_assetManager->getRoot()->errList) {
                LOGE << "Line " << lineIndex + 1 << " err:" << errorString;
            }
        }

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
#if defined(__ANDROID__) && !defined(ARA_ANDROID_PURE_NATIVE_APP)
#else
        getWinMan()->setAssetManager(m_assetManager.get());
#endif
#ifdef ARA_USE_GLFW
        if (m_loadMouseCursorIcons) {
            getWinMan()->loadMouseCursors();
        }
#endif

        if (!AssetManager::usingComp() && m_continousChangeCheck) {
            startContinousCheck();
        }

#endif
    }
}

void GLBase::startContinousCheck() {
#ifndef __ANDROID__
    m_updtResCb.emplace_back([this] {
        if (m_assetManager) {
            m_assetManager->callResSourceChange();
            m_assetManager->callForChangesInFolderFiles();
        }
    });

    m_resUpdt = std::jthread([this] (const std::stop_token& stopToken){
        while (!stopToken.stop_requested()) {
            checkResourceChanges();
            for (size_t i=0; i<16; ++i) {
                this_thread::sleep_for(chrono::milliseconds(50));
                if (stopToken.stop_requested()) {
                    break;
                }
            }
        }

        clearGlCbQueue();
        m_resUpdtExited.notify();
    });

    m_resUpdt.detach();
#endif
}

void GLBase::checkResourceChanges() const {
    if (m_assetManager->checkForChangesInFolderFiles()) {
        const auto win = getWinMan()->getFocusedWin();
#ifdef ARA_USE_GLFW
        // style updating must be sync with the gl loop. get the actually focused window and push the update to its gl-queue
        if (win) {
            win->setGlCb([&] {
#endif
                for (auto& cb : m_updtResCb) {
                    cb();
                }
#ifdef ARA_USE_GLFW
            });
        } else {
            for (auto& cb : m_updtResCb) {
                cb();
            }
        }
#endif
    }
}

/// <summary>
/// init standard resources in existing context
/// </summary>
void GLBase::destroy(const bool terminateGLFW) {
    if (!m_inited) {
        return;
    }

    if (m_glCallbackLoopRunning) {
        stopProcCallbackLoop();
    }
    m_glCallbacks.clear();

    // gl render loop for sure is stopped at this point, that means also the gl context is unbound
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    if (m_selfManagedCtx && m_win) {
        m_win->makeCurrent();
    }
#endif
    // remove all gl resources, must be done on a valid gl context/ g_stdQuad.reset();
    m_shaderCollector.clear();
    glDeleteVertexArrays(1, &m_nullVao);

    if (auto source = m_resUpdt.get_stop_source(); source.stop_possible()) {
        auto r = source.request_stop();
        m_resUpdtExited.wait();
    }

    m_assetManager.reset();
    m_assetManager = nullptr;

    m_textureCollector.clear();

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    if (m_selfManagedCtx && m_win) {
        GLWindow::makeNoneCurrent();
        m_winMan->removeWin(m_win, terminateGLFW);
    }
#elif _WIN32
    destroyCtx();
#endif
    m_inited      = false;
    m_checkedCaps = false;
}

/// <summary>
/// create an individual context with local basic resources
/// </summary>
#ifdef ARA_USE_GLFW
unique_ptr<GLWindow> GLBase::createOpenGLCtx(const bool initGLFW) {
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
    if (!m_glCallbackLoopRunning) {
        // start a separate thread for processing
        m_glCallbackLoop = std::thread([this] { glCallbackLoop(); });
        m_glCallbackLoop.detach();

        // wait for loop to be running
        m_glCallbackLoopRunningSem.wait();
        m_glCallbackLoopRunningSem.reset();
    }
}

void GLBase::glCallbackLoop() {
#ifdef ARA_USE_GLFW
    // initially make the GLBase context current
    if (!m_win) {
        return;
    }
    m_win->makeCurrent();
#elif _WIN32
    if (!m_hdc || !m_hRC) {
        LOGE << "m_glbase.renderLoop Error, context not valid";
        return;
    }

    if (!wglMakeCurrent(m_hdc, m_hRC)) LOGE << "m_glbase.renderLoop Error, could not make current context " << m_hRC;
#endif

    m_glCallbackLoopRunningSem.notify();  // wait until another thread is waiting for this notify
    m_glCallbackLoopRunning = true;

    while (m_glCallbackLoopRunning) {
        m_sema.wait(0);  // wait infinitely
        m_mtx.lock();
        iterateGlCallback();
        m_mtx.unlock();
        glFinish();
    }

    m_glCallbackLoopRunning = false;
#ifdef ARA_USE_GLFW
    glfwMakeContextCurrent(nullptr);
#elif _WIN32
    wglMakeCurrent(nullptr, nullptr);
#endif

    m_loopExit.notify();
}

void GLBase::iterateGlCallback() {
    for (auto it = m_glCallbacks.begin(); it != m_glCallbacks.end();) {
        if (it->first()) {
            if (it->second) {
                it->second->notify();
            }
            it = m_glCallbacks.erase(it);
        } else {
            ++it;
        }
    }
}

void GLBase::stopProcCallbackLoop() {
    if (m_glCallbackLoopRunning) {
        m_glCallbackLoopRunning = false;
        m_sema.notify();  // notify in case the loop is waiting;
        m_loopExit.wait(0);
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

    // create a message window for receiving the windows messages from the core
    // (WS_VISIBLE not set and no ShowWindow() call)
    string name = "GLBaseWin";
    m_hWnd      = CreateWindowA(m_wglClassName.c_str(), name.c_str(), WS_POPUPWINDOW, 0, 0, 10,
                                10,  // ...anything, the window is invisible
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
    m_contexts.emplace_back(ctx);
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
    m_contexts.remove_if([ctx](const void *lc) { return lc == ctx; });
}

#if !defined(ARA_USE_GLFW) && defined(_WIN32)
LRESULT CALLBACK m_glbase.WGLMessageHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hWnd, message, wParam, lParam);
}
#endif

void GLBase::shareCtx() const {
    if (!m_contexts.empty()) {
        auto c = getGLCtx();
#ifdef _WIN32
        wglShareLists(static_cast<HGLRC>(m_contexts.front()), static_cast<HGLRC>(c.ctx));
#endif
    }
}

void GLBase::addEvtCb(const std::function<bool()> &func, const bool forcePush) const {
#ifdef __ANDROID__
    func();
#else
    if (std::this_thread::get_id() == m_mainThreadId && !forcePush) {
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
    std::unique_lock<std::mutex> lock(m_mtx);
    m_glCallbacks.emplace_back(func, sema);
    if (sema) {
        m_sema.notify();  // in the worst case, is sent before g_sema is waiting, true flag wait until another thread
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
    glXMakeContextCurrent(static_cast<Display *>(ctx.deviceHandle), static_cast<GLXDrawable>(ctx.drawable),
                          static_cast<GLXDrawable>(ctx.drawable), static_cast<GLXContext>(ctx.ctx));
#endif
}

void GLBase::initAppMsg(const char *fontFile, const int fontHeight, int screenWidth, int screenHeight) {
    m_typoGlyphMap = make_unique<TypoGlyphMap>(screenWidth, screenHeight);
    m_typoGlyphMap->loadFont(fontFile, &m_shaderCollector);
    m_typoFontHeight = fontHeight;
}

void GLBase::clearGlCbQueue() {
    unique_lock lock(m_mtx);
    m_glCallbacks.clear();
}

void GLBase::setResRootPath(const std::string &str) {
#ifndef ARA_USE_CMRC
    m_resRootPath = (filesystem::current_path() / str).string();
#else
    m_resRootPath = str;
#endif
}

GLStateManager &GLBase::stateMan() {
    if (!m_inited) {
        initToThisCtx();
    }
    return m_stateMan;
}

void GLBase::makeCurrent() {
    if (!m_inited) {
        initToThisCtx();
    }
    m_win->makeCurrent();
}

Quad *GLBase::stdQuad() {
    if (!m_inited) {
        initToThisCtx();
    }
    return m_stdQuad.get();
}

bool GLBase::getUseFallback() {
    if (!m_inited) {
        init();
    }
    return m_useFallback;
}

int32_t GLBase::maxNrAttachments() {
    if (!m_inited) {
        initToThisCtx();
    }
    return m_caps.max_nr_attachments;
}

void GLBase::setAppMsgStaticInfoNumLines(const size_t count) {
    m_appMsgStaticNumLines = count;
    m_appMsgStatic.resize(count);
}

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
GLWindow *GLBase::getWin() {
    if (!m_inited) {
        init();
    }
    return m_win;
}
#ifdef ARA_USE_GLFW
GLContext GLBase::getGlfwHnd() {
    if (!m_inited) {
        init();
    }
    return m_win->getCtx();
}
#else
GLContext GLBase::getGlfwHnd() {
    return nullptr;
}
#endif
WindowManager *GLBase::getWinMan() const {
    return m_winMan.get();
}
#endif

}  // namespace ara
