//
// Created by Sven Hahne on 09.10.2020.
//

#include "UIWindow.h"

#include <Windows/WindowsMousePos.h>
#include <X11/X11MousePos.h>

#include "UIApplication.h"
#include "Windows/DisplayScaling.h"

using namespace glm;
using namespace std;

namespace fs = std::filesystem;
namespace ph = std::placeholders;

namespace ara {

UIWindow::UIWindow(const UIWindowParams& par)
    : WindowBase(),
    m_glbase(par.glbase),
    m_drawMan{make_unique<DrawManager>(par.glbase)},
#ifdef ARA_USE_GLES31
    m_multisample(false),
#else
    m_multisample(par.multisample),
#endif
    m_selfManagedCtx(!par.initToCurrentCtx) {
    uint32_t rWidth  = par.size.x;
    uint32_t rHeight = par.size.y;
    uint32_t vWidth  = par.size.x;
    uint32_t vHeight = par.size.y;

    bool restartGlBaseLoop = false;

    // init a UIWindow, using the GLFWWindowManager and GLbase in a separate thread
    if (!par.initToCurrentCtx) {
#ifndef ARA_USE_EGL
        // check if GLBase render loop is running, if this is the case, stop it and start it later again. Otherwise,
        // context sharing will fail
        if (m_glbase->isRunning()) {
            restartGlBaseLoop = true;
            m_glbase->stopRenderLoop();
        }

#endif
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
        m_winHandle = m_glbase->getWinMan()->addWin(glWinPar{
            .decorated = par.osDecoration,
            .floating = par.floating,
            .debug = m_debugGLFWwin,
            .shift = par.shift,
            .size = par.size,
            .scaleToMonitor = par.scaleToMonitor,
            .shareCont = static_cast<void *>(m_glbase->getGlfwHnd()),
            .transparentFramebuffer = par.transparentFB,
            .extWinHandle = par.extWinHandle,
        });

        if (!m_winHandle) {
            return;
        }

        m_winHandle->setDrawFunc([&](double time, double dt, int ctxNr) { return draw(time, dt, ctxNr); });

        // set hid callbacks. This must be done this way, as GlFW expects static c-style callbacks, which is not possible
        // when aiming for modular c++ style window architectures
        m_winHandle->addKeyCb([this](int p1, int p2, int p3, int p4) { key_callback(p1, p2, p3, p4); });
        m_winHandle->setCharCb([this](unsigned int p1) { char_callback(p1); });
        m_winHandle->setMouseButtonCb([this](int button, int action, int mods) { mouseBut_callback(button, action, mods); });
        m_winHandle->setMouseCursorCb([this](double x, double y) { cursor_callback(x, y); });
        m_winHandle->setWindowSizeCb([this](int w, int h) { window_size_callback(w, h); });
        m_winHandle->setScrollCb([this](double xOffs, double yOffs) { scroll_callback(xOffs, yOffs); });
        m_winHandle->setWindowPosCb([this](int posX, int posY) { window_pos_callback(posX, posY); });
        m_winHandle->setWindowMaximizeCb([this](int maximized) { window_maximize_callback(maximized); });
        m_winHandle->setWindowFocusCb([this](int focused) { window_focus_callback(focused); });
        m_winHandle->setWindowIconfifyCb([this](int iconified) { window_minimize_callback(iconified); });
        m_winHandle->setCloseCb([this] { window_close_callback(); });
        m_winHandle->setWindowRefreshCb([this] { window_refresh_callback(); });

        // double check if the requested size was accepted
        vWidth             = m_winHandle->getWidth();
        vHeight            = m_winHandle->getHeight();
        rWidth             = m_winHandle->getWidthReal();
        rHeight            = m_winHandle->getHeightReal();
        s_devicePixelRatio = m_winHandle->getContentScale().x;
#endif
#ifndef ARA_USE_EGL
    } else if (!m_glbase->isInited()) {
#endif
        // init GLBase to this context
        m_glbase->useSelfManagedCtx(false);

#ifndef __ANDROID__
        // resources updating must be done this way in case of non-window managed setup
        m_glbase->setUpdtResCb([this] {
            addGlCb(this, "resUpt", [this] {
                if (m_glbase->getAssetManager()) {
                    m_glbase->getAssetManager()->callResSourceChange();
                    m_glbase->getAssetManager()->callForChangesInFolderFiles();
                    setResChanged(true);
                    update();
                }
                return true;
            });
        });
#endif
        m_glbase->init();  // makes no context current
        m_winHandle->makeCurrent();
    }

#if defined(__ANDROID__) && !defined(ARA_ANDROID_PURE_NATIVE_APP)
    m_glbase->init();  // makes no context current
    s_devicePixelRatio = m_glbase->g_androidDensity;
#endif

    m_dataFolder = m_glbase->g_resRootPath;
    initGLEW();

    if (!s_inited) {
        WindowBase::init(0, 0, vWidth, vHeight);

        // create the FBO to draw the SceneGraph and it's object map
        if (m_multisample) {
            m_sceneFbo = make_unique<FBO>(FboInitParams{m_glbase, static_cast<int>(rWidth), static_cast<int>(rHeight), 1,
                                                        GL_RGBA8, GL_TEXTURE_2D_MULTISAMPLE, true, 1, 1, 2, GL_CLAMP_TO_EDGE, false});
        } else {
            m_sceneFbo = make_unique<FBO>(FboInitParams{m_glbase, static_cast<int>(rWidth), static_cast<int>(rHeight), 1,
                                                        GL_RGBA8, GL_TEXTURE_2D, true, 1, 1, 1, GL_CLAMP_TO_EDGE, false});
        }
        s_shCol.setShaderHeader(m_glbase->getShaderHeader());

        m_quad = make_unique<Quad>(QuadInitParams{
            .pos = {0.f, 0.f},
            .size = {1.f, 1.f},
            .color = { 1.f, 0.f, 0.f, 1.f }
        });
        m_normQuad = make_unique<Quad>(QuadInitParams{ .color = {1.f, 0.f, 0.f, 1.f} });
        glGenVertexArrays(1, &m_nullVao);

        m_objSel = make_unique<ObjectMapInteraction>(&s_shCol, rWidth, rHeight, m_sceneFbo.get(), m_multisample);
        m_drawMan->setMaxObjId((uint32_t)m_objSel->getMaxObjId());
        m_drawMan->setShaderCollector(&s_shCol);

        //-------------------------------------------------------------------------------------------

#ifdef ARA_USE_GLFW
        if (!par.initToCurrentCtx) {
            // pass the newly created cursors to the GLFWWindow handle
            m_winHandle->setMouseCursorIcon(m_glbase->getWinMan()->m_diagResizeAscCursor, WinMouseIcon::lbtrResize);
            m_winHandle->setMouseCursorIcon(m_glbase->getWinMan()->m_diagResizeDescCursor, WinMouseIcon::ltbrResize);
        }
#endif

        //-------------------------------------------------------------------------------------------

        // first argument: should the request be processed?
        m_procSteps[Callbacks] = ProcStep{true, [&] { iterateGlCallbacks(); }};
        m_procSteps[Draw]      = ProcStep{true, [&] { drawNodeTree(); }};

        //-------------------------------------------------------------------------------------------

        m_colors[uiColors::background]     = glm::vec4(0.2f, 0.2f, 0.2f, 1.f);
        m_colors[uiColors::darkBackground] = glm::vec4(0.12f, 0.12f, 0.12f, 1.f);
        m_colors[uiColors::sepLine]        = glm::vec4(0.3f, 0.3f, 0.3f, 1.f);
        m_colors[uiColors::font]           = glm::vec4(0.8f, 0.8f, 0.8f, 1.f);
        m_colors[uiColors::highlight]      = glm::vec4(0.f, 0.6f, 0.87f, 1.f);
        m_colors[uiColors::black]          = glm::vec4(0.f, 0.f, 0.f, 1.f);
        m_colors[uiColors::blue]           = glm::vec4(0.f, 0.6f, 0.9f, 1.f);
        m_colors[uiColors::darkBlue]       = glm::vec4(0.f, 0.36f, 0.5f, 1.f);
        m_colors[uiColors::white]          = glm::vec4(1.f, 1.f, 1.f, 1.f);

        m_minWinSize = ivec2(300, 300);
        m_sharedRes  = UISharedRes{(void *)this,
                                  &s_shCol,
                                  m_winHandle,
                                  m_objSel.get(),
                                  m_quad.get(),
                                  m_normQuad.get(),
                                  getOrthoMat(),
                                  m_dataFolder,
                                  &m_procSteps,
                                  this,
                                  false,
                                  &m_colors,
                                  glm::ivec2(60, 30),
                                  m_stdPadding,
                                  &m_minWinSize,
                                  m_glbase->getAssetManager(),
                                  &m_nullVao,
                                  m_drawMan.get(),
                                  m_glbase};

        //-------------------------------------------------------------------------------------------
        // UI ELEMENTS
        m_uiRoot = make_unique<UINode>();
        m_uiRoot->setSharedRes(&m_sharedRes);
        m_uiRoot->setName("root");  // set the name of this UINode

        if (!par.initToCurrentCtx) {
            m_contentRoot = m_uiRoot->addChild<UINode>();
            m_contentRoot->setName("ContentRoot");
            m_contentRoot->setSize(1.f, -(m_sharedRes.gridSize.y + static_cast<int>(m_stdPadding) * 2));
            m_contentRoot->setAlignY(valign::bottom);

#if defined(ARA_USE_GLFW)
            // create a MenuBar
            m_menuBar = m_uiRoot->addChild<MenuBar>();
            m_menuBar->setPadding(m_stdPadding * 3, m_stdPadding, m_stdPadding * 3, m_stdPadding);
            m_menuBar->setAlignY(valign::top);
            m_menuBar->setSize(1.f, m_sharedRes.gridSize.y + (int)m_stdPadding * 2);
            m_menuBar->setBackgroundColor(m_colors[uiColors::background]);
            m_menuBar->setWindowHandle(m_winHandle);

            // since the GLFWWindow is not part of UIWindow, these functions have to be bound this way
            m_menuBar->setCloseFunc([this] {
                // we are in the gl HID thread of this UIWindow
                if (m_closeFunc) {
                    m_closeFunc(this);
                } else if (m_appHandle && m_isMainWindow) {
                    // this is called from GL HID queue
                    ivec2 diagPos(0, 0);
                    ivec2 diagSize(500, 150);
                    if (m_winHandle) {
                        diagPos.x = (m_winHandle->getSize().x - diagSize.x) / 2 + m_winHandle->getPosition().x;
                        diagPos.y = (m_winHandle->getSize().y - diagSize.y) / 2 + m_winHandle->getPosition().y;
                    }

                    m_appHandle->openInfoDiag((InfoDiagParams{
                        .pos = diagPos,
                        .size = diagSize,
                        .tp = infoDiagType::confirm,
                        .msg = "Do you really want to quit?",
                        .minStayTime =  0,
                        .isModal = true,
                        .onConfirm = [this] {
                            m_winHandle->hide();
                            m_appHandle->exit();
                            return false;  // don't close the window, stop after immediately after this point
                        }
                    }));
                }
            });

            m_menuBar->setMinimizeFunc([this] {
                m_winProcCb.emplace_back([this] { m_winHandle->minimize(); });
                m_sharedRes.requestRedraw = true;
            });
            m_menuBar->setMaximizeFunc([this] {
                m_winProcCb.emplace_back([this] { m_winHandle->maximize(); });
                m_sharedRes.requestRedraw = true;
            });
            m_menuBar->setRestoreFunc([this] {
                m_winProcCb.emplace_back([this] { m_winHandle->restore(); });
                m_sharedRes.requestRedraw = true;
            });

            m_windowResizeAreas = m_uiRoot->addChild<WindowResizeAreas>();
            m_windowResizeAreas->setSize(1.f, 1.f);
#endif
        } else {
            m_menuBarEnabled             = false;
            m_windowResizeHandlesEnabled = false;
        }

        //-------------------------------------------------------------------------------------------

        initGL();
        UIWindow::onSetViewport(0, 0, static_cast<int>(vWidth), static_cast<int>(vHeight));

        m_stdTex = s_shCol.getStdTex();
        if (m_multisample) {
            m_stdTexMulti = s_shCol.getStdTexMulti();
        }
        m_lastLeftMouseDown = chrono::system_clock::now();
        s_inited            = true;
    }

#ifdef ARA_USE_GLFW
    if (!par.initToCurrentCtx && restartGlBaseLoop) {
        GLWindow::makeNoneCurrent();
        m_glbase->startRenderLoop();
        // no context bound at this point
    }
    // uiwindow context bound if not restartGlBaseLoop
#endif
}

void UIWindow::initGL() const {
    glLineWidth(1.f);

#ifndef ARA_USE_GLES31
    m_multisample ? glEnable(GL_MULTISAMPLE) : glDisable(GL_MULTISAMPLE);
#ifndef __EMSCRIPTEN__
    glEnable(GL_SAMPLE_SHADING);  // to use multisampling in shaders
    glEnable(GL_LINE_SMOOTH);     // bei der implementation von nvidia gibt es nur
                                  // LineWidth 0 -1 ...
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
#endif
#endif

    glSampleCoverage(GL_SAMPLE_ALPHA_TO_COVERAGE, false);  // multifragment processing
    glEnable(GL_BLEND);
    glEnable(GL_SCISSOR_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void UIWindow::open() {
#ifdef ARA_USE_GLFW
    if (m_isModal) {
        getApplicationHandle()->setActiveModalWin(this);
    }
    m_winHandle->open();
#endif
}

void UIWindow::close(bool direct) {
    if (!s_inited) {
        return;
    }
    if (!direct) {
        m_glbase->runOnMainThread([this] { return closeEvtLoopCb(); });
    } else {
        closeEvtLoopCb();
    }
}

bool UIWindow::closeEvtLoopCb() {
    // if this was called from key event callback, it will cause a crash, because we are still iterating through
    // GLFWWindow member variables. check if the window was already closed;
    if (!s_inited) {
        return true;
    }

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    if (m_selfManagedCtx) {
        if (m_isModal || m_glbase->getWinMan()->getFixFocus()) {
            if (m_winHandle && m_glbase->getWinMan()->getFixFocus() == m_winHandle) {
                m_glbase->getWinMan()->setFixFocus(nullptr);
            }

            if (m_appHandle) {
                m_appHandle->setActiveModalWin(nullptr);  // just sets flags
            }
        }

        if (m_winHandle) {
            // set a callback to be called when the window draw loop exits, but
            // the gl context is still valid remove all resources and cleanup
            m_winHandle->setOnCloseCb([this] {
                m_uiRoot.reset();
                if (m_objSel) {
                    m_objSel.reset();
                }
                s_shCol.clear();  // remove gl resources
                if (!m_texCol.empty()){
                    m_texCol.clear();
                }
                if (m_quad) {
                    m_quad.reset();
                }
                if (m_normQuad) {
                    m_normQuad.reset();
                }
                if (m_sceneFbo) {
                    m_sceneFbo.reset();
                }
            });

            m_winHandle->stopDrawThread();  // also calls close() on GLFWWindow
        }
    }
#endif

    // removes the window from the GLFWWindowsManager's m_windows array, i.e. destroys the Window and its context
    // m_winHandle is invalid afterward
#ifdef ARA_USE_GLFW
    if (m_selfManagedCtx) {
        if (m_winHandle->getOnCloseCb()) {
            m_winHandle->getOnCloseCb()();
        }
        m_glbase->getWinMan()->removeWin(m_winHandle, false);
    }
#endif

    m_winHandle = nullptr;
    s_inited    = false;

    return true;
}

/** if this function doesn't draw avoid the swap to be called afterwards. the return value indicates whether a swap is
 * needed or not */
bool UIWindow::draw(double time, double dt, int ctxNr) {
    // MenuBar win resize/minimize/move callbacks
    if (!m_winProcCb.empty()) {
        Conditional w;
        m_glbase->runOnMainThread([this, &w] {
            for (auto &it : m_winProcCb) {
                it();
            }
            m_winProcCb.clear();
            w.notify();
            return true;
        });
        w.wait();
    }

    // for window manipulating commands (close, maximize, restore, size, position or minimized) a different event queue
    // must be used in order to avoid double mutex locking with HID events
    WindowBase::procChangeWin();

    // process hid events, received in the WinBase HID callbacks this function passes down HID events to the UINodes and
    // by this assures, that all UINode HID functions are called within the GL-context of their parent window
    WindowBase::procHid();

    m_doSwap = m_procSteps[Draw].active;

    if (s_inited) {
        m_procSteps[Callbacks].active = true;  // always process callbacks
        if (m_selfManagedCtx) {
            s_drawMtx.lock();
        }
        s_isDrawing = true;

        // process all steps that have the first and second bool set to true
        for (auto &it : m_procSteps) {
            if (it.second.active) {
                if (it.second.func) {
                    it.second.func();
                }
                it.second.active = false;
            }
        }

        s_isDrawing = false;
        if (m_selfManagedCtx) {
            s_drawMtx.unlock();
        }
        m_resChanged = false;
    }

#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    // proc force redraw if requested
    if (m_selfManagedCtx && m_sharedRes.requestRedraw) {
        m_sharedRes.requestRedraw = false;
        m_procSteps[Draw].active  = true;
        m_winHandle->forceRedraw();
    }
#endif
    return m_doSwap;  // GLFWWindow will call glfwSwapBuffers when swap is true
}

void UIWindow::drawNodeTree() {
    if (!m_objSel) {
        return;
    }

#ifdef __ANDROID__
    glClear(GL_COLOR_BUFFER_BIT);
#endif

    m_sceneFbo->bind();
    m_sceneFbo->clear();  // also clears depth

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // clear the objId FBO
    m_objSel->reset();

    // Note: at the end of m_uiRoot->drawAsRoot the drawSets VAOs are updated,
    // which is a synchronization point between CPU and GPU on systems with a
    // slow bus this might kill a considerable amount of performance
    m_uiRoot->drawAsRoot(m_objSel->getIdCtr());

#ifndef FORCE_INMEDIATEMODE_RENDERING
    m_drawMan->draw();
#endif
    copyToScreen();  // --- blit the result. Binds draw framebuffer 0 which is the same as m_sceneFbo->unbind()
}

void UIWindow::copyToScreen() {
    if (m_multisample) {
        if (m_doSwap) {
#ifdef __APPLE__
            if (!m_glbase->rendererIsIntel()) {
#endif
                glBindFramebuffer(GL_READ_FRAMEBUFFER, m_sceneFbo->getFbo());
                glReadBuffer(GL_COLOR_ATTACHMENT0);
                glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
                glBlitFramebuffer(0, 0, m_sceneFbo->getWidth(), m_sceneFbo->getHeight(), 0, 0, m_sceneFbo->getWidth(),
                                  m_sceneFbo->getHeight(), GL_COLOR_BUFFER_BIT, GL_LINEAR);
#ifdef __APPLE__
            } else {
                m_sceneFbo->unbind();  // also clears depth

                // glBlitBuffer results in a darken image on osx 10.15 Intel HD
                // 4000
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // draw the FBO
                m_stdTexMulti->begin();
                m_stdTexMulti->setIdentMatrix4fv("m_pvm");
                m_stdTexMulti->setUniform1i("tex", 0);
                m_stdTexMulti->setUniform1i("nrSamples", m_sceneFbo->getNrSamples());
                m_stdTexMulti->setUniform2f("fboSize", m_sceneFbo->getWidth(), m_sceneFbo->getHeight());

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_sceneFbo->getColorImg(0));

                m_normQuad->draw();
            }
#endif
        }
    } else {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_sceneFbo->getFbo());
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, m_sceneFbo->getWidth(), m_sceneFbo->getHeight(), 0, 0, m_sceneFbo->getWidth(),
                          m_sceneFbo->getHeight(), GL_COLOR_BUFFER_BIT, GL_LINEAR);
        /*
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // draw the FBO
                m_stdTex->begin();
                m_stdTex->setIdentMatrix4fv("m_pvm");
                m_stdTex->setUniform1i("tex", 0);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, m_sceneFbo->getColorImg(0));

                m_normQuad->draw();
        */
    }
}

