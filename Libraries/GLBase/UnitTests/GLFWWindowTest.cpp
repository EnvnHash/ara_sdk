#include "GLBaseUnitTestCommon.h"
#include "OSHidSimulation.h"
#include <WindowManagement/GLFWWindow.h>
#include <future>

using namespace std;
using namespace glm;

namespace ara::GLBaseUnitTest::GLFWWindowHidTest {

    GLFWWindow* w=nullptr;
    bool didCall = false;
    int runIt = 0;
    int maxRunIt = 5;

    bool staticDrawFunc(double, double, int) {
        didCall = true;
        w->close();
        return false;
    }

    void initWin(GLFWWindow& win) {
        w = &win;
        didCall = false;
        runIt = 0;
        ASSERT_TRUE(win.init(glWinPar{ .shift = { 30, 30 }, .size = { 400, 400 } }));
        ASSERT_TRUE(initGLEW());
    }

    void hidTestBody(const function<void(GLFWWindow&)>& setCbFunc, const function<void()>& emitEventFunc) {
        GLFWWindow win;
        initWin(win);
        setCbFunc(win);

        while (!didCall && runIt < 5) {
            glfwSwapBuffers(win.getCtx());
            glfwPollEvents();
            emitEventFunc();
            ++runIt;
        }

        EXPECT_TRUE(didCall);
        win.destroy(false);
    }

    void setCbCalled() {
        didCall = true; runIt = maxRunIt;
    }
/*
    TEST(GLBaseTest, CreateWindowTest) {
        GLFWWindow win;
        initWin(win);
        win.runLoop(staticDrawFunc, false, false);
        EXPECT_TRUE(didCall);
    }
*/
    // GLFW callbacks
#ifdef __linux__

    TEST(GLBaseTest, LibGLFWKeyCbTest) {
        hidTestBody([&](GLFWWindow& win){ glfwSetKeyCallback(win.getCtx(), [](auto ...args) { setCbCalled(); }); },
                    []{ SimulateKeyPress('B'); });
    }

    TEST(GLBaseTest, LibGLFWCharCbTest) {
        hidTestBody([&](GLFWWindow &win) { glfwSetCharCallback(win.getCtx(), [](auto ...args) { setCbCalled(); }); },
                    [] { SimulateKeyPress('A'); });
    }

    TEST(GLBaseTest, LibGLFWMouseButCbTest) {
        hidTestBody([](GLFWWindow &win) { glfwSetMouseButtonCallback(win.getCtx(), [](auto ...args) { setCbCalled(); });},
                    [] { SimulateMouseButtonClick(200, 200, 1); });
    }

    TEST(GLBaseTest, LibGLFWMouseCursorCbTest) {
        hidTestBody([](GLFWWindow &win) { glfwSetCursorPosCallback(win.getCtx(), [](auto ...args) { setCbCalled(); });},
                    [] { SimulateMouseMovement(200, 200, 250, 250); });
    }

    TEST(GLBaseTest, LibGLFWScrollCbTest) {
        hidTestBody([](GLFWWindow &win) { glfwSetScrollCallback(win.getCtx(), [](auto ...args) { setCbCalled(); });},
                    [] { SimulateWheel(w->getCtx(), 20); });
    }

    TEST(GLBaseTest, LibGLFWWindowCloseCbTest) {
        hidTestBody([](GLFWWindow &win) { glfwSetWindowCloseCallback(win.getCtx(), [](auto ...args) { setCbCalled(); });},
                    [] { CloseWindow(w->getCtx()); });
    }

    TEST(GLBaseTest, LibGLFWWindowMaximizeCbTest) {
        hidTestBody([](GLFWWindow &win) { glfwSetWindowMaximizeCallback(win.getCtx(), [](auto ...args) { setCbCalled(); });},
                    [] { glfwMaximizeWindow(w->getCtx()); std::this_thread::sleep_for(std::chrono::milliseconds(200)); });
    }

    TEST(GLBaseTest, LibGLFWWindowFocusCbTest) {
        hidTestBody([](GLFWWindow &win) { glfwSetWindowFocusCallback(win.getCtx(), [](auto ...args) { setCbCalled(); });},
                    [] { });
    }

    TEST(GLBaseTest, LibGLFWWindowSizeCbTest) {
        hidTestBody([](GLFWWindow &win) { glfwSetWindowSizeCallback(win.getCtx(), [](auto ...args) { setCbCalled(); });},
                    [] { w->resize(100, 100); });
    }

