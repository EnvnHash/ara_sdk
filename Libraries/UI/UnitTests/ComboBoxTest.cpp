//
// Created by sven on 03-04-25.
//
//

#include "TestCommon.h"

#include "UIApplication.h"
#include <UIElements/Menu/ComboBox.h>
#include <UIElements/Div.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::ComboBoxTest {
    ComboBox& addCombo(const UIApplication& app, bool& flag) {
        const auto rootNode = app.getMainWindow()->getRootNode();
        auto& combo = rootNode->push<ComboBox>(UINodePars{
            .pos = ivec2{0,50},
            .size = ivec2{200,40},
            .fgColor = vec4{1.f, 1.f, 1.f, 1.f},
            .bgColor = vec4{.1f, .1f, .1f, 1.f},
            .borderWidth = 2,
            .borderRadius = 5,
            .borderColor = rootNode->getSharedRes()->colors->at(uiColors::blue),
            .padding = vec4{5.f, 5.f, 5.f, 5.f}
        });

        combo.setMenuName("ComboBox");
        combo.setFontType("regular");

        combo.addEntry("Entry 1", [&]{ LOG << " entry one "; flag = true; });
        combo.addEntry("Entry 2", [&]{ LOG << " entry two "; });
        combo.addEntry("Entry 3", [&]{ LOG << " entry three "; });
        combo.addEntry("Entry 4", [&]{ LOG << " entry four "; });
        return combo;
    }

    void openMenu(const UIApplication& app) {
        iterate(app);
        app.getMainWindow()->onMouseDownLeft(175, 65, false, false, false);
        app.getMainWindow()->onMouseUpLeft();
    }

    void hoverOverFirstEntry(const UIApplication& app) {
        iterate(app);
        app.getMainWindow()->onMouseMove(64, 95, 0);
    }

    TEST(UITest, ComboBoxTest) {
        bool entryOne = false;
        appBody([&](const UIApplication& app){
            addCombo(app, entryOne);
        }, [&](UIApplication& app){
            //compareFrameBufferToImage(filesystem::current_path() / "combo_test_ref.png",
              //                        app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
        }, 600, 400);
    }

    TEST(UITest, ComboBoxTestClicked) {
        bool entryOne = false;
        appBody([&](const UIApplication& app){
            addCombo(app, entryOne);
            openMenu(app);
        }, [&](const UIApplication& app){
            compareFrameBufferToImage(filesystem::current_path() / "combo_test_ref2.png",
                                      app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
        }, 600, 400);
    }

    TEST(UITest, ComboBoxTestListHover) {
        bool entryOne = false;
        appBody([&](const UIApplication& app){
            addCombo(app, entryOne);
            openMenu(app);
            hoverOverFirstEntry(app);
            iterate(app);
        }, [&](const UIApplication& app){
              compareFrameBufferToImage(filesystem::current_path() / "combo_test_ref3.png",
                                      app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
        }, 600, 400);
    }

    TEST(UITest, ComboBoxTestMouseInOut) {
        bool entryOne = false;
        array<int32_t, 4> mouseInCntr{};
        array<int32_t, 4> mouseOutCntr{};

        appBody([&](const UIApplication& app){
            auto& combo = addCombo(app, entryOne);
            openMenu(app);
            const auto entr1 = combo.getEntry("Entry 1");
            entr1->addMouseInCb([&](hidData& d) { mouseInCntr[0]++; });
            entr1->addMouseOutCb([&](hidData& d) { mouseOutCntr[0]++; });

            hoverOverFirstEntry(app);
            iterate(app);
            app.getMainWindow()->onMouseMove(10, 10, 0);
        }, [&](const UIApplication& app){
              EXPECT_EQ(mouseInCntr[0], 1);
              EXPECT_EQ(mouseOutCntr[0], 1);
        }, 600, 400);
    }

    TEST(UITest, ComboBoxTestMouseInOut2) {
        bool entryOne = false;
        array<int32_t, 4> mouseInCntr{};
        array<int32_t, 4> mouseOutCntr{};

        appBody([&](const UIApplication& app){
            auto& combo = addCombo(app, entryOne);
            openMenu(app);
            const auto entr1 = combo.getEntry("Entry 1");
            entr1->addMouseInCb([&](hidData&) { mouseInCntr[0]++; });
            entr1->addMouseOutCb([&](hidData&) { mouseOutCntr[0]++; });

            const auto entr2 = combo.getEntry("Entry 2");
            entr2->addMouseInCb([&](hidData&) { mouseInCntr[1]++; });
            entr2->addMouseOutCb([&](hidData&) { mouseOutCntr[1]++; });

            hoverOverFirstEntry(app);
            iterate(app);
            app.getMainWindow()->onMouseMove(64, 95, 0);
            iterate(app);
            app.getMainWindow()->onMouseMove(64, 125, 0);
            iterate(app);
            app.getMainWindow()->onMouseMove(64, 220, 0);
        }, [&](const UIApplication& app){
            EXPECT_EQ(mouseInCntr[0], 1); EXPECT_EQ(mouseOutCntr[0], 1);
            EXPECT_EQ(mouseInCntr[1], 1); EXPECT_EQ(mouseOutCntr[1], 1);
        }, 600, 400);
    }

    TEST(UITest, ComboBoxTestListClicked) {
        bool entryOne = false;
        appBody([&](const UIApplication& app){
            const auto mainWin = app.getMainWindow();
            addCombo(app, entryOne);
            openMenu(app);
            hoverOverFirstEntry(app);

            iterate(app);
            mainWin->onMouseDownLeft(64, 95, false, false, false);
            mainWin->onMouseUpLeft();

        }, [&](const UIApplication& app){
             compareFrameBufferToImage(filesystem::current_path() / "combo_test_ref.png",
                                      app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);

            ASSERT_TRUE(entryOne);
        }, 600, 400);
    }

}