void UIWindow::update() {
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    if (m_selfManagedCtx && m_winHandle->isRunning() && m_winHandle->isInited()) {
        m_procSteps[Draw].active = true;
        m_winHandle->iterate();
    }
#else
    m_procSteps[Draw].active = true;
#endif
}

void UIWindow::iterate() {
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    if (m_selfManagedCtx && m_winHandle->isRunning() && m_winHandle->isInited()) m_winHandle->iterate();
#endif
}

void UIWindow::getActualMonitorMaxArea(int win_xpos, int win_ypos) {
#ifdef ARA_USE_GLFW
    int           count;
    GLFWmonitor **monitors = glfwGetMonitors(&count);
    for (auto i = 0; i < count; i++) {
        const GLFWvidmode *mode = glfwGetVideoMode(monitors[i]);
        int                vxpos, vypos;
        glfwGetMonitorPos(monitors[i], &vxpos, &vypos);

        // check if the window top left corner lies within this monitor
        if (win_xpos > vxpos && win_xpos < (vxpos + mode->width) && win_ypos > vypos &&
            win_ypos < (vypos + mode->height)) {
            int ma_xpos, ma_ypos, ma_width, ma_height;
            glfwGetMonitorWorkarea(monitors[i], &ma_xpos, &ma_ypos, &ma_width, &ma_height);
            setMonitorMaxArea(ma_xpos, ma_ypos, ma_width, ma_height);
        }
    }
#endif
}

