//
// Created by sven on 17-11-25.
//

#include "TestCommon.h"
#include <UIElements/Text/TextBlock.h>
#include <UINodeFactory.h>

#ifdef ARA_USE_CLIP
#include "clip.h"
#endif

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::UIEditTest {

UIEdit& addFloatEdit(const UIApplication& app) {
    auto& ed = app.getRootNode()->push<UIEdit>(UINodePars {
        .pos = ivec2{10,10},
        .size = ivec2{200,28},
        .bgColor = vec4{.15f, .15f, .15f, 1.f}
    });
    ed.setOpt(UIEdit::single_line | UIEdit::num_fp);
    ed.setValue(1.234f);
    return ed;
}

std::string getSelAllCopyOutput(const UIApplication &app, UIEdit* ed) {
    std::string value;
#ifdef ARA_USE_CLIP
    const auto mainWin = app.getMainWindow();
    // simulate select all
    mainWin->onMouseDownLeft(102, 22, false, false, false);
    mainWin->onMouseUpLeft();

    mainWin->onMouseDownLeft(102, 22, false, false, false);
    mainWin->onMouseUpLeft();

    app.getWinBase()->draw(0, 0, 0);
    mainWin->swap();

    // simulate ctrl + c
    mainWin->onKeyDown(ARA_KEY_C, false, true, false);

    clip::get_text(value);
#endif
    return value;
}

TEST(UITest, UIEditFloatTest) {
    registerDefaultUITypes();

    appBody([&](const UIApplication &app) {
        auto& ed3 = addFloatEdit(app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditFloatBackspaceTest) {
    registerDefaultUITypes();

    UIEdit* ed = nullptr;
    appBody([&](const UIApplication &app) {
        ed = &addFloatEdit(app);

        const auto mainWin = app.getMainWindow();
        mainWin->onMouseDownLeft(122, 22, false, false, false);
        mainWin->onMouseUpLeft();

        app.getWinBase()->draw(0, 0, 0);
        mainWin->swap();

        mainWin->onKeyDown(ARA_KEY_BACKSPACE, false, false, false);
        mainWin->onKeyUp(ARA_KEY_BACKSPACE, false, false, false);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_backspace_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        const auto str = getSelAllCopyOutput(app, ed);
        EXPECT_EQ(str, "1.340");
        EXPECT_EQ(ed->getValue(), 1.34f);
    }, 300, 100);
}

TEST(UITest, UIEditFloatDelTest) {
    registerDefaultUITypes();

    UIEdit* ed = nullptr;
    appBody([&](const UIApplication &app) {
        ed = &addFloatEdit(app);

        const auto mainWin = app.getMainWindow();
        mainWin->onMouseDownLeft(114, 22, false, false, false);
        mainWin->onMouseUpLeft();

        app.getWinBase()->draw(0, 0, 0);
        mainWin->swap();

        mainWin->onKeyDown(ARA_KEY_DELETE, false, false, false);
        mainWin->onKeyUp(ARA_KEY_DELETE, false, false, false);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_del_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);

        const auto str = getSelAllCopyOutput(app, ed);
        EXPECT_EQ(str, "1.240");
        EXPECT_EQ(ed->getValue(), 1.24f);
    }, 300, 100);
}

TEST(UITest, UIEditFloatInsertTest) {
    registerDefaultUITypes();

    UIEdit* ed = nullptr;
    appBody([&](const UIApplication &app) {
        ed = &addFloatEdit(app);

        const auto mainWin = app.getMainWindow();
        mainWin->onMouseDownLeft(114, 22, false, false, false);
        mainWin->onMouseUpLeft();
        mainWin->onChar(ARA_KEY_7);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_insert_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);

        const auto str = getSelAllCopyOutput(app, ed);
        EXPECT_EQ(str, "1.2734");
        EXPECT_EQ(ed->getValue(), 1.2734f);
    }, 300, 100);
}

TEST(UITest, UIEditFloatWheelTest) {
    registerDefaultUITypes();

    UIEdit* ed = nullptr;
    appBody([&](const UIApplication &app) {
        ed = &addFloatEdit(app);
        ed->setUseWheel(true);

        const auto mainWin = app.getMainWindow();
        mainWin->onMouseDownLeft(122, 22, false, false, false);
        mainWin->onMouseUpLeft();
        mainWin->onWheel(1.f);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_wheel_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        const auto str = getSelAllCopyOutput(app, ed);
        EXPECT_EQ(str, "1.334");
        EXPECT_EQ(ed->getValue(), 1.334f);
    }, 300, 100);
}

}