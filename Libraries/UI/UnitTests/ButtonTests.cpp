//
// Created by sven on 11/15/20.
//

#include "test_common.h"

#include "UIApplication.h"
#include <UIElements/Button/Button.h>
#include <DrawManagers/DrawManager.h>

using namespace std;

namespace ara::SceneGraphUnitTest::ButtonTests{

TEST(UITest, ButtonTests) {
    glm::ivec2 buttSize{200, 100};

    appBody([&](UIApplication* app){
        auto rootNode = app->getMainWindow()->getRootNode();
        glm::vec4 fgColor(0.f, 0.f, 1.f, 1.f);
        glm::vec4 bgColor(0.2f, 0.2f, 0.2f, 1.f);
        auto button = rootNode->addChild<Button>(0, 0, buttSize.x, buttSize.y, fgColor, bgColor);
        button->setFontSize(30);
        button->setText("HelloBut");
        button->setTextAlign(align::center, valign::center);
        button->setPadding(20.f, 0.f, 0.f, 0.f);
        button->setBorderWidth(1);
        button->setBorderColor(1.f, 0.f, 0.f, 1.f);
        button->setBorderRadius(25);
    }, [&](UIApplication* app){
        compareFrameBufferToImage(filesystem::current_path() / "butt_test.png", app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 800, 400);
}

TEST(UITest, ButtonClickTests) {
    glm::ivec2 buttSize{200, 100};
    bool flag = false;

    appBody([&](UIApplication* app){
        auto rootNode = app->getMainWindow()->getRootNode();
        glm::vec4 fgColor(0.f, 0.f, 1.f, 1.f);
        glm::vec4 bgColor(0.2f, 0.2f, 0.2f, 1.f);
        auto button = rootNode->addChild<Button>(0, 0, buttSize.x, buttSize.y, fgColor, bgColor);
        button->setFontSize(30);
        button->setText("HelloBut");
        button->setTextAlign(align::center, valign::center);
        button->setPadding(20.f, 0.f, 0.f, 0.f);
        button->setBorderWidth(1);
        button->setBorderColor(1.f, 0.f, 0.f, 1.f);
        button->setBorderRadius(25);
        button->setClickedCb([&]{
            flag = true;
        });

        app->getWinBase()->draw(0, 0, 0);
        app->getMainWindow()->swap();

        app->getMainWindow()->onMouseDownLeft(100, 50, false, false, false);
        app->getMainWindow()->onMouseUpLeft();

    }, [&](UIApplication* app){
        ASSERT_TRUE(flag);
        compareFrameBufferToImage(filesystem::current_path() / "butt_test.png", app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 800, 400);
}

}