/** HID callbacks called from the glfw event loop */
void UIWindow::key_callback(int key, int scancode, int action, int mods) {
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    int  outKey  = key;
    bool isAlt   = (key == GLSG_KEY_LEFT_ALT) || (key == GLSG_KEY_RIGHT_ALT);
    bool isCtrl  = (key == GLSG_KEY_LEFT_CONTROL) || (key == GLSG_KEY_RIGHT_CONTROL);
    bool isShift = (key == GLSG_KEY_LEFT_SHIFT) || (key == GLSG_KEY_RIGHT_SHIFT);

    // convert special keys, ...QT and GLFW use different key codes
    switch (key) {
        case GLSG_KEY_UP: outKey = 19; break;
        case GLSG_KEY_DOWN: outKey = 21; break;
        case GLSG_KEY_LEFT: outKey = 18; break;
        case GLSG_KEY_RIGHT: outKey = 20; break;
        default: break;
    }

    if (action == GLSG_PRESS || action == GLSG_REPEAT) {
        WindowBase::osKeyDown(outKey, (mods & GLSG_MOD_SHIFT) || isShift, (mods & GLSG_MOD_CONTROL) || isCtrl,
                              (mods & GLSG_MOD_ALT) || isAlt);
    }

    if (action == GLSG_RELEASE) {
        WindowBase::osKeyUp(outKey, (mods & GLSG_MOD_SHIFT) || isShift, (mods & GLSG_MOD_CONTROL) || isCtrl,
                            (mods & GLSG_MOD_ALT) || isAlt);
    }

    if ((key == GLSG_KEY_ESCAPE || (key == GLSG_KEY_F4 && (mods & GLSG_MOD_ALT))) && action == GLSG_PRESS) {
        // must be done indirect since we are still iterating over GLFWWindow
        // member variables, but GLFWWindow will be destroyed by this command
        if (m_appHandle && m_appHandle->getMainWindow() == this) {
#ifdef ARA_DEBUG
            // event loop should only contain this event ... which is very
            // unlikely not to be that way
            m_glbase->runOnMainThread(
                [this] {
                    m_appHandle->exit();
                    return true;
                },
                true);  // since we are on the main thread here, force pushing
                        // to the queue
#else
            // this is called from GL HID queue
            ivec2 diagPos{0};
            ivec2 diagSize{500, 150};
            diagPos.x = (getWidth() - diagSize.x) / 2 + getPosition().x;
            diagPos.y = (getHeight() - diagSize.y) / 2 + getPosition().y;

            m_appHandle->openInfoDiag(diagPos.x, diagPos.y, diagSize.x, diagSize.y, infoDiagType::confirm,
                                      "Do you really want to quit?", true, 0, [this] {
                                          m_appHandle->exit();
                                          return false;
                                      });
#endif
        }
    }

    iterate();  // -> procHID
#endif
}