    TEST(GLBaseTest, LibGLFWWindowPosCbTest) {
        hidTestBody([](GLFWWindow &win) { glfwSetWindowPosCallback(win.getCtx(), [](auto ...args) { setCbCalled(); });},
                    [] { w->setPosition(100, 100); });
    }

    TEST(GLBaseTest, LibGLFWWindowIconifyCbTest) {
        hidTestBody([](GLFWWindow &win) { glfwSetWindowIconifyCallback(win.getCtx(), [](auto ...args) { setCbCalled(); });},
                    [] { glfwIconifyWindow(w->getCtx()); });
    }

    // Window Callbacks

    TEST(GLBaseTest, LibGLFWWindowKeyWinCbTest) {
        hidTestBody([](GLFWWindow &win) { win.setWinCallback(winCb::Key, [](int, int, int, int) { setCbCalled(); }); },
                    [] { SimulateKeyPress('B'); });
    }

    TEST(GLBaseTest, LibGLFWWindowCharWinCbTest) {
        hidTestBody([](GLFWWindow &win) { win.setWinCallback(winCb::Char, static_cast<std::function<void(unsigned int)>>([](unsigned int) { setCbCalled(); })); },
                    [] { SimulateKeyPress('B'); });
    }

    TEST(GLBaseTest, LibGLFWMouseButWinCbTest) {
        hidTestBody([](GLFWWindow &win) { win.setWinCallback(winCb::MouseButton, [](int, int, int) { setCbCalled(); }); },
                    [] { SimulateMouseButtonClick(200, 200, 1); });
    }

    TEST(GLBaseTest, LibGLFWMouseCursorWinCbTest) {
        hidTestBody([](GLFWWindow &win) { win.setWinCallback(winCb::CursorPos, static_cast<std::function<void(double, double)>>([](double, double) { setCbCalled(); })); },
                    [] { SimulateMouseMovement(200, 200, 250, 250); });
    }

    TEST(GLBaseTest, LibGLFWScrollWinCbTest) {
        hidTestBody([](GLFWWindow &win) { win.setWinCallback(winCb::Scroll, static_cast<std::function<void(double, double)>>([](double, double) { setCbCalled(); })); },
                    [] { SimulateWheel(w->getCtx(), 20); });
    }

    TEST(GLBaseTest, LibGLFWWindowCloseWinCbTest) {
        hidTestBody([](GLFWWindow &win) { win.setWinCallback(winCb::WindowClose, [] { setCbCalled(); }); },
                    [] { CloseWindow(w->getCtx()); });
    }

    TEST(GLBaseTest, LibGLFWWindowMaximizeWinCbTest) {
        hidTestBody([](GLFWWindow &win) { win.setWinCallback(winCb::WindowMaximize, static_cast<std::function<void(int)>>([](int) { setCbCalled(); })); },
                    [] { glfwMaximizeWindow(w->getCtx()); std::this_thread::sleep_for(std::chrono::milliseconds(200)); });
    }

    TEST(GLBaseTest, LibGLFWWindowFocusWinCbTest) {
        hidTestBody([](GLFWWindow &win) { win.setWinCallback(winCb::WindowFocus, static_cast<std::function<void(int)>>([](int) { setCbCalled(); })); },
                    [] { });
    }

    TEST(GLBaseTest, LibGLFWWindowSizeWinCbTest) {
        hidTestBody([](GLFWWindow &win) { win.setWinCallback(winCb::WindowSize, static_cast<std::function<void(int, int)>>([](int, int) { setCbCalled(); })); },
                    [] { w->resize(100, 100); });
    }

    TEST(GLBaseTest, LibGLFWWindowPosWinCbTest) {
        hidTestBody([](GLFWWindow &win) { win.setWinCallback(winCb::WindowPos, static_cast<std::function<void(int, int)>>([](int, int) { setCbCalled(); })); },
                    [] { w->setPosition(100, 100); });
    }

    // global Callbacks
    TEST(GLBaseTest, LibGLFWWindowKeyGlobalCbTest) {
        hidTestBody([](GLFWWindow &win) {
            win.setWinCallback(winCb::Key, [](int, int, int, int) { });
            win.addGlobalHidCallback(winCb::Key, nullptr, [](int, int, int, int) { setCbCalled(); });
        }, [] { SimulateKeyPress('B'); });
    }

