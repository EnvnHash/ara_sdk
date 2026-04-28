//
// Created by sven on 11/15/20.
//

#include "TestCommon.h"

#include "UIApplication.h"
#include <UIElements/Button/Button.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::ButtonTests{

Button& setupAndDrawButton(const UIApplication& app, Property<bool>* prop=nullptr) {
    auto& but = setupTestButton(app, prop);
    iterate(app);
    return but;
}

TEST(UITest, BasicButtonTest) {
    appBody([&](const UIApplication& app){
        setupTestButton(app);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "butt_test.png",
                                  app.getWinBase()->getWidth(),
                                  app.getWinBase()->getHeight(),
                                  1);
    }, 800, 400);
}

TEST(UITest, ButtonClickTest) {
    bool flag = false;
    appBody([&](const UIApplication& app){
        setupTestButton(app).setClickedCb([&]{
            flag = true;
        });

        iterate(app);
        simulateMouseClick(app, 100, 50);
    }, [&](const UIApplication& app){
        ASSERT_TRUE(flag);
        compareFrameBufferToImage(filesystem::current_path() / "butt_test.png",
                                  app.getWinBase()->getWidth(),
                                  app.getWinBase()->getHeight(), 1);
    }, 800, 400);
}

TEST(UITest, ButtonPropertyTest) {
    Property prop = false;
    appBody([&](const UIApplication& app){
        setupAndDrawButton(app, &prop);
        prop = true;
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "butt_test_selected.png",
                                  app.getWinBase()->getWidth(),
                                  app.getWinBase()->getHeight(), 1);
        ASSERT_TRUE(prop());
    }, 800, 400);
}

TEST(UITest, ButtonPropertySelectDeselectTest) {
    Property prop = false;
    appBody([&](const UIApplication& app){
        setupAndDrawButton(app, &prop);
        iterate(app);
        simulateMouseClick(app, 100, 50);
        iterate(app);
        simulateMouseClick(app, 100, 50);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "butt_test.png",
                                  app.getWinBase()->getWidth(),
                                  app.getWinBase()->getHeight(), 1);
        ASSERT_FALSE(prop());
    }, 800, 400);
}

TEST(UITest, ButtonSelectDeselectTest) {
    appBody([&](const UIApplication& app){
        setupAndDrawButton(app);
        iterate(app);
        simulateMouseClick(app, 100, 50);
        iterate(app);
        simulateMouseClick(app, 100, 50);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "butt_test.png",
                                  app.getWinBase()->getWidth(),
                                  app.getWinBase()->getHeight(), 1);
    }, 800, 400);
}

TEST(UITest, ButtonPropertyOnChangedTest) {
    Property prop = false;
    appBody([&](const UIApplication& app){
        setupAndDrawButton(app, &prop);
        app.getMainWindow()->onMouseDownLeft(100, 50, false, false, false);
        app.getMainWindow()->onMouseUpLeft();
        iterate(app);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "butt_test_selected.png",
                                  app.getWinBase()->getWidth(),
                                  app.getWinBase()->getHeight(), 1);
        ASSERT_TRUE(prop());
    }, 800, 400);
}

}