void UIWindow::char_callback(unsigned int codepoint) {
    WindowBase::osChar(codepoint);
    iterate();
}

/** input in virtual pixels. The inconsistency between windows and osx (Windows
 * returns real pixels, macOS returns virtual (hdpi independent) pixels) is
 * fixed for GLFWWindows before this method is called **/
void UIWindow::cursor_callback(double xpos, double ypos) {
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    m_lastMouseX = xpos;
    m_lastMouseY = ypos;

    if (!m_winHandle->isRunning() || !m_winHandle->isInited()) return;

    WindowBase::osMouseMove((float)xpos, (float)ypos, 0);
    iterate();  // -> procHID
#endif
}

void UIWindow::mouseBut_callback(int button, int action, int mods) {
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)

#ifdef ARA_USE_GLFW
    if (m_lastMouseX == 0.f && m_lastMouseY == 0.f && m_winHandle) {
        double xpos, ypos;
        glfwGetCursorPos(m_winHandle->getCtx(), &xpos, &ypos);
        m_lastMouseX = xpos;
        m_lastMouseY = ypos;
    }
#endif

    if (button == GLSG_MOUSE_BUTTON_LEFT && action == GLSG_PRESS)
        WindowBase::osMouseDownLeft((float)m_lastMouseX, (float)m_lastMouseY, mods & GLSG_MOD_SHIFT,
                                    mods & GLSG_MOD_CONTROL, mods & GLSG_MOD_ALT);

    if (button == GLSG_MOUSE_BUTTON_LEFT && action == GLSG_RELEASE) WindowBase::osMouseUpLeft();

    if (button == GLSG_MOUSE_BUTTON_RIGHT && action == GLSG_PRESS)
        WindowBase::osMouseDownRight((float)m_lastMouseX, (float)m_lastMouseY, mods & GLSG_MOD_SHIFT,
                                     mods & GLSG_MOD_CONTROL, mods & GLSG_MOD_ALT);

    if (button == GLSG_MOUSE_BUTTON_RIGHT && action == GLSG_RELEASE) WindowBase::osMouseUpRight();

    iterate();  // the UIWindow's draw function needs to be called in order to
                // have WindowBase::procHid(); be called

#endif
}

void UIWindow::scroll_callback(double xoffset, double yoffset) {
    WindowBase::osWheel((float)yoffset);  // degree
    iterate();
}

/** xpos, ypos -> top left corner of the window in relation to workarea */
void UIWindow::window_pos_callback(int xpos, int ypos) {
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    WindowBase::osSetWinPos([this, xpos, ypos] {
        // this is also called on Windows when the window is minimized
        // since there are invalid numbers, we have to filter those calls

        // check if the window will be completely invisible
        int *wa = m_winHandle->getWorkArea();
        if (wa && wa[2] && wa[3] &&
            !(xpos > wa[2]                                   // outside right bounds
              || xpos > wa[3]                                // outside bottom bounds
              || (xpos + m_winHandle->getSize().x < wa[0])   // outside left bounds
              || (ypos + m_winHandle->getSize().y < wa[1]))  // outside top bounds
        ) {
            getActualMonitorMaxArea(xpos, ypos);
            for (auto &it : m_globalWinPosCb) it.second(xpos, ypos);

            iterate();  // -> procHID
        }
    });

#endif
}

void UIWindow::window_maximize_callback(int maximized) {
    WindowBase::osMaximize([this] {
        ivec2 actWinSize = getSize();
        setMonitorMaxArea(0, 0, actWinSize.x, actWinSize.y);
    });
    iterate();
}

void UIWindow::window_minimize_callback(int iconified) {
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    WindowBase::osMinimize([this, iconified] {
        // if the window is modal, be sure that the main window is also
        // minimized
        if (iconified && m_isModal && getApplicationHandle()) {
            auto mainWin = getApplicationHandle()->getMainWindow();
            if (mainWin && mainWin != this && mainWin->getWinHandle()->isOpen()) {
                mainWin->getWinHandle()->minimize();
            }
        }

        // if this window was restored and is a modal dialog, be sure that the
        // mainwindow is also restored
        if (!iconified && m_isModal && getApplicationHandle()) {
            auto mainWin = getApplicationHandle()->getMainWindow();
            if (mainWin && mainWin != this && mainWin->getWinHandle()->isMinimized()) {
                mainWin->getWinHandle()->restore();
            }
        }

        // if the mainwindow was restored, be sure that in case there is any
        // modal dialog present, it also get's restored
        if (!iconified && getApplicationHandle() && getApplicationHandle()->getMainWindow() == this)
            for (auto &w : *getApplicationHandle()->getUIWindows()) {
                if (w->isModal() && w->getWinHandle()->isMinimized()) {
                    w->getWinHandle()->restore();
                }

                if (w->isModal() && !w->getWinHandle()->isOpen()) {
                    w->getWinHandle()->open();
                }
            }

        // if the main window is minimized, be sure all modal dialogues are
        // hidden
        if (iconified && getApplicationHandle() && getApplicationHandle()->getMainWindow() == this) {
            for (auto &w : *getApplicationHandle()->getUIWindows()) {
                if (w->isModal()) w->getWinHandle()->hide();
            }
        }
    });
    iterate();
#endif
}

