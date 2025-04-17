//
// Created by sven on 11/15/20.
//

#include "test_common.h"

#include <UIApplication.h>
#include "UI/Button/Button.h"

using namespace std;

namespace ara::GLSceneGraphUnitTest::ButtonTests{

TEST(GLSceneGraphTest, ButtonTests) {
    glm::ivec2 buttSize{200, 100};

    appBody([&](UIApplication* app){
        auto rootNode = app->getMainWindow()->getRootNode();
        glm::vec4 fgColor(0.f, 0.f, 1.f, 1.f);
        glm::vec4 bgColor(0.2f, 0.2f, 0.2f, 1.f);
        auto button = rootNode->addChild<Button>(0, 0, buttSize.x, buttSize.y, fgColor, bgColor);
        button->setFontSize(50);
        button->setText("HelloBut");
        button->setTextAlign(align::left, valign::center);
        button->setPadding(20.f, 0.f, 0.f, 0.f);
        button->setBorderWidth(1);
        button->setBorderColor(1.f, 0.f, 0.f, 1.f);
        button->setBorderRadius(30);
    }, [&](UIApplication* app){
        auto data = getPixels(0, app->getWinBase()->getHeight() - buttSize.y, buttSize.x, buttSize.y);
        compareBitmaps(data, filesystem::current_path() / "butt_test.png", buttSize.x, buttSize.y);
    });
}


TEST(GLSceneGraphTest, ButtonClickTests) {
    glm::ivec2 buttSize{200, 100};
    bool flag = false;

    appBody([&](UIApplication* app){
        auto rootNode = app->getMainWindow()->getRootNode();
        glm::vec4 fgColor(0.f, 0.f, 1.f, 1.f);
        glm::vec4 bgColor(0.2f, 0.2f, 0.2f, 1.f);
        auto button = rootNode->addChild<Button>(0, 0, buttSize.x, buttSize.y, fgColor, bgColor);
        button->setFontSize(50);
        button->setText("HelloBut");
        button->setTextAlign(align::left, valign::center);
        button->setPadding(20.f, 0.f, 0.f, 0.f);
        button->setBorderWidth(1);
        button->setBorderColor(1.f, 0.f, 0.f, 1.f);
        button->setBorderRadius(30);
        button->setClickedCb([&]{
            flag = true;
        });

        app->getWinBase()->draw(0, 0, 0);
        app->getMainWindow()->swap();

        app->getMainWindow()->onMouseDownLeft(100, 50, false, false, false);
        app->getMainWindow()->onMouseUpLeft();

    }, [&](UIApplication* app){
        ASSERT_TRUE(flag);
        auto data = getPixels(0, app->getWinBase()->getHeight() - buttSize.y, buttSize.x, buttSize.y);
        compareBitmaps(data, filesystem::current_path() / "butt_test.png", buttSize.x, buttSize.y);
    });
}

}