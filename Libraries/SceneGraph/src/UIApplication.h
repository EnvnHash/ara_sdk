//
// Created by sven on 11/15/20.
//

#pragma once

#include <WindowManagement/WindowManager.h>

#include <Dialoges/InfoDialog.h>
#include "UIApplicationBase.h"

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
    virtual ~UIApplication() = default;

    virtual void init(std::function<void()> func);

    /// \brief initialization function for sequential rendering. A loop which calls the WindowManager's iterate function,
    /// which will call all window's draw functions on after another
    virtual void initThread(const std::function<void()>& initCb);

    template <class T>
    T* addWindow(const UIWindowParams& params) {
#ifndef ARA_USE_EGL
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
        // no context bound at this point

        UIWindowParams p = params;
        p.glbase        = &m_glbase;
#if defined(__ANDROID__) && defined(ARA_ANDROID_PURE_NATIVE_APP)
        p.extWinHandle = static_cast<void*>(m_androidNativeWin);
        m_uiWindows.emplace_back(std::make_unique<T>(p));
#elif defined(__ANDROID__) && !defined(ARA_ANDROID_PURE_NATIVE_APP)
        m_uiWindows.emplace_back(std::make_unique<T>(p));
#else
        m_uiWindows.emplace_back(std::make_unique<T>(p));
#endif

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
        auto ptr = std::find_if(m_uiWindows.begin(), m_uiWindows.end(),
                                [win](const std::unique_ptr<UIWindow>& w) { return w.get() == win; });

        if (ptr != m_uiWindows.end()) {
            m_uiWindows.erase(ptr);
        }
    }

    // utility function for connecting to properties. stores a local callback function add passes it as to the property
    // which will store a reference to it as a weak pointer on dtor the referring pointer is freed and by
    // weak_ptr.lock() checking it's weak_ptr reference inside the Property is deleted implicitly
    template <typename T>
    void onChanged(Property<T>* p, std::function<void(std::any)> f) {
        m_onValChangedCb[p] = std::make_shared<std::function<void(std::any)>>(f);
        if (p) {
            p->onPreChange(m_onValChangedCb[p]);
        }
    }

    /// \brief initialization function for multithreaded rendering. This calls the WindowManager's
    /// startThreadedRendering function, which will start an individual drawing thread for each window
    virtual void startRenderLoop();
    virtual void update() { m_iterate.notify(); }

    // Info dialoges
    virtual void openInfoDiag(const InfoDiagParams& params);
    virtual void openInfoDiag(infoDiagType tp, const std::string& msg, const std::function<bool()>& onConfirm);
    virtual void showInfo(const std::string& msg, long minStayTime = 500, int width = 250, int height = 100,
                          bool isModal = true, std::function<void()> onClose = nullptr,
                          std::function<void()> onInfoOpen = nullptr);
    virtual void showCancel(std::string msg, long minStayTime, int width, int height, bool isModal, std::function<bool()> cancelCb);
    virtual void closeInfoDiag();

    UIWindow* getInfoDiag() { return m_infoDiag; }
    void      waitForExit() { m_exitSema.wait(0); }
    void      startEventLoop(); // run a pure event loop inside WindowManager, -> event will be distributed to all registered windows

    virtual void    setActiveModalWin(UIWindow* win);
    void            setEnableMenuBar(bool val) { m_menuBarEnabled = val; }
    void            setEnableWindowResizeHandles(bool val) { m_windowResizeHandlesEnabled = val; }
    void            setMultisample(bool val) { m_multisample = val; }
    void            setInitRes(bool val) { m_initRes = val; }
    void            setOsWindowDecoration(bool val) { m_osWinDecoration = val; }
    void            setRun(bool val) { m_run = val; }
    void            setDataModel(void* model) { m_dataModel = model; }
    void            setScaleToMonitor(bool val) { m_scaleToMonitor = val; }

    virtual std::filesystem::path dataPath();
    virtual UINode* getRootNode(uint winIdx = 0) { return (m_uiWindows.size() > winIdx) ? m_uiWindows[winIdx]->getRootNode() : nullptr; }
    virtual UIWindow* getWinBase(uint winIdx = 0) { return m_uiWindows.size() > winIdx ? m_uiWindows[winIdx].get() : nullptr; }

    UIWindow*                               getMainWindow() { return m_mainWindow; }
    std::vector<std::unique_ptr<UIWindow>>* getUIWindows() { return &m_uiWindows; }
    void*                                   getDataModel() { return m_dataModel; }

    void stop();            /// for single threaded mode
    virtual void exit();    /// exit the application, must called from the main thread in multi-thread setups

protected:
    std::thread                                                               m_guiThread;
    Conditional                                                               m_iterate;
    Conditional                                                               m_exitSema;
    Conditional                                                               m_loopExitSema;
    std::function<void()>                                                     m_infoDiagCreatedCb;
    std::unordered_map<void*, std::shared_ptr<std::function<void(std::any)>>> m_onValChangedCb;

    std::vector<std::unique_ptr<UIWindow>> m_uiWindows;
    InfoDialog*                            m_infoDiag = nullptr;
    std::thread::id                        m_mainThreadId;

    double dt          = 1.0;
    void*  m_dataModel = nullptr;
    bool   m_scaleToMonitor = false;
};

}  // namespace ara