void UIWindow::window_focus_callback(int focused) {}

void UIWindow::window_size_callback(int width, int height) {
    WindowBase::osSetViewport(0, 0, width, height);
    iterate();
}

void UIWindow::window_close_callback() {
#ifdef ARA_USE_GLFW
    // when alt+F4 pressed on windows, prevent to close -> there should be a dialog before
    glfwSetWindowShouldClose((GLFWwindow *)m_winHandle->getWin(), GLFW_FALSE);
#endif
}

void UIWindow::window_refresh_callback() {
    // normally no update needed.
    // But needed when window is moving from one display to another
    // TODO: how can it be avoid to call this when not necessary?
    update();
}

/** HID callbacks, called from the gl draw loop */

void UIWindow::onKeyDown(int key, bool shiftPressed, bool ctrlPressed, bool altPressed) {
    if (key == GLSG_KEY_O && shiftPressed) {
        m_showObjMap = !m_showObjMap;
        m_sharedRes.setDrawFlag();
    } else if (key == GLSG_KEY_D && shiftPressed && !ctrlPressed) {
        m_uiRoot->dump();
    }

    m_hidData.shiftPressed = shiftPressed;
    m_hidData.ctrlPressed  = ctrlPressed;
    m_hidData.altPressed   = altPressed;
    m_hidData.procSteps    = &m_procSteps;
    m_hidData.key          = key;

    if (m_inputFocusNode) {
        m_inputFocusNode->keyDown(&m_hidData);
        if (m_inputFocusNode->getKeyDownCb()) {
            (*m_inputFocusNode->getKeyDownCb())(&m_hidData);
        }
    }

    for (auto &it : m_globalKeyDownCb) {
        it.second(&m_hidData);
    }
}

void UIWindow::onKeyUp(int key, bool shiftReleased, bool ctrlReleased, bool altReleased) {
    if (shiftReleased) m_hidData.shiftPressed = false;
    if (ctrlReleased) m_hidData.ctrlPressed = false;
    if (altReleased) m_hidData.altPressed = false;

    m_hidData.procSteps = &m_procSteps;
    m_hidData.key       = key;

    if (m_inputFocusNode) {
        m_inputFocusNode->keyUp(&m_hidData);
        if (m_inputFocusNode->getKeyUpCb()) {
            (*m_inputFocusNode->getKeyUpCb())(&m_hidData);
        }
    }

    for (auto &it : m_globalKeyUpCb) {
        it.second(&m_hidData);
    }
}

void UIWindow::onChar(unsigned int codepoint) {
    hidData data;
    data.procSteps = &m_procSteps;
    data.codepoint = codepoint;

    if (m_inputFocusNode) {
        m_inputFocusNode->onChar(&data);
    }
}

void UIWindow::fillHidData(hidEvent evt, float xPos, float yPos, bool shiftPressed, bool ctrlPressed, bool altPressed) {
    if (evt == hidEvent::MouseDownLeft || evt == hidEvent::MouseDownRight) {
        m_hidData.dragStart = true;
        m_mouseClickPos     = m_hidData.mousePos;

        m_hidData.shiftPressed = shiftPressed;
        m_hidData.ctrlPressed  = ctrlPressed;
        m_hidData.altPressed   = altPressed;
    }

    if (evt == hidEvent::MouseDownLeft) {
        m_hidData.mousePressed = true;
    }

    if (evt == hidEvent::MouseDownRight) {
        m_hidData.mouseRightPressed = true;
    }

    // window relative mouse position in virtual pixels (origin top,left)
    m_hidData.mousePos.x = xPos;
    m_hidData.mousePos.y = yPos;

    // window relative mouse position in hardware pixels (origin bottom,left) for direct usage in opengl
    m_hidData.mousePosFlipY.x = xPos / s_windowViewport.z;
    m_hidData.mousePosFlipY.y = 1.f - (yPos / s_windowViewport.w);

    m_hidData.mousePosNorm.x = xPos / s_windowViewport.z;
    m_hidData.mousePosNorm.y = yPos / s_windowViewport.w;

    m_hidData.mousePosNormFlipY.x = xPos / s_windowViewport.z;
    m_hidData.mousePosNormFlipY.y = 1.f - (yPos / s_windowViewport.w);

    if (evt == hidEvent::MouseDownLeft || evt == hidEvent::MouseDownRight) {
        m_hidData.mouseClickPosFlipY = m_hidData.mousePosNormFlipY;
    }

    m_hidData.objId        = getObjAtPos(m_hidData.mousePos.x, m_hidData.mousePos.y, evt);
    m_hidData.hitNode[evt] = m_opi.foundNode;

    // store mouseDown ObjId for dragging (objId must not change while dragging)
    if (evt == hidEvent::MouseDownLeft || evt == hidEvent::MouseDownRight) {
        m_hidData.clickedObjId = m_hidData.objId;
    }

    m_hidData.mousePosNormFlipY /= s_devicePixelRatio;

    // setup mouse click data
    m_hidData.cp_editM  = &m_cp_editM;
    m_hidData.procSteps = &m_procSteps;

    m_hidData.getScreenMousePos(s_devicePixelRatio);
}

void UIWindow::onMouseDownLeft(float xPos, float yPos, bool shiftPressed, bool ctrlPressed, bool altPressed) {
    fillHidData(hidEvent::MouseDownLeft, xPos, yPos, shiftPressed, ctrlPressed, altPressed);

    // check if double click
    auto   now  = chrono::system_clock::now();
    double diff = chrono::duration<double, std::milli>(now - m_lastLeftMouseDown).count();

    m_hidData.isDoubleClick = glm::length(m_lastClickedPos - vec2{xPos, yPos}) < 7.f && (diff < 500.f);
    m_lastLeftMouseDown     = now;
    m_hidData.movedPix.x    = 0.f;
    m_hidData.movedPix.y    = 0.f;

    // callbacks that are not related to a specific UINode
    for (auto &it : m_globalMouseDownLeftCb) {
        it.second(&m_hidData);
    }

    m_draggingNodeTree.clear();
    std::copy(m_opi.localTree.begin(), m_opi.localTree.end(), std::back_inserter(m_draggingNodeTree));

    // iterate mouse click through UINode - Tree
    m_hidData.reset();
    if (m_opi.foundNode && !m_opi.localTree.empty()) {
        UINode::hidIt(&m_hidData, hidEvent::MouseDownLeft, m_opi.localTree.begin(), m_opi.localTree);
    }

    m_lastClickedPos.x = xPos;
    m_lastClickedPos.y = yPos;

    m_procSteps[Draw].active = true;
}

