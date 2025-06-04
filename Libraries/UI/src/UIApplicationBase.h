//
// Created by sven on 05-07-22.
//

#pragma once

#include "UIWindow.h"

#ifdef ARA_USE_GLBASE
#include <GLBase.h>
#endif

namespace ara {

class UIApplicationBase {
public:
    virtual ~UIApplicationBase() = default;
    virtual void init(std::function<void(UINode&)>) = 0;
    virtual void resume() {}

    virtual void initGLBase();
    /// start the global opengl processing loop in a separate thread
    void startGLBaseProcCallbackLoop();
    /// stops the global opengl processing loop
    void stopGLBaseProcCallbackLoop();
    /// destroy the global opengl context and all its resources
    void destroyGLBase();
    /// cause a draw loop iteration on the main window
    void mainWinIterate() const;
    /// causes a draw loop iteration on the global WindowManager
    void windowManagerIterate();
    /// calls startThreadedRendering and startEventLoop on the windowManager when GLFW or EGL is used, otherwise does nothing
    void startThreadedRendering();
    /// stops the uiwindow rendering threads and the eventLoop on the windowManager when GLFW or EGL is used, otherwise does nothing
    void stopThreadedRendering();

    virtual void startSingleThreadUiRendering(const std::function<void(UINode*)>& initCb) {}

    /// get a reference to the WindowManager
    auto getWinMan() { return m_glbase.getWinMan(); }
    /// get a reference to GLBase instance containing all opengl related shared resources
    auto getGLBase() { return &m_glbase; }
    auto getMainWindow() const { return m_mainWindow; }

    virtual void update() { m_iterate.notify(); }

    void setWinWidth(int w) { m_winSize.x = w; }
    void setWinHeight(int g) { m_winSize.y = g; }
    void setWinSize(const glm::ivec2& s) { m_winSize = s; }
    void setRunFlag(bool val) { m_run = val; }

public:
    std::string m_internalPath;
    std::string m_externalPath;

protected:
    bool m_run                        = false;
    bool m_inited                     = false;
    bool m_initRes                    = true;
    bool m_initSignaled               = false;
    bool m_threadedWindowRendering    = true;
    bool m_menuBarEnabled             = true;
    bool m_windowResizeHandlesEnabled = true;
    bool m_osWinDecoration            = false;
    bool m_multisample                = false;

    Conditional m_initSema;
    UIWindow*   m_mainWindow = nullptr;
    Conditional m_iterate;
    glm::ivec2  m_winSize = { 1280, 720 };

#ifdef ARA_USE_GLBASE
    GLBase m_glbase;
#endif
};

}  // namespace ara