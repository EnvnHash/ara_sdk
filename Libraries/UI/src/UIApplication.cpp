//
// Created by sven on 11/15/20.
//

#include <UIApplication.h>
#include <UIElements/UINodeBase/UINode.h>
#include <Utils/TextureCollector.h>

using namespace glm;
using namespace std;

namespace ara {

UIApplication::UIApplication() :
#if defined(__ANDROID__) && defined(ARA_ANDROID_PURE_NATIVE_APP)
      UIAppAndroidNative()
#elif defined(__ANDROID__) && !defined(ARA_ANDROID_PURE_NATIVE_APP)
      UIAppAndroidJNI()
#else
      UIApplicationBase()
#endif
{}

void UIApplication::initGLBase() {
#ifdef ARA_USE_GLBASE
    if (!m_glbase.isInited()) {
        m_glbase.init(m_initRes);

        // this is called from GLBase resource update thread, in order to not update Resource while they are used,
        // only a flag is set and a redraw forced. Styles will be updated during UINode::draw iteration this is only
        // needed in debug mode, when the resources can change during runtime
        if (m_glbase.getAssetManager() && !m_glbase.getAssetManager()->usingComp()) {
            m_glbase.setUpdtResCb([this] {
                for (const auto& it : m_uiWindows) {
                    it->setResChanged(true);
                    it->update();
                }
            });
        }
    }
#endif
}

void UIApplication::mainWinDefaultSetup() {
    m_mainWindow->setApplicationHandle(this);
    m_mainWindow->setEnableWindowResizeHandles(m_windowResizeHandlesEnabled);
    m_mainWindow->setEnableMenuBar(m_menuBarEnabled);
}

void UIApplication::init(std::function<void(UINode&)> initCb) {
    m_mainWindow = addWindow(UIWindowParams{
        .size           = m_winSize,
        .shift          = {100, 100},
        .osDecoration   = m_osWinDecoration,
        .transparentFB  = false,
        .multisample    = m_multisample,
#ifdef __ANDROID__
        .extWinHandle   = static_cast<void*>(m_androidNativeWin),
#endif
        .scaleToMonitor = m_scaleToMonitor,
        .initCb         = initCb,
    });

    mainWinDefaultSetup();
    startThreadedRendering();
    startGLBaseProcCallbackLoop();
    m_inited = true;
}

void UIApplication::initSingleThreaded(const std::function<void()>& initCb) {
    m_threadedWindowRendering = false;

    m_mainWindow = addWindow(UIWindowParams{
        .size           = m_winSize,
        .shift          = {100, 100},
        .osDecoration   = m_osWinDecoration,
        .transparentFB  = false,
        .multisample    = m_multisample,
        .scaleToMonitor = m_scaleToMonitor
    });

    mainWinDefaultSetup();

    // deactivate the context of the newly created window, otherwise it will be blocked by the main thread and the
    // guiThread won't be able to make it current
    GLWindow::makeNoneCurrent();

    startGLBaseProcCallbackLoop();
    initThread(initCb);

    m_inited = true;
}

void UIApplication::startSingleUiThread(const std::function<void()>& initCb) {
    m_guiThread = std::thread([this, &initCb] {
        initThread(initCb);
    });
    m_guiThread.detach();
    m_initSema.wait(0);
}

void UIApplication::initThread(const std::function<void()>& initCb) {
    m_mainWindow->makeCurrent();
    m_run = true;

    glViewport(0, 0, static_cast<GLsizei>(m_mainWindow->getWidthReal()), static_cast<GLsizei>(m_mainWindow->getHeightReal()));
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (initCb) {
        initCb();
        // just in case there UIWindow::draw() is called in the initCb, be sure that for the next drawing iteration
        // Draw and ObjMap procsteps are true
        m_mainWindow->getProcSteps()->at(Draw).active = true;
        mainWinIterate();
    }

    while (m_run) {
        if (m_initSignaled) {
            m_iterate.wait(0);
        }

        windowManagerIterate();

        if (!m_initSignaled) {
            m_initSema.notify();
            m_initSignaled = true;
        }
    }

    GLWindow::makeNoneCurrent();

    m_initSignaled = false;
    m_loopExitSema.notify();
}

void UIApplication::startRenderLoop() {
    m_glbase.startGlCallbackProcLoop();
    m_mainThreadId = this_thread::get_id();
    m_inited       = true;
    startThreadedRendering();
}

void UIApplication::openInfoDiag(const InfoDiagParams& params) {
    m_glbase.runOnMainThread([this, params] {
        // check if another info dialog is open, if this is the case, close it
        if (m_infoDiag) {
            m_infoDiag->setRemoveCb(nullptr);  // avoid removeWindow to be called twice
            auto ptr   = m_infoDiag;
            m_infoDiag = nullptr;
            ptr->addCloseEvent(nullptr);  // frees GLFWWindow instance in GLFWWindowManager
            removeWindow(ptr);            // remove the dialog from the m_uiWindow vector -> calls dtor
        }

        // create an info dialog window center above the main Window
        m_infoDiag = addWindow<InfoDialog>(UIWindowParams{.size = params.size, .shift = params.pos });
        m_infoDiag->setApplicationHandle(this);
        m_infoDiag->setModal(params.isModal);
        m_infoDiag->setInfoMsg(params.msg);
        m_infoDiag->setConfirmCb(params.onConfirm);
        m_infoDiag->setCloseCb(params.onClose);
        m_infoDiag->setCancelCb(params.onCancel);
        m_infoDiag->setMinStayTime(static_cast<int>(params.minStayTime));
        m_infoDiag->setRemoveCb([this] {
            m_glbase.runOnMainThread([this] {
                if (m_infoDiag) {
                    const auto ptr   = m_infoDiag;
                    m_infoDiag = nullptr;
                    removeWindow(ptr);
                }
                return true;
            });
        });

        // m_infoDiag->setType manipulated the m_setStyleFunc, so must be done sync with gl loop
        m_infoDiag->addGlCb(this, "setTp", [this, &params] {
            m_infoDiag->setType(params.tp);  // causes a rebuildCustomStyle
            return true;
        });

        GLWindow::makeNoneCurrent();
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
        m_infoDiag->startRenderLoop();
#endif
        if (m_infoDiagCreatedCb) {
            m_infoDiagCreatedCb();
            m_infoDiagCreatedCb = nullptr;
        }
        return true;
    });
}

void UIApplication::showInfo(const std::string& msg, long minStayTime, int width, int height, bool isModal,
                             std::function<void()> onClose, std::function<void()> onInfoOpen) {
    m_infoDiagCreatedCb = std::move(onInfoOpen);

    ivec2 diagPos{0, 0};
    if (!m_uiWindows.empty()) {
        diagPos.x = (static_cast<int32_t>(m_uiWindows.front()->getWidth()) - width) / 2 + m_uiWindows.front()->getPosition().x;
        diagPos.y = (static_cast<int32_t>(m_uiWindows.front()->getHeight()) - height) / 2 + m_uiWindows.front()->getPosition().y;
    }

    openInfoDiag(InfoDiagParams{
        .pos = diagPos,
        .size = {width, height},
        .tp = infoDiagType::info,
        .msg = msg,
        .minStayTime =  minStayTime,
        .isModal = isModal,
        .onClose = std::move(onClose)
    });
}

void UIApplication::showCancel(std::string msg, long minStayTime, int width, int height, bool isModal,
                               std::function<bool()> cancelCb) {
    ivec2 diagPos{0, 0};
    if (!m_uiWindows.empty()) {
        diagPos.x = (static_cast<int32_t>(m_uiWindows.front()->getWidth()) - width) / 2 + m_uiWindows.front()->getPosition().x;
        diagPos.y = (static_cast<int32_t>(m_uiWindows.front()->getHeight()) - height) / 2 + m_uiWindows.front()->getPosition().y;
    }

    openInfoDiag(InfoDiagParams{
        .pos = diagPos,
        .size = {width, height},
        .tp = infoDiagType::cancel,
        .msg = std::move(msg),
        .minStayTime =  minStayTime,
        .isModal = isModal,
        .onCancel = std::move(cancelCb)
    });
}

void UIApplication::openInfoDiag(infoDiagType tp, const std::string& msg, const std::function<bool()>& onConfirm) {
    ivec2 diagPos{0, 0};
    ivec2 diagSize{750, 150};

    if (!m_uiWindows.empty()) {
        diagPos = (m_uiWindows.front()->getSize() - diagSize) / 2 + m_uiWindows.front()->getPosition();
    }

    openInfoDiag(InfoDiagParams{
        .pos = diagPos,
        .size = diagSize,
        .tp = infoDiagType::cancel,
        .msg = msg,
        .minStayTime =  500,
        .isModal = true,
        .onConfirm = onConfirm
    });
}

void UIApplication::setActiveModalWin(UIWindow *win) {
    if (win) {
        for (const auto &it : m_uiWindows | views::filter([win](auto& i) { return i && i.get() != win; })) {
            it->setBlockHid(true);
        }
    } else {
        for (const auto &it : m_uiWindows | views::filter([](auto& i) { return i != nullptr; })) {
            it->setBlockHid(false);
        }
    }
}

void UIApplication::startEventLoop() {
#ifdef ARA_USE_GLFW
    m_glbase.getWinMan()->startEventLoop();
#endif
}

std::filesystem::path UIApplication::dataPath() {
    return m_mainWindow ? m_mainWindow->getSharedRes()->dataPath : std::filesystem::current_path();
}

void UIApplication::closeInfoDiag() {
    m_glbase.runOnMainThread([this] {
        if (m_infoDiag) {
            m_infoDiag->close();
        } else {
            // there may be situation where the closeInfoDiag will reach even
            // before the window is created
            m_infoDiagCreatedCb = [this] {
                if (m_infoDiag) {
                    m_infoDiag->close();
                }
            };
        }
        return true;
    });
}

void UIApplication::stop() {
    m_run = false;
    m_iterate.notify();
    m_loopExitSema.wait(0);
#ifdef ARA_USE_GLFW
    m_glbase.getWinMan()->stopEventLoop();
#endif
}

void UIApplication::exit() {
    if (!m_threadedWindowRendering) {
        m_run = false;
        m_iterate.notify();
        m_loopExitSema.wait(0);
    }

    // close windows and immediately remove them from the queue
    for (auto win = m_uiWindows.begin(); win != m_uiWindows.end();) {
        win->get()->close(true);
        win = m_uiWindows.erase(win);
    }

    stopThreadedRendering();
    stopGLBaseProcCallbackLoop();

    destroyGLBase();
    m_exitSema.notify();
}

}  // namespace ara