void UIWindow::procInputFocus() {
    if (m_hidData.hitNode[hidEvent::MouseDownLeft]) {
        auto lastFocusNode = m_inputFocusNode;

        // old focused node looses focus
        if (m_inputFocusNode && m_inputFocusNode != m_hidData.hitNode[hidEvent::MouseDownLeft]) {
            if (m_inputFocusNode->getFocusAllowed()) {
                m_inputFocusNode->setInputFocus(false);
                m_inputFocusNode->onLostFocus();
            }
            m_inputFocusNode = nullptr;
        }

        // newly found Node gets focus
        m_inputFocusNode = m_hidData.hitNode[hidEvent::MouseDownLeft];
        if (m_inputFocusNode && m_inputFocusNode->getFocusAllowed()) {
            m_inputFocusNode->setInputFocus(true);
            m_inputFocusNode->onGotFocus();
        } else if (m_inputFocusNode && m_inputFocusNode->getReturnFocusCb()) {
            m_inputFocusNode->getReturnFocusCb()();
        } else {
            m_inputFocusNode = lastFocusNode;
        }
    }
}

void UIWindow::onMouseUpLeft() {
    m_hidData.releasedObjId = m_hidData.objId;
    m_hidData.consumed      = false;

    // callbacks that are not related to a specific UINode
    for (auto &it : m_globalMouseUpLeftCb) {
        it.second(&m_hidData);
    }

    // in case an object id changed during mouseDrag explicitly call the mouseup on this element
    if (m_hidData.dragging && m_draggingNode && !m_draggingNode->isHIDBlocked()) {
        m_draggingNode->mouseUp(&m_hidData);
        for (auto &it : m_draggingNode->getMouseUpCb()) {
            it.first(&m_hidData);
        }
    } else {
        m_hidData.reset();
    }

    if (m_opi.foundNode && !m_opi.localTree.empty()) {
        UINode::hidIt(&m_hidData, hidEvent::MouseUpLeft, m_opi.localTree.begin(), m_opi.localTree);
    }

    m_hidData.hitNode[hidEvent::MouseDownLeft] = nullptr;
    m_hidData.mousePressed                     = false;
    m_hidData.dragStart                        = true;
    m_hidData.dragging                         = false;
    m_hidData.objId                            = 0;
    m_draggingNode                             = nullptr;
}

void UIWindow::onMouseDownRight(float xPos, float yPos, bool shiftPressed, bool ctrlPressed, bool altPressed) {
    // ignore mouseDownRight, when in dragging mode
    if (m_hidData.dragging) {
        return;
    }

    fillHidData(hidEvent::MouseDownRight, xPos, yPos, shiftPressed, ctrlPressed, altPressed);

    // callbacks that are not related to a specific UINode
    for (auto &it : m_globalMouseDownRightCb) {
        it.second(&m_hidData);
    }

    m_draggingNodeTree.clear();
    std::copy(m_opi.localTree.begin(), m_opi.localTree.end(), std::back_inserter(m_draggingNodeTree));

    // iterate mouse click through UINode - Tree
    m_hidData.reset();
    if (m_opi.foundNode && !m_opi.localTree.empty()) {
        UINode::hidIt(&m_hidData, hidEvent::MouseDownRight, m_opi.localTree.begin(), m_opi.localTree);
    }
}

void UIWindow::onMouseUpRight() {
    m_hidData.releasedObjId = m_hidData.objId;

    // in case an object id changed during mouseDrag explicitly call the mouseup on this element
    if (m_hidData.dragging && m_draggingNode && !m_draggingNode->isHIDBlocked()) {
        m_draggingNode->mouseUpRight(&m_hidData);
        for (auto &it : m_draggingNode->getMouseUpRightCb()) {
            it.first(&m_hidData);
        }
    }

    m_hidData.reset();
    if (m_opi.foundNode && !m_opi.localTree.empty()) {
        UINode::hidIt(&m_hidData, hidEvent::MouseUpRight, m_opi.localTree.begin(), m_opi.localTree);
    }

    m_hidData.mousePressed                      = false;
    m_hidData.mouseRightPressed                 = false;
    m_hidData.dragStart                         = false;
    m_hidData.dragging                          = false;
    m_hidData.objId                             = 0;
    m_hidData.hitNode[hidEvent::MouseDownRight] = nullptr;
    m_draggingNode                              = nullptr;
}

void UIWindow::onMouseMove(float xpos, float ypos, ushort _mode) {
    fillHidData(hidEvent::MouseMove, xpos, ypos, m_hidData.shiftPressed, m_hidData.ctrlPressed, m_hidData.altPressed);

    if (!m_uiRoot) {
        return;
    }

    auto foundNode = m_opi.foundNode;
    m_hidData.reset();
    if (m_opi.foundNode && !m_opi.localTree.empty()){
        UINode::hidIt(&m_hidData, hidEvent::MouseMove, m_opi.localTree.begin(), m_opi.localTree);
    }

    m_hidData.newNode = (void *)foundNode;

    for (auto &it : m_globalMouseMoveCb) {
        it.second(&m_hidData);
    }

    // if the mouse moved to another UiNode, advice the old node of this event
    if (m_lastHoverFound && m_lastHoverFound != foundNode && !m_hidData.dragging && !m_lastHoverFound->isHIDBlocked()) {
        m_lastHoverFound->mouseOut(&m_hidData);  // mouse out has to be processed before mouse in !!!
    }

    if (foundNode && (!m_lastHoverFound || m_lastHoverFound != foundNode) && !m_hidData.dragging &&
        !foundNode->isHIDBlocked()) {
        foundNode->mouseIn(&m_hidData);
    }

    m_lastHoverFound = foundNode;

    m_hidData.movedPix = m_hidData.mousePos - m_mouseClickPos;
    bool isValidDrag = glm::length(m_hidData.movedPix) > 4.f;

    if ((m_hidData.mousePressed || m_hidData.mouseRightPressed) && !m_draggingNodeTree.empty() &&
        (!m_hidData.dragStart || isValidDrag)) {
        // since we are using a minimum distance for validating drag gestures, we have to correct the movedPix in order
        // to avoid jumps
        if (m_hidData.dragStart && isValidDrag) {
            m_mouseClickPos      = m_hidData.mousePos;
            m_hidData.movedPix.x = 0.f;
            m_hidData.movedPix.y = 0.f;
        }

        // always use clickedObjId when dragging
        if (m_hidData.dragStart) {
            m_hidData.objId = m_hidData.clickedObjId;
        }

        UINode::hidIt(&m_hidData, hidEvent::MouseDrag, m_draggingNodeTree.begin(), m_draggingNodeTree);
        m_hidData.dragStart = false;
        m_hidData.dragging  = true;
    }
}

void UIWindow::onWheel(float deg) {
    m_hidData.objId   = (int)getObjAtPos(m_hidData.mousePos.x, m_hidData.mousePos.y, hidEvent::MouseWheel);
    m_hidData.degrees = deg;

    m_hidData.reset();
    if (m_opi.foundNode && !m_opi.localTree.empty()) {
        UINode::hidIt(&m_hidData, hidEvent::MouseWheel, m_opi.localTree.begin(), m_opi.localTree);
    }

    m_procSteps[Draw].active = true;
}