    TEST(GLBaseTest, LibGLFWWindowCharGlobalCbTest) {
        hidTestBody([](GLFWWindow &win) {
            win.setWinCallback(winCb::Char, static_cast<std::function<void(unsigned int)>>([](unsigned int) { }));
            win.addGlobalHidCallback(winCb::Char, nullptr, static_cast<std::function<void(unsigned int)>>([](unsigned int) { setCbCalled(); }));
        }, [] { SimulateKeyPress('B'); });
    }


    TEST(GLBaseTest, LibGLFWMouseButGlobCbTest) {
        hidTestBody([](GLFWWindow &win) {
            win.setWinCallback(winCb::MouseButton, [](int, int, int) { });
            win.addGlobalHidCallback(winCb::MouseButton, nullptr, [](int, int, int) { setCbCalled(); });
        }, [] { SimulateMouseButtonClick(200, 200, 1); });
    }

    TEST(GLBaseTest, LibGLFWMouseCursorGlobCbTest) {
        hidTestBody([](GLFWWindow &win) {
            win.setWinCallback(winCb::CursorPos, static_cast<std::function<void(double, double)>>([](double, double) { }));
            win.addGlobalHidCallback(winCb::CursorPos, nullptr, static_cast<std::function<void(double, double)>>([](double, double) { setCbCalled(); }));
        }, [] { SimulateMouseMovement(200, 200, 250, 250); });
    }

    TEST(GLBaseTest, LibGLFWScrollGlobCbTest) {
        hidTestBody([](GLFWWindow &win) {
            win.setWinCallback(winCb::Scroll, static_cast<std::function<void(double, double)>>([](double, double) { }));
            win.addGlobalHidCallback(winCb::Scroll, nullptr, static_cast<std::function<void(double, double)>>([](double, double) { setCbCalled(); }));
        }, [] { SimulateWheel(w->getCtx(), 20); });
    }

    TEST(GLBaseTest, LibGLFWWindowCloseGlobCbTest) {
        hidTestBody([](GLFWWindow &win) {
            win.setWinCallback(winCb::WindowClose, [] { });
            win.addGlobalHidCallback(winCb::WindowClose, nullptr, [] { setCbCalled(); });
        }, [] { CloseWindow(w->getCtx()); });
    }

    TEST(GLBaseTest, LibGLFWWindowMaximizeGlobCbTest) {
        hidTestBody([](GLFWWindow &win) {
            win.setWinCallback(winCb::WindowMaximize, static_cast<std::function<void(int)>>([](int) { }));
            win.addGlobalHidCallback(winCb::WindowMaximize, nullptr, static_cast<std::function<void(int)>>([](int) { setCbCalled(); }));
        }, [] { glfwMaximizeWindow(w->getCtx()); std::this_thread::sleep_for(std::chrono::milliseconds(200)); });
    }

    TEST(GLBaseTest, LibGLFWWindowFocusGlobCbTest) {
        hidTestBody([](GLFWWindow &win) {
            win.setWinCallback(winCb::WindowFocus, static_cast<std::function<void(int)>>([](int) { }));
            win.addGlobalHidCallback(winCb::WindowFocus, nullptr, static_cast<std::function<void(int)>>([](int) { setCbCalled(); }));
        }, [] { });
    }

    TEST(GLBaseTest, LibGLFWWindowSizeGlobCbTest) {
        hidTestBody([](GLFWWindow &win) {
            win.setWinCallback(winCb::WindowSize, static_cast<std::function<void(int, int)>>([](int, int) { }));
            win.addGlobalHidCallback(winCb::WindowSize, nullptr, static_cast<std::function<void(int, int)>>([](int, int) { setCbCalled(); }));
        }, [] { w->resize(100, 100); });
    }

    TEST(GLBaseTest, LibGLFWWindowPosGlobCbTest) {
        hidTestBody([](GLFWWindow &win) {
            win.setWinCallback(winCb::WindowPos, static_cast<std::function<void(int, int)>>([](int, int) { }));
            win.addGlobalHidCallback(winCb::WindowPos, nullptr, static_cast<std::function<void(int, int)>>([](int, int) { setCbCalled(); }));
        }, [] { w->setPosition(100, 100); });
    }
#endif
}