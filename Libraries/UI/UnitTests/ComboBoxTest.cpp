//
// Created by sven on 03-04-25.
//
//

#include "TestCommon.h"

#include "UIApplication.h"
#include <UIElements/ComboBox.h>
#include <UIElements/Div.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::ComboBoxTest {

    void drawAndSwap(UIApplication& app) {
        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();
    }

    void addCombo(UIApplication& app, bool& flag) {
        auto rootNode = app.getMainWindow()->getRootNode();
        auto combo = rootNode->addChild<ComboBox>(UINodePars{
            .pos = ivec2{0,50},
            .size = ivec2{200,40},
            .fgColor = vec4{1.f, 1.f, 1.f, 1.f},
            .bgColor = vec4{.1f, .1f, .1f, 1.f},
            .borderWidth = 2,
            .borderRadius = 5,
            .borderColor = rootNode->getSharedRes()->colors->at(uiColors::blue),
            .padding = vec4{5.f, 5.f, 5.f, 5.f}
        });

        combo->setMenuName("ComboBox");
        combo->setFontType("regular");

        combo->addEntry("Entry 1", [&]{ LOG << " entry one "; flag = true; });
        combo->addEntry("Entry 2", [&]{ LOG << " entry two "; });
        combo->addEntry("Entry 3", [&]{ LOG << " entry three "; });
        combo->addEntry("Entry 4", [&]{ LOG << " entry four "; });
    }

    void openMenu(UIApplication& app) {
        drawAndSwap(app);
        app.getMainWindow()->onMouseDownLeft(175, 65, false, false, false);
    }

    void hoverOverFirstEntry(UIApplication& app) {
        drawAndSwap(app);
        app.getMainWindow()->onMouseMove(64, 95, 0);
    }

    TEST(UITest, ComboBoxTest) {
        bool entryOne = false;
        appBody([&](UIApplication& app){
            addCombo(app, entryOne);
        }, [&](UIApplication& app){
            //compareFrameBufferToImage(filesystem::current_path() / "combo_test_ref.png",
              //                        app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
        }, 600, 400);
    }

    TEST(UITest, ComboBoxTestClicked) {
        bool entryOne = false;
        appBody([&](UIApplication& app){
            addCombo(app, entryOne);
            openMenu(app);
        }, [&](UIApplication& app){
            compareFrameBufferToImage(filesystem::current_path() / "combo_test_ref2.png",
                                      app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
        }, 600, 400);
    }

    TEST(UITest, ComboBoxTestListHover) {
        bool entryOne = false;
        appBody([&](UIApplication& app){
            auto mainWin = app.getMainWindow();
            addCombo(app, entryOne);
            openMenu(app);
            hoverOverFirstEntry(app);
            drawAndSwap(app);
        }, [&](UIApplication& app){
            compareFrameBufferToImage(filesystem::current_path() / "combo_test_ref3.png",
                                      app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
        }, 600, 400);
    }

    TEST(UITest, ComboBoxTestListClicked) {
        bool entryOne = false;
        appBody([&](UIApplication& app){
            auto mainWin = app.getMainWindow();
            addCombo(app, entryOne);
            openMenu(app);
            hoverOverFirstEntry(app);

            drawAndSwap(app);
            mainWin->onMouseDownLeft(64, 95, false, false, false);
            mainWin->onMouseUpLeft();

        }, [&](UIApplication& app){
             compareFrameBufferToImage(filesystem::current_path() / "combo_test_ref.png",
                                      app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);

            ASSERT_TRUE(entryOne);
        }, 600, 400);
    }

}