// input always in virtual pixels
void UIWindow::onSetViewport(int x, int y, int width, int height) {
    bool clearFonts = false;

#ifdef ARA_USE_GLFW
    if (m_winHandle) {
        s_devicePixelRatio = m_winHandle->getContentScale().x;
    }
#elif _WIN32
    if (m_forceDpiScale > -1.f) {
        // fonts must be updated in case dpi has changed
        // clearFonts = s_devicePixelRatio != m_forceDpiScale;
        s_devicePixelRatio = m_forceDpiScale;
        m_forceDpiScale    = -1.f;
    } else {
        float yscale;
        auto  hwnd = (HWND)getWinHandle();
        getDisplayScale(hwnd, &s_devicePixelRatio, &yscale);
    }
#endif
    // in virtual pixels
    s_windowViewport.x = static_cast<float>(x);
    s_windowViewport.y = static_cast<float>(y);
    s_windowViewport.z = static_cast<float>(width);
    s_windowViewport.w = static_cast<float>(height);

    // in hardware pixels
    s_viewPort = glm::round(s_windowViewport * s_devicePixelRatio);

    // update orthoMat
    s_orthoMat = glm::ortho(0.f, s_windowViewport.z, s_windowViewport.w, 0.f);
    m_sceneFbo->resize((uint32_t)s_viewPort.z, (uint32_t)s_viewPort.w);

    if (m_uiRoot) {
        m_uiRoot->setViewport(0.f, 0.f, s_windowViewport.z, s_windowViewport.w);
    }

    glViewport(0, 0, (GLsizei)s_viewPort.z, (GLsizei)s_viewPort.w);

    if (clearFonts && m_sharedRes.res && m_sharedRes.drawMan) {
        m_sharedRes.res->clearGLFont();
        m_sharedRes.drawMan->clearFonts();
    }

    update();

    // callbacks that are not related to a specific UINode
    for (auto &it : m_globalSetViewportCb) it.second(x, y, (int)((float)width), (int)((float)height));
}

void UIWindow::onNodeRemove(UINode *node) {
    if (!node) {
        return;
    }

    removeGlobalMouseDownLeftCb(node);

    if (m_inputFocusNode && m_inputFocusNode == node) {
        m_inputFocusNode = nullptr;
    }

    if (m_hidData.hitNode[hidEvent::MouseDownLeft] == node) {
        m_hidData.hitNode[hidEvent::MouseDownLeft] = nullptr;
    }

    if (m_hidData.hitNode[hidEvent::MouseDownRight] == node) {
        m_hidData.hitNode[hidEvent::MouseDownRight] = nullptr;
    }

    if (m_lastHoverFound == node) {
        m_lastHoverFound = nullptr;
    }

    if (m_opi.foundNode && m_opi.foundNode == node) {
        m_opi.foundNode = nullptr;
    }
}

int32_t UIWindow::getObjAtPos(float x, float y, hidEvent evt) {
    glm::vec2 pos{x, y};

    if (!m_uiRoot) {
        m_opi.foundNode = nullptr;
        return 0;
    }

    m_opi.it             = m_uiRoot->getChildren().end() - 1; // take the last element of the tree
    m_opi.list           = &m_uiRoot->getChildren();
    m_opi.pos            = pos;
    m_opi.foundNode      = nullptr;
    m_opi.treeLevel      = 0;
    m_opi.foundTreeLevel = -1;
    m_opi.event          = evt;

    UINode::objPosIt(m_opi);

    if (!m_opi.foundNode) {
        return 0;
    }

    // build a local tree containing the found node and all its parent nodes
    m_opi.localTree.clear();
    auto p = m_opi.foundNode;
    m_opi.localTree.emplace_back(p);

    while (p->getParent()) {
        m_opi.localTree.emplace_back(p->getParent());
        p = p->getParent();
    }

    return static_cast<int32_t>(m_opi.foundId);
}

void UIWindow::setModal(bool val) {
#if defined(ARA_USE_GLFW) || defined(ARA_USE_EGL)
    if (m_selfManagedCtx) {
        m_isModal = val;
        if (val) {
            if (m_winHandle) {
                m_glbase->getWinMan()->setFixFocus(m_winHandle);
            }
            if (m_appHandle) {
                m_appHandle->setActiveModalWin(this);
            }
        } else {
            if (m_winHandle && m_glbase->getWinMan()->getFixFocus() == m_winHandle) {
                m_glbase->getWinMan()->setFixFocus(nullptr);
            }
        }
    }
#endif
}

void UIWindow::setBlockHid(bool val) {
    m_blockHID = val;

    // block all but the menu bar
    if (m_uiRoot) {
        m_uiRoot->setHIDBlocked(val);
    }

    if (m_selfManagedCtx && m_menuBarEnabled && val && m_menuBar) {
        m_menuBar->setHIDBlocked(false);
    }
}

void UIWindow::setInputFocusNode(UINode *node, bool procLostFocus) {
    if (m_inputFocusNode && procLostFocus) {
        m_inputFocusNode->onLostFocus();
        m_inputFocusNode->setInputFocus(false);
    }

    m_inputFocusNode = node;

    if (m_inputFocusNode) m_inputFocusNode->onGotFocus();
}

void UIWindow::setAppIcon(std::string &path) {
#if defined(ARA_USE_GLFW) && defined(ARA_USE_FREEIMAGE)
    // set app icon
    std::vector<uint8_t> vp;
    m_glbase->getAssetManager()->loadResource(nullptr, vp, "precision/vioso6_48x48.png");

    if (vp.empty()) return;

    FIBITMAP          *pBitmap;
    FreeImg_MemHandler mh(&vp[0], vp.size());
    FREE_IMAGE_FORMAT  fif = FreeImage_GetFileTypeFromHandle(mh.io(), (fi_handle)&mh, 0);
    if (!(pBitmap = FreeImage_LoadFromHandle(fif, mh.io(), (fi_handle)&mh, 0))) return;

    FreeImage_FlipVertical(pBitmap);

    GLFWimage logo;
    logo.width  = 48;
    logo.height = 48;
    // logo.pixels = (uint8_t*)pBitmap->data;
    logo.pixels = (uint8_t *)pBitmap->data + logo.width * 8;

    int nrChan = 4;

    // convert to bgra
    std::array<uint8_t, 4> t{};
    int                    x, y;
    for (y = 0; y < (logo.height); y++) {
        for (x = 0; x < logo.width; x++) {
            int idx = ((y * logo.width) + x) * nrChan;

            t[0] = logo.pixels[idx + 2];
            t[1] = logo.pixels[idx + 1];
            t[2] = logo.pixels[idx + 0];
            t[3] = logo.pixels[idx + 3];

            std::copy(&t[0], &t[0] + nrChan, &logo.pixels[idx]);
        }
    }

    glfwSetWindowIcon((GLFWwindow *)getWinHandle()->getWin(), 1, &logo);
#endif
}

