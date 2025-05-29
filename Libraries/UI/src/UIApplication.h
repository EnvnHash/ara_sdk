//
// Created by sven on 11/15/20.
//

#pragma once

#include <Asset/AssetManager.h>
#include <WindowManagement/WindowManager.h>
#include <Dialoges/InfoDialog.h>
#include <Property.h>
#include "UIApplicationBase.h"
#include <UIWindow.h>
#include <UIElements/WindowElements/MenuBar.h>

#if defined(__ANDROID__) && defined(ARA_ANDROID_PURE_NATIVE_APP)
#include "Android/UIAppAndroidNative.h"
#elif defined(__ANDROID__) && !defined(ARA_ANDROID_PURE_NATIVE_APP)
#include "Android/UIAppAndroidJNI.h"
#endif

namespace ara {

class UIApplication :
#if defined(__ANDROID__) && defined(ARA_ANDROID_PURE_NATIVE_APP)
    public UIAppAndroidNative
#elif defined(__ANDROID__) && !defined(ARA_ANDROID_PURE_NATIVE_APP)
    public UIAppAndroidJNI
#else
    public UIApplicationBase
#endif
{
public:
    UIApplication();
    ~UIApplication() override = default;

    void init(std::function<void(UINode&)> initCb) override;
    void initSingleThreaded(const std::function<void()>& initCb);
    void initGLBase() override;
    void startSingleUiThread(const std::function<void()>& f);
    void initThread(const std::function<void()>& initCb);
    void mainWinDefaultSetup();

    template <class T>
    T* addWindow(const UIWindowParams& params) {
        initGLBase();

        UIWindowParams p = params;
        p.glbase        = &m_glbase;
#if defined(__ANDROID__) && defined(ARA_ANDROID_PURE_NATIVE_APP)
        p.extWinHandle = static_cast<void*>(m_androidNativeWin);
#endif

        m_uiWindows.emplace_back(std::make_unique<T>(p));
        auto newWindow = m_uiWindows.back().get();
        if (newWindow->getProcSteps()) {
            newWindow->getProcSteps()->at(Draw).active = true;
        }

        newWindow->setApplicationHandle(this);
        if (m_uiWindows.size() == 1) {
            newWindow->setToMainWindow();
            m_mainWindow = newWindow;
        }

        return dynamic_cast<T*>(m_uiWindows.back().get());
    }

    UIWindow* addWindow(const UIWindowParams& params) {
        return addWindow<UIWindow>(params);
    }

    // called from UIWindow::close() when the removeFromApp bool is set
    void removeWindow(UIWindow* win) {
        std::erase_if(m_uiWindows, [win](const auto& it){ return it.get() == win; });
    }

    // utility function for connecting to properties. stores a local callback function add passes it as to the property
    // which will store a reference to it as a weak pointer on dtor the referring pointer is freed and by
    // weak_ptr.lock() checking it's weak_ptr reference inside the Property is deleted implicitly
    template <typename T>
    void onChanged(Property<T>* p, std::function<void(std::any)> f) {
        m_onValChangedCb[p] = std::make_shared<std::function<void(std::any)>>(f);
        if (p != nullptr) {
            p->onPreChange(m_onValChangedCb[p]);
        }
    }

    /// \brief initialization function for multithreaded rendering. This calls the WindowManager's
    /// startThreadedRendering function, which will start an individual drawing thread for each window
    virtual void startRenderLoop();

    // Info dialoges
    virtual void openInfoDiag(const InfoDiagParams& params);
    virtual void openInfoDiag(infoDiagType tp, const std::string& msg, const std::function<bool()>& onConfirm);
    virtual void showInfo(const std::string& msg, long minStayTime = 500, int width = 250, int height = 100,
                          bool isModal = true, std::function<void()> onClose = nullptr,
                          std::function<void()> onInfoOpen = nullptr);
    virtual void showCancel(std::string msg, long minStayTime, int width, int height, bool isModal, std::function<bool()> cancelCb);
    virtual void closeInfoDiag();

    UIWindow* getInfoDiag() const { return m_infoDiag; }
    void      waitForExit() { m_exitSema.wait(0); }
    void      startEventLoop(); // run a pure event loop inside WindowManager, -> event will be distributed to all registered windows

    virtual void    setActiveModalWin(UIWindow* win);
    void            setEnableMenuBar(const bool val) { m_menuBarEnabled = val; }
    void            setEnableWindowResizeHandles(const bool val) { m_windowResizeHandlesEnabled = val; }
    void            setMultisample(const bool val) { m_multisample = val; }
    void            setInitRes(const bool val) { m_initRes = val; }
    void            setOsWindowDecoration(const bool val) { m_osWinDecoration = val; }
    void            setRun(const bool val) { m_run = val; }
    void            setDataModel(void* model) { m_dataModel = model; }
    void            setScaleToMonitor(const bool val) { m_scaleToMonitor = val; }

    virtual std::filesystem::path dataPath();

    UINode*     getRootNode(const uint winIdx = 0) const { return (m_uiWindows.size() > winIdx) ? m_uiWindows[winIdx]->getRootNode() : nullptr; }
    UIWindow*   getWinBase(const uint winIdx = 0) const { return m_uiWindows.size() > winIdx ? m_uiWindows[winIdx].get() : nullptr; }
    auto        getMainWindow() const { return m_mainWindow; }
    auto        getUIWindows() { return &m_uiWindows; }
    auto        getDataModel() const { return m_dataModel; }

    void stop();            /// for single threaded mode
    virtual void exit();    /// exit the application, must called from the main thread in multi-thread setups

protected:
    std::thread                                                               m_guiThread;
    Conditional                                                               m_exitSema;
    Conditional                                                               m_loopExitSema;
    std::function<void()>                                                     m_infoDiagCreatedCb;
    std::unordered_map<void*, std::shared_ptr<std::function<void(std::any)>>> m_onValChangedCb;

    std::vector<std::unique_ptr<UIWindow>> m_uiWindows;
    InfoDialog*                            m_infoDiag = nullptr;
    std::thread::id                        m_mainThreadId{};

    double dt          = 1.0;
    void*  m_dataModel = nullptr;
    bool   m_scaleToMonitor = false;
};

}  // namespace ara
