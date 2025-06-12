//
// Created by sven on 11/15/20.
//

#include "TestCommon.h"

#include "UIApplication.h"
#include <UIElements/Button/Button.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::ButtonTests{

Button* setupTestButton(UIApplication& app, Property<bool>* prop=nullptr) {
    auto rootNode = app.getMainWindow()->getRootNode();
    auto button = rootNode->addChild<Button>(UINodePars{
        .size = ivec2{200, 100},
        .fgColor = vec4{0.f, 0.f, 1.f, 1.f},
        .bgColor = vec4{0.2f, 0.2f, 0.2f, 1.f},
        .borderWidth = 1,
        .borderRadius = 25,
        .borderColor = vec4{1.f, 0.f, 0.f, 1.f},
        .padding = vec4{20.f, 0.f, 0.f, 0.f}
    });

    button->setFontSize(30);
    button->setText("HelloBut");
    button->setTextAlign(align::center, valign::center);
    button->setBackgroundColor(glm::vec4{0.4f, 0.4, 0.8f, 1.f}, state::selected);

    if (prop) {
        button->setIsToggle(true);
        button->setProp(*prop);
    }

    return button;
}

Button* setupAndDrawButton(UIApplication& app, Property<bool>* prop=nullptr) {
    auto but = setupTestButton(app, prop);
    app.getWinBase()->draw(0, 0, 0);
    app.getMainWindow()->swap();
    return but;
}

TEST(UITest, ButtonTests) {
    appBody([&](UIApplication& app){
        setupTestButton(app);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "butt_test.png",
                                  app.getWinBase()->getWidth(),
                                  app.getWinBase()->getHeight(),
                                  1);
    }, 800, 400);
}

TEST(UITest, ButtonClickTests) {
    bool flag = false;

    appBody([&](UIApplication& app){
        setupTestButton(app)->setClickedCb([&]{
            flag = true;
        });

        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();

        app.getMainWindow()->onMouseDownLeft(100, 50, false, false, false);
        app.getMainWindow()->onMouseUpLeft();
    }, [&](UIApplication& app){
        ASSERT_TRUE(flag);
        compareFrameBufferToImage(filesystem::current_path() / "butt_test.png",
                                  app.getWinBase()->getWidth(),
                                  app.getWinBase()->getHeight(), 1);
    }, 800, 400);
}

TEST(UITest, ButtonPropertyTests) {
    Property<bool> prop = false;

    appBody([&](UIApplication& app){
        setupAndDrawButton(app, &prop);
        prop = true;
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "butt_test_selected.png",
                                  app.getWinBase()->getWidth(),
                                  app.getWinBase()->getHeight(), 1);
        ASSERT_TRUE(prop());
    }, 800, 400);
}

TEST(UITest, ButtonPropertyOnChangedTests) {
    Property<bool> prop = false;

    appBody([&](UIApplication& app){
        setupAndDrawButton(app, &prop);
        app.getMainWindow()->onMouseDownLeft(100, 50, false, false, false);
        app.getMainWindow()->onMouseUpLeft();

        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "butt_test_selected.png",
                                  app.getWinBase()->getWidth(),
                                  app.getWinBase()->getHeight(), 1);
        ASSERT_TRUE(prop());
    }, 800, 400);
}

}