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

static inline string testText = "This is a line of text to test the label, long enough to see the ellipsis at the end.";

UIEdit& pushEdit(const UIApplication& app, const int32_t width=200) {
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
    ed.setText(testText);
    return ed;
}

UIEdit& addLongStringEdit(const UIApplication& app) {
    auto& edit = addStringEdit(app);
    edit.setFontSize(13);
    edit.setSize(280, 80);
    edit.removeOpt(UIEdit::single_line);
    string longText;
    for (int i=0; i<3; ++i) {
        longText += testText;
    }
    edit.setText(longText);
    return edit;
}

void clickEditField(const UIApplication& app, const int32_t xPos, const int32_t yPos=22) {
    const auto mainWin = app.getMainWindow();
    mainWin->onMouseDownLeft(static_cast<float>(xPos), static_cast<float>(yPos), false, false, false);
    mainWin->onMouseUpLeft();
    iterate(app);
}

std::string getSelAllCopyOutput(const UIApplication &app) {
    std::string value;
#ifdef ARA_USE_CLIP
    const auto mainWin = app.getMainWindow();
    // simulate select all
    clickEditField(app, 102);
    clickEditField(app, 102);
    iterate(app);

    // simulate ctrl + c
    mainWin->onKeyDown(ARA_KEY_C, false, true, false);

    clip::get_text(value);
#endif
    return value;
}

TEST(UITest, UIEditFloatTest) {
    appBody([&](const UIApplication &app) {
        addFloatEdit(app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditFloatBackspaceTest) {
    UIEdit* ed = nullptr;
    appBody([&](const UIApplication &app) {
        ed = &addFloatEdit(app);
        clickEditField(app, 122);
        simulateKeyPress(ARA_KEY_BACKSPACE, app);
        simulateKeyPress(ARA_KEY_ENTER, app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_backspace_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        const auto str = getSelAllCopyOutput(app);
        EXPECT_EQ(str, "1.340");
        EXPECT_EQ(ed->getValue(), 1.34f);
    }, 300, 100);
}

TEST(UITest, UIEditFloatBackspaceTest2) {

    appBody([&](const UIApplication &app) {
        addFloatEdit(app);
        clickEditField(app, 122);
        simulateKeyPress(ARA_KEY_BACKSPACE, app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_backspace_test2.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditFloatDelTest) {
    UIEdit* ed = nullptr;
    appBody([&](const UIApplication &app) {
        ed = &addFloatEdit(app);
        clickEditField(app, 114);
        simulateKeyPress(ARA_KEY_DELETE, app);
        simulateKeyPress(ARA_KEY_ENTER, app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_del_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        const auto str = getSelAllCopyOutput(app);
        EXPECT_EQ(str, "1.240");
        EXPECT_EQ(ed->getValue(), 1.24f);
    }, 300, 100);
}

TEST(UITest, UIEditFloatInsertTest) {

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
    UIEdit* ed = nullptr;
    appBody([&](const UIApplication &app) {
        ed = &addFloatEdit(app);
        ed->setUseWheel(true);
        simulateMouseClick(app, 122, 22);
        const auto mainWin = app.getMainWindow();
        mainWin->onWheel(1.f);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_wheel_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        const auto str = getSelAllCopyOutput(app);
        EXPECT_EQ(str, "1.334");
        EXPECT_EQ(ed->getValue(), 1.334f);
    }, 300, 100);
}

TEST(UITest, UIEditFloatWheelPlusCtrlTest) {
    UIEdit* ed = nullptr;
    appBody([&](const UIApplication &app) {
        ed = &addFloatEdit(app);
        ed->setUseWheel(true);
        iterate(app);
        const auto mainWin = app.getMainWindow();
        mainWin->onMouseMove(122, 22, 0);
        iterate(app);
        mainWin->onKeyDown(0, false, true, false);
        mainWin->onWheel(1.f);
        mainWin->onKeyUp(0, false, true, false);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_wheel_ctrl_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        const auto str = getSelAllCopyOutput(app);
        EXPECT_EQ(str, "1.244");
        EXPECT_EQ(ed->getValue(), 1.244f);
    }, 300, 100);
}

TEST(UITest, UIEditFloatWheelPlusShiftTest) {
    UIEdit* ed = nullptr;
    appBody([&](const UIApplication &app) {
        ed = &addFloatEdit(app);
        ed->setUseWheel(true);
        iterate(app);
        const auto mainWin = app.getMainWindow();
        mainWin->onMouseMove(122, 22, 0);
        iterate(app);

        mainWin->onKeyDown(0, true, false, false);
        mainWin->onWheel(1.f);
        mainWin->onKeyUp(0, true, false, false);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_wheel_shift_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        const auto str = getSelAllCopyOutput(app);
        EXPECT_EQ(str, "2.234");
        EXPECT_EQ(ed->getValue(), 2.234f);
    }, 300, 100);
}

TEST(UITest, UIEditFloatKeyUpPlusCtrlTest) {
    UIEdit* ed = nullptr;
    appBody([&](const UIApplication &app) {
        ed = &addFloatEdit(app);
        ed->setUseWheel(true);
        iterate(app);
        simulateMouseClick(app, 122, 22);
        iterate(app);
        const auto mainWin = app.getMainWindow();
        mainWin->onKeyDown(ARA_KEY_UP, false, true, false);
        mainWin->onKeyUp(ARA_KEY_UP, false, true, false);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_keyup_ctrl_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        const auto str = getSelAllCopyOutput(app);
        EXPECT_EQ(str, "1.244");
        EXPECT_EQ(ed->getValue(), 1.244f);
    }, 300, 100);
}

TEST(UITest, UIEditFloatKeyUpPlusShiftTest) {
    UIEdit* ed = nullptr;
    appBody([&](const UIApplication &app) {
        ed = &addFloatEdit(app);
        ed->setUseWheel(true);
        iterate(app);
        simulateMouseClick(app, 122, 22);
        iterate(app);
        const auto mainWin = app.getMainWindow();
        mainWin->onKeyDown(ARA_KEY_UP, true, false, false);
        mainWin->onKeyUp(ARA_KEY_UP, true, false, false);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_keyup_shift_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        const auto str = getSelAllCopyOutput(app);
        EXPECT_EQ(str, "2.234");
        EXPECT_EQ(ed->getValue(), 2.234f);
    }, 300, 100);
}

TEST(UITest, UIEditFloatWheelTest2) {
    UIEdit* ed = nullptr;
    appBody([&](const UIApplication &app) {
        ed = &addFloatEdit(app);
        ed->setUseWheel(true);
        ed->setWidth(30);
        iterate(app);
        const auto mainWin = app.getMainWindow();
        mainWin->onMouseMove(32, 22, 0);
        for (int i = 0; i < 5; ++i) {
            mainWin->onWheel(1.f);
            iterate(app);
        }
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_float_wheel_test2.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditOverflowTest) {
    appBody([&](const UIApplication &app) {
        addStringEdit(app);
        clickEditField(app, 195);
        simulateKeyPress(ARA_KEY_RIGHT, app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_overflow_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditOverflowTestMoveCursorToEnd) {

    appBody([&](const UIApplication &app) {
        addStringEdit(app);
        clickEditField(app, 195);
        for (int i = 0; i < 56; ++i) {
            simulateKeyPress(ARA_KEY_RIGHT, app);
        }
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_overflow_test2.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditOverflowTestMoveCursorToEndAndBack) {

    appBody([&](const UIApplication &app) {
        addStringEdit(app);
        clickEditField(app, 195);
        for (int i = 0; i < 56; ++i) {
            simulateKeyPress(ARA_KEY_RIGHT, app);
        }
        for (int i = 0; i < 85; ++i) {
            simulateKeyPress(ARA_KEY_LEFT, app);
        }
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_overflow_test3.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditOverflowTestEndKeyTest) {

    appBody([&](const UIApplication &app) {
        addStringEdit(app);
        clickEditField(app, 195);
        simulateKeyPress(ARA_KEY_END, app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_overflow_test2.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditOverflowTestEndAndHomeKey) {

    appBody([&](const UIApplication &app) {
        addStringEdit(app);
        clickEditField(app, 195);
        simulateKeyPress(ARA_KEY_END, app);
        simulateKeyPress(ARA_KEY_HOME, app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_overflow_test3.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditOverflowRepeatedHomeEnd) {
    UIEdit* ed = nullptr;
    appBody([&](const UIApplication &app) {
        ed = &addStringEdit(app);
        clickEditField(app, 105);
        simulateKeyPress(ARA_KEY_HOME, app);
        for (int i=0; i<20; i++) {
            simulateKeyPress(ARA_KEY_END, app);
            simulateKeyPress(ARA_KEY_HOME, app);
            iterate(app);
        }
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_overflow_test_front_end.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditTextResetTest) {
    UIEdit* ed;
    appBody([&](const UIApplication &app) {
        ed = &addStringEdit(app);
        iterate(app);
        ed->setText("");
    }, [&](const UIApplication &app) {
        EXPECT_EQ(ed->getText(), "");
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_reset_text.png",
                          app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditSingleLineClickEndTest) {
    appBody([&](const UIApplication &app) {
        auto& ed = addStringEdit(app);
        ed.setText("short Text");
        clickEditField(app, 100);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_string_click_end.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditSingleLineKeyEndTest) {
    appBody([&](const UIApplication &app) {
        auto& ed = addStringEdit(app);
        ed.setText("short Text");
        clickEditField(app, 20);
        simulateKeyPress(ARA_KEY_END, app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_string_click_end.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditMultiSetCursorCenterTest) {
    appBody([&](const UIApplication &app) {
        addLongStringEdit(app);
        clickEditField(app, 150, 50);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_string_multiline_select.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditMultiSetCursorCenterAndArrowRightTest) {
    appBody([&](const UIApplication &app) {
        addLongStringEdit(app);
        clickEditField(app, 150, 50);
        for (int i=0;i<4;i++) {
            simulateKeyPress(ARA_KEY_RIGHT, app);
        }
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_string_multiline_select_move.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditMultiSetCursorArrowRightAndRecenterTest) {
    appBody([&](const UIApplication &app) {
        addLongStringEdit(app);
        clickEditField(app, 150, 50);
        for (int i=0;i<4;i++) {
            simulateKeyPress(ARA_KEY_RIGHT, app);
        }
        iterate(app);
        clickEditField(app, 50, 50);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_string_multiline_reselect.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditMultiLineSetCursorToEnd) {
    appBody([&](const UIApplication &app) {
        addLongStringEdit(app);
        clickEditField(app, 282, 50);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_string_multiline_select_end.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditMultiLineMoveCursorUp) {
    appBody([&](const UIApplication &app) {
        addLongStringEdit(app);
        clickEditField(app, 150, 50);
        simulateKeyPress(ARA_KEY_UP, app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_string_multiline_moev_cursor_up.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditMultiLineMoveCursorDown) {
    appBody([&](const UIApplication &app) {
        addLongStringEdit(app);
        clickEditField(app, 150, 50);
        simulateKeyPress(ARA_KEY_DOWN, app);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_string_multiline_moev_cursor_down.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditStringCopyPaste) {
    UIEdit* edit2=nullptr;
    appBody([&](const UIApplication &app) {
        addStringEdit(app);
        edit2 = &addStringEdit(app);
        edit2->setText("");
        edit2->setY(40);
        const auto copied = getSelAllCopyOutput(app);
        EXPECT_EQ(copied, testText);
        clickEditField(app, 102, 52);
        const auto mainWin = app.getMainWindow();
        mainWin->onKeyDown(ARA_KEY_V, false, true, false);
    }, [&](const UIApplication &app) {
        EXPECT_EQ(edit2->getText(), testText);
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_copy_paste_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditStringCopyReplacePaste) {
    appBody([&](const UIApplication &app) {
        addStringEdit(app);
        auto& edit2 = addStringEdit(app);
        edit2.setText("Bing Bang");
        edit2.setY(40);
        const auto copied = getSelAllCopyOutput(app);
        EXPECT_EQ(copied, testText);
        clickEditField(app, 102, 52);
        clickEditField(app, 102, 52);
        const auto mainWin = app.getMainWindow();
        mainWin->onKeyDown(ARA_KEY_V, false, true, false);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_copy_paste_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditStringCopyInsertPaste) {
    appBody([&](const UIApplication &app) {
        addStringEdit(app);
        auto& edit2 = addStringEdit(app);
        edit2.setText("Bing Bang");
        edit2.setY(40);
#ifdef ARA_USE_CLIP
        clip::set_text("hello");
#endif
        clickEditField(app, 60, 52);
        const auto mainWin = app.getMainWindow();
        mainWin->onKeyDown(ARA_KEY_V, false, true, false);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_copy_insert_paste_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditStringCutAndPaste) {
    UIEdit* edit;
    appBody([&](const UIApplication &app) {
        edit = &addStringEdit(app);
        edit->setText("Bing Bang Dideldi");
        iterate(app);
        const auto mainWin = app.getMainWindow();
        mainWin->onMouseDownLeft(46, 25, false, false, false);
        mainWin->onMouseMove(89, 21, 0);
        mainWin->onMouseUpLeft();
        iterate(app);
        simulateKeyPress(ARA_KEY_X, app, false, true, false);
        iterate(app);
        EXPECT_EQ(edit->getText(), "Bing Dideldi");
        simulateMouseClick(app, 106, 21);
        simulateKeyPress(ARA_KEY_V, app, false, true, false);
    }, [&](const UIApplication &app) {
        EXPECT_EQ(edit->getText(), "Bing Dideldi Bang");
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_cut_and_paste_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

TEST(UITest, UIEditMultiLineCutAndPasteOnSelection) {
    UIEdit* edit;
    appBody([&](const UIApplication &app) {
        edit = &addLongStringEdit(app);
        iterate(app);
        const auto mainWin = app.getMainWindow();
        mainWin->onMouseDownLeft(67, 37, false, false, false);
        mainWin->onMouseMove(101, 50, 0);
        mainWin->onMouseUpLeft();
        iterate(app);
        simulateKeyPress(ARA_KEY_X, app, false, true, false);
        iterate(app);
        mainWin->onMouseDownLeft(38, 56, false, false, false);
        mainWin->onMouseMove(239, 57, 0);
        mainWin->onMouseUpLeft();
        iterate(app);
        simulateKeyPress(ARA_KEY_V, app, false, true, false);
    }, [&](const UIApplication &app) {
        EXPECT_EQ(edit->getText(), "This is a line of text to test the label, long enough to see the long enough to see the ellipsis at the end. ellipsis at the end.This is a line of text to test the label,long enough to see the ellipsis at the end.");
        compareFrameBufferToImage(filesystem::current_path() / "uiedit_string_multiline_cut_and_paste_on.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 100);
}

}