//
// Created by sven on 03-04-25.
//
//

#include "TestCommon.h"

#include "UIApplication.h"
#include <UIElements/ComboBox.h>
#include <UIElements/Div.h>

using namespace std;

namespace ara::UiUnitTest::ComboBoxTest {

    void addCombo(UIApplication* app, bool& flag) {
        auto rootNode = app->getMainWindow()->getRootNode();

        auto combo = rootNode->addChild<ComboBox>();
        combo->setMenuName("ComboBox");
        combo->setPos(0,50);
        combo->setSize(200,40);
        combo->setFontType("regular");
        combo->setPadding(5.f);
        combo->setBackgroundColor(.1f, .1f, .1f, 1.f);
        combo->setBorderRadius(5);
        combo->setBorderWidth(2);
        combo->setBorderColor(rootNode->getSharedRes()->colors->at(uiColors::blue));
        combo->setColor(1.f, 1.f, 1.f, 1.f);

        combo->addEntry("Entry 1", [&]{ LOG << " entry one "; flag = true; });
        combo->addEntry("Entry 2", [&]{ LOG << " entry two "; });
        combo->addEntry("Entry 3", [&]{ LOG << " entry three "; });
        combo->addEntry("Entry 4", [&]{ LOG << " entry four "; });
    }

    TEST(UITest, ComboBoxTest) {
        bool entryOne = false;
        appBody([&](UIApplication* app){
            addCombo(app, entryOne);
        }, [&](UIApplication* app){
            compareFrameBufferToImage(filesystem::current_path() / "combo_test_ref.png",
                                      app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
        }, 600, 400);
    }

    TEST(UITest, ComboBoxTestClicked) {
        bool entryOne = false;
        appBody([&](UIApplication* app){
            auto mainWin = app->getMainWindow();

            addCombo(app, entryOne);

            app->getWinBase()->draw(0, 0, 0);
            app->getMainWindow()->swap();

            mainWin->onMouseDownLeft(175, 65, false, false, false);

        }, [&](UIApplication* app){
            compareFrameBufferToImage(filesystem::current_path() / "combo_test_ref2.png",
                                      app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
        }, 600, 400);
    }

    TEST(UITest, ComboBoxTestListHover) {
        bool entryOne = false;
        appBody([&](UIApplication* app){
            auto mainWin = app->getMainWindow();
            addCombo(app, entryOne);

            app->getWinBase()->draw(0, 0, 0);
            app->getMainWindow()->swap();

            mainWin->onMouseDownLeft(175, 65, false, false, false);

            app->getWinBase()->draw(0, 0, 0);
            app->getMainWindow()->swap();

            mainWin->onMouseMove(64, 95, 0);

            app->getWinBase()->draw(0, 0, 0);
            app->getMainWindow()->swap();

        }, [&](UIApplication* app){
            compareFrameBufferToImage(filesystem::current_path() / "combo_test_ref3.png",
                                      app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
        }, 600, 400);
    }

    TEST(UITest, ComboBoxTestListClicked) {
        bool entryOne = false;
        appBody([&](UIApplication* app){
            auto mainWin = app->getMainWindow();
            addCombo(app, entryOne);

            app->getWinBase()->draw(0, 0, 0);
            app->getMainWindow()->swap();

            mainWin->onMouseDownLeft(175, 65, false, false, false);

            app->getWinBase()->draw(0, 0, 0);
            app->getMainWindow()->swap();

            mainWin->onMouseMove(64, 95, 0);

            app->getWinBase()->draw(0, 0, 0);
            app->getMainWindow()->swap();

            mainWin->onMouseDownLeft(64, 95, false, false, false);
            mainWin->onMouseUpLeft();

        }, [&](UIApplication* app){
             compareFrameBufferToImage(filesystem::current_path() / "combo_test_ref.png",
                                      app->getWinBase()->getWidth(), app->getWinBase()->getHeight());

            ASSERT_TRUE(entryOne);
        }, 600, 400);
    }
}