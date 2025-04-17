//
// Created by sven on 05-07-22.
//

#include "UIApplicationBase.h"

using namespace std;

namespace ara {

void UIApplicationBase::initGLBase() {
#ifdef ARA_USE_GLBASE
    if (!m_glbase.isInited()) {
        m_glbase.init(m_initRes);
    }
#endif
}

void UIApplicationBase::startGLBaseRenderLoop() {
#ifdef ARA_USE_GLBASE
    m_glbase.startRenderLoop();
#endif
}

void UIApplicationBase::stopGLBaseRenderLoop() {
#ifdef ARA_USE_GLBASE
    m_glbase.stopRenderLoop();
#endif
}

void UIApplicationBase::destroyGLBase() {
#ifdef ARA_USE_GLBASE
    m_glbase.destroy(true);
#endif
}

void UIApplicationBase::mainWinIterate() {
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    m_mainWindow->getWinHandle()->iterate();
#endif
}

void UIApplicationBase::glBaseIterate() {
#if (defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)) && defined(ARA_USE_GLBASE)
    m_glbase.getWinMan()->iterate(true);  // iterate once on start then only after new event
#endif
}

void UIApplicationBase::startThreadedRendering() {
#if (defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)) && defined(ARA_USE_GLBASE) && !defined(__ANDROID__)
    // start GLFWWindowManager threaded rendering -> blocking
    m_glbase.getWinMan()->startThreadedRendering();
    m_glbase.getWinMan()->startEventLoop();
#endif
}

void UIApplicationBase::stopThreadedRendering() {
#if (defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)) && defined(ARA_USE_GLBASE)
    if (m_threadedWindowRendering) m_glbase.getWinMan()->stopThreadedRendering();  // exit GLFWWindow::runLoop

    m_glbase.getWinMan()->setBreakEvtLoop(true);  // avoid the event queue to iterate further
    m_glbase.getWinMan()->stopEventLoop();        // we are still in the eventqueue and thus can't
                                                  // wait for it to quit,
    // so this will just set a bool flag, to make the queue quit, after this is
    // done
#endif
}

}  // namespace ara