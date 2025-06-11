//
// Created by sven on 11/15/20.
//

#include "TestCommon.h"

#include "UIApplication.h"
#include <UIElements/Button/Button.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::ButtonTests{

Button* setupTestButton(UIApplication& app) {
    auto rootNode = app.getMainWindow()->getRootNode();
    ivec2 buttSize{200, 100};

    auto button = rootNode->addChild<Button>(UINodePars{
        .size = buttSize,
        .fgColor = vec4{0.f, 0.f, 1.f, 1.f},
        .bgColor = vec4{0.2f, 0.2f, 0.2f, 1.f} });

    button->setFontSize(30);
    button->setText("HelloBut");
    button->setTextAlign(align::center, valign::center);
    button->setPadding(20.f, 0.f, 0.f, 0.f);
    button->setBorderWidth(1);
    button->setBorderColor(1.f, 0.f, 0.f, 1.f);
    button->setBorderRadius(25);
    return button;
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

}