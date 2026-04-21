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

UIEdit& pushEdit(const UIApplication& app, int32_t width=200) {
    return app.getRootNode()->push<UIEdit>(UINodePars {
        .pos = ivec2{10,10},
        .size = ivec2{width,28},
        .bgColor = vec4{.15f, .15f, .15f, 1.f}
    });
}

UIEdit& addFloatEdit(const UIApplication& app) {
    auto& ed = pushEdit(app);
    ed.setOpt(UIEdit::single_line | UIEdit::num_fp);
    ed.setValue(1.234f);
    return ed;
}

UIEdit& addStringEdit(const UIApplication& app) {
    auto& ed = pushEdit(app, 202);
    ed.setOpt(UIEdit::single_line);
    ed.setBorderWidth(1);
    ed.setBorderColor(1.f, 1.f, 1.f, 1.f);
    ed.setTextAlignX(align::left);
    ed.setText("This is a line of text to test the label, long enough to see the ellipsis at the end.");
    return ed;
}

std::string getSelAllCopyOutput(const UIApplication &app) {
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

void clickEditField(const UIApplication& app, const int32_t xPos) {
    const auto mainWin = app.getMainWindow();
    mainWin->onMouseDownLeft(xPos, 22, false, false, false);
    mainWin->onMouseUpLeft();

    app.getWinBase()->draw(0, 0, 0);
    app.getMainWindow()->swap();
}

void clickArrowKey(const int key, const UIApplication& app) {
    const auto mainWin = app.getMainWindow();
    mainWin->onKeyDown(key, false, false, false);
    mainWin->onKeyUp(key, false, false, false);
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
        clickEditField(app, 122);
        clickArrowKey(ARA_KEY_BACKSPACE, app);
        clickArrowKey(ARA_KEY_ENTER, app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_backspace_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        const auto str = getSelAllCopyOutput(app);
        EXPECT_EQ(str, "1.340");
        EXPECT_EQ(ed->getValue(), 1.34f);
    }, 300, 100);
}

TEST(UITest, UIEditFloatBackspaceTest2) {
    registerDefaultUITypes();

    appBody([&](const UIApplication &app) {
        addFloatEdit(app);
        clickEditField(app, 122);
        clickArrowKey(ARA_KEY_BACKSPACE, app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_backspace_test2.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditFloatDelTest) {
    registerDefaultUITypes();

    UIEdit* ed = nullptr;
    appBody([&](const UIApplication &app) {
        ed = &addFloatEdit(app);

        clickEditField(app, 114);
        clickArrowKey(ARA_KEY_DELETE, app);
        clickArrowKey(ARA_KEY_ENTER, app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_del_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);

        const auto str = getSelAllCopyOutput(app);
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

        const auto str = getSelAllCopyOutput(app);
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
        const auto str = getSelAllCopyOutput(app);
        EXPECT_EQ(str, "1.334");
        EXPECT_EQ(ed->getValue(), 1.334f);
    }, 300, 100);
}

TEST(UITest, UIEditOverflowTest) {
    registerDefaultUITypes();

    appBody([&](const UIApplication &app) {
        addStringEdit(app);
        clickEditField(app, 195);
        clickArrowKey(ARA_KEY_RIGHT, app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_overflow_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditOverflowTestMoveCursoToEnd) {
    registerDefaultUITypes();

    appBody([&](const UIApplication &app) {
        addStringEdit(app);
        clickEditField(app, 195);
        for (int i = 0; i < 56; ++i) {
            clickArrowKey(ARA_KEY_RIGHT, app);
        }
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_overflow_test2.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditOverflowTestMoveCursoToEndAndBack) {
    registerDefaultUITypes();

    appBody([&](const UIApplication &app) {
        addStringEdit(app);
        clickEditField(app, 195);
        for (int i = 0; i < 56; ++i) {
            clickArrowKey(ARA_KEY_RIGHT, app);
        }
        for (int i = 0; i < 85; ++i) {
            clickArrowKey(ARA_KEY_LEFT, app);
        }
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_overflow_test3.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditOverflowTestEndKeyTest) {
    registerDefaultUITypes();

    appBody([&](const UIApplication &app) {
        addStringEdit(app);
        clickEditField(app, 195);
        clickArrowKey(ARA_KEY_END, app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_overflow_test2.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditOverflowTestEndAndHomeKey) {
    registerDefaultUITypes();

    appBody([&](const UIApplication &app) {
        addStringEdit(app);
        clickEditField(app, 195);
        clickArrowKey(ARA_KEY_END, app);
        clickArrowKey(ARA_KEY_HOME, app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_overflow_test3.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditTextResetTest) {
    registerDefaultUITypes();
    UIEdit* ed;
    appBody([&](const UIApplication &app) {
        ed = &addStringEdit(app);
        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();
        ed->setText("");
    }, [&](const UIApplication &app) {
        EXPECT_EQ(ed->getText(), "");
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_reset_text.png",
                          app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

}