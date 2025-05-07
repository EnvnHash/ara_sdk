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

    /** this is meant for a simple one window setup. An initCb can be passed,
     * that is supposed to configure the ui-scenegraph UI rendering is done in a
     * separate thread, though this function is non-blocking For more complex
     * applications it is recommended to overload this function and decide based
     * on the application whether rendering should be done threaded or running
     * as a blocking loop in the main-thread
     */
    virtual void init(std::function<void()> func) = 0;

    void initGLBase();
    /// start the global opengl processing loop in a separate thread
    void startGLBaseRenderLoop();
    /// stops the global opengl processing loop
    void stopGLBaseRenderLoop();
    /// destroy the global opengl context and all its resources
    void destroyGLBase();
    /// cause a draw loop iteration on the main window
    void mainWinIterate() const;
    /// causes a draw loop iteration on the glbase gl context
    void glBaseIterate();
    /// calls startThreadedRendering and startEventLoop on the windowManager when GLFW or EGL is used, otherwise does nothing
    void startThreadedRendering();
    /// stops the uiwindow rendering threads and the eventLoop on the windowManager when GLFW or EGL is used, otherwise does nothing
    void stopThreadedRendering();

    /// get a reference to the WindowManager
    WindowManager* getWinMan() { return m_glbase.getWinMan(); }
    /// get a reference to GLBase instance containing all opengl related shared resources
    GLBase* getGLBase() { return &m_glbase; }

    void setWinWidth(int w) { winWidth = w; }
    void setWinHeight(int g) { winHeight = g; }

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

    int winWidth  = 1280;
    int winHeight = 720;

    Conditional m_initSema;
    UIWindow*   m_mainWindow = nullptr;

#ifdef ARA_USE_GLBASE
    GLBase m_glbase;
#endif
};

}  // namespace ara