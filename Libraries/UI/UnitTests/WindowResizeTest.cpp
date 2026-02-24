//
// Created by sven on 24-02-26.
//

#include "TestCommon.h"
#include <UINodeFactory.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::WindowResizeTest {

#ifdef ARA_USE_GLFW

void setMouseTo(const UIApplication &app, ivec2 pos) {
    auto mainWin = app.getMainWindow();
    mainWin->onMouseMove(pos.x, pos.y, 0); // should set dragStart on WindowResizeArea
    glfwSetCursorPos(static_cast<GLFWwindow*>(mainWin->getWinHandle()->getWin()), pos.x, pos.y);
    app.getWinBase()->draw(0, 0, 0);
    app.getMainWindow()->swap();
}

void runScaling(const ivec2& initMp, const ivec2& moveTo, const ivec2& resultingWinSize) {
    registerDefaultUITypes();
    LOG << "WindowResizeTest: DON'T MOVE THE MOUSE WHILE RUNNING THIS TEST!!!";
    appBody([&](const UIApplication &app) {
        auto mainWin = app.getMainWindow();
        auto sz = mainWin->getSize();
        EXPECT_EQ(300, sz.x);
        EXPECT_EQ(300, sz.y);
    }, [&](const UIApplication &app) {
        auto mainWin = app.getMainWindow();
        for (int i=0; i<2; ++i) setMouseTo(app, initMp); // should set dragStart on WindowResizeArea
        mainWin->onMouseDownLeft(initMp.x, initMp.y, false, false, false);
        for (int i=0; i<2; ++i) setMouseTo(app, moveTo); // one redundant call to have winCB called
        mainWin->onMouseUpLeft();

        auto sz = mainWin->getSize();
        EXPECT_EQ(resultingWinSize.x, sz.x);
        EXPECT_EQ(resultingWinSize.y, sz.y);
    }, 300, 300, nullptr, true);
}

// DON'T MOVE THE MOUSE WHILE RUNNING THIS TEST!!!
TEST(UITest, WindowResizeTopTest) {
    runScaling(ivec2{150, 5}, ivec2{150, -50}, ivec2{300, 355});
}

TEST(UITest, WindowResizeBottomTest) {
    runScaling(ivec2{150, 295}, ivec2{150, 400}, ivec2{300, 405});
}

TEST(UITest, WindowResizeLeftTest) {
    runScaling(ivec2{5, 150}, ivec2{-50, 150}, ivec2{355, 300});
}

TEST(UITest, WindowResizeRightTest) {
    runScaling(ivec2{295, 150}, ivec2{400, 150}, ivec2{405, 300});
}

TEST(UITest, WindowResizeBottomRightTest) {
    runScaling(ivec2{295, 295}, ivec2{400, 400}, ivec2{405, 405});
}

TEST(UITest, WindowResizeTopRightTest) {
    runScaling(ivec2{295, 5}, ivec2{350, -50}, ivec2{355, 355});
}

TEST(UITest, WindowResizeTopLeftTest) {
    runScaling(ivec2{5, 5}, ivec2{-50, -50}, ivec2{355, 355});
}

TEST(UITest, WindowResizeBottomLeftTest) {
    runScaling(ivec2{5, 295}, ivec2{-50, 400}, ivec2{355, 405});
}

#endif


}