void UIWindow::setMouseCursorVisible(bool val) {
#ifdef ARA_USE_GLFW
    if (m_winHandle && m_winHandle->getWin())
        m_glbase->runOnMainThread([this, val] {
            glfwSetInputMode(m_winHandle->getCtx(), GLFW_CURSOR, val ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
            return true;
        });
#elif _WIN32
    if (val)
        while (::ShowCursor(val) < 0) {
        }
    else
        while (::ShowCursor(val) > -1) {
        }
#endif
}

void UIWindow::addGlobalWinPosCb(void* ptr, std::function<void(int, int)> f) {
    m_globalWinPosCb[ptr] = std::move(f);
}

void UIWindow::removeGlobalWinPosCb(void* ptr) {
    if (m_globalWinPosCb.empty()) return;
    auto it = m_globalWinPosCb.find(ptr);
    if (it != m_globalWinPosCb.end()) {
        m_globalWinPosCb.erase(it);
    }
}

void UIWindow::addGlobalSetViewportCb(void* ptr, std::function<void(int, int, int, int)> f) {
    m_globalSetViewportCb[ptr] = std::move(f);
}

void UIWindow::removeGlobalSetViewportCb(void* ptr) {
    if (m_globalSetViewportCb.empty()) return;
    auto it = m_globalSetViewportCb.find(ptr);
    if (it != m_globalSetViewportCb.end()) {
        m_globalSetViewportCb.erase(it);
    }
}

void UIWindow::addGlobalMouseDownLeftCb(void* ptr, std::function<void(hidData*)> f) {
    m_globalMouseDownLeftCb[ptr] = std::move(f);
}

void UIWindow::removeGlobalMouseDownLeftCb(void* ptr) {
    if (m_globalMouseDownLeftCb.empty()) return;
    auto it = m_globalMouseDownLeftCb.find(ptr);
    if (it != m_globalMouseDownLeftCb.end()) {
        m_globalMouseDownLeftCb.erase(it);
    }
}

void UIWindow::addGlobalMouseUpLeftCb(void* ptr, std::function<void(hidData*)> f) {
    m_globalMouseUpLeftCb[ptr] = std::move(f);
}

void UIWindow::removeGlobalMouseUpLeftCb(void* ptr) {
    if (m_globalMouseUpLeftCb.empty()) return;
    auto it = m_globalMouseUpLeftCb.find(ptr);
    if (it != m_globalMouseUpLeftCb.end()) {
        m_globalMouseUpLeftCb.erase(it);
    }
}

void UIWindow::addGlobalMouseDownRightCb(void* ptr, std::function<void(hidData*)> f) {
    m_globalMouseDownRightCb[ptr] = std::move(f);
}

void UIWindow::removeGlobalMouseDownRightCb(void* ptr) {
    if (m_globalMouseDownLeftCb.empty()) return;
    auto it = m_globalMouseDownRightCb.find(ptr);
    if (it != m_globalMouseDownRightCb.end()) {
        m_globalMouseDownRightCb.erase(it);
    }
}

void UIWindow::addGlobalMouseMoveCb(void* ptr, std::function<void(hidData*)> f) {
    m_globalMouseMoveCb[ptr] = std::move(f);
}

void UIWindow::removeGlobalMouseMoveCb(void* ptr) {
    if (m_globalMouseMoveCb.empty()) return;
    auto it = m_globalMouseMoveCb.find(ptr);
    if (it != m_globalMouseMoveCb.end()) {
        m_globalMouseMoveCb.erase(it);
    }
}

void UIWindow::addGlobalKeyDownCb(void* ptr, std::function<void(hidData*)> f) {
    m_globalKeyDownCb[ptr] = std::move(f);
}

void UIWindow::removeGlobalKeyDownCb(void* ptr) {
    if (m_globalKeyDownCb.empty()) return;
    auto it = m_globalKeyDownCb.find(ptr);
    if (it != m_globalKeyDownCb.end()) {
        m_globalKeyDownCb.erase(it);
    }
}

void UIWindow::addGlobalKeyUpCb(void* ptr, std::function<void(hidData*)> f) {
    m_globalKeyUpCb[ptr] = std::move(f);
}

void UIWindow::removeGlobalKeyUpCb(void* ptr) {
    if (m_globalKeyUpCb.empty()) return;
    auto it = m_globalKeyUpCb.find(ptr);
    if (it != m_globalKeyUpCb.end()) {
        m_globalKeyUpCb.erase(it);
    }
}

void UIWindow::setEnableMenuBar(bool val) {
    m_menuBarEnabled = val;
    if (m_menuBar) {
        m_menuBar->setVisibility(val);
        m_contentRoot->setVisibility(val);
    }
}

void UIWindow::setEnableWindowResizeHandles(bool val) {
    m_windowResizeHandlesEnabled = val;
    if (m_windowResizeAreas) {
        m_windowResizeAreas->setVisibility(val);
    }
}

void UIWindow::setMonitorMaxArea(int x, int y, int w, int h) {
    monitorMaxArea.x = x;
    monitorMaxArea.y = y;
    monitorMaxArea.z = w;
    monitorMaxArea.w = h;
}

void UIWindow::startRenderLoop() {
    if (m_winHandle->getDrawFunc()) {
        m_winHandle->startDrawThread(m_winHandle->getDrawFunc());
    }
    m_winHandle->getGlInitedSema()->wait(0);
}

void UIWindow::stopRenderLoop() {
    if (m_winHandle && m_winHandle->isRunning()) {
        m_winHandle->stopDrawThread();
    }
}

void UIWindow::setResChanged(bool val) {
    m_resChanged = val;
    if (m_uiRoot) {
        m_uiRoot->reqTreeChanged(true);
    }
}

void UIWindow::addGlCb(void* cbName, const std::string& fName, const std::function<bool()>& func) {
    WindowBase::addGlCb(cbName, fName, func);
    update();
    m_sharedRes.requestRedraw = true;
}

// wrapping up the util filedialog, since in combination with MFC this causes
// issues which have to be addressed
#ifdef _WIN32
std::string UIWindow::OpenFileDialog(std::vector<COMDLG_FILTERSPEC> &allowedSuffix) {
#ifdef _WIN32
#ifdef ARA_USE_GLFW
    HWND owner = getWinHandle()->getHwndHandle();
#else
    HWND owner = (HWND)getWinHandle();
#endif
#endif

#if defined(_WIN32) && !defined(ARA_USE_GLFW)
    // HACK: when used with MFC, SaveFileDialog prohibits the mouseup to reach
    // UIWin, so do it explicitly
    onMouseUpLeft();
    setInputFocusNode(nullptr, true);  // avoid crashed when in use with MFC, ... there may be
                                       // hid commandos, which reach after the dialog closes
    setBlockDraw(true);
#endif
    std::string out;
   // std::string out = OpenFileDialog(allowedSuffix, owner);
#if defined(_WIN32) && !defined(ARA_USE_GLFW)
    setBlockDraw(false);
#endif
    return out;
}

std::string UIWindow::SaveFileDialog(const std::vector<std::pair<std::string, std::string>>& fileTypes) {
#ifdef _WIN32
#ifdef ARA_USE_GLFW
    HWND owner = getWinHandle()->getHwndHandle();
#else
    HWND owner = (HWND)getWinHandle();
#endif
#endif

#if defined(_WIN32) && !defined(ARA_USE_GLFW)
    // HACK: when used with MFC, SaveFileDialog prohibits the mouseup to reach
    // UIWin, so do it explicitly
    onMouseUpLeft();
    setInputFocusNode(nullptr, true);
    setBlockDraw(true);
#endif

    std::string out;
//   std::string out = SaveFileDialog(std::move(fileTypes), owner);

#if defined(_WIN32) && !defined(ARA_USE_GLFW)
    setBlockDraw(false);
#endif
    return out;
}
#endif

}  // namespace ara