//
// Created by sven on 11/15/20.
//
#include "test_common.h"

#include <UIApplication.h>
#include <UI/ScrollView.h>
#include <UI/Div.h>

using namespace std;

namespace ara::GLSceneGraphUnitTest::ScrollViewTest{

    TEST(GLSceneGraphTest, ScrollViewTestNoScrollbar) {
        Texture tex;
        UIApplication app;
        appBody([&](UIApplication* app){
            auto rootNode = app->getMainWindow()->getRootNode();

            auto scrollView = rootNode->addChild<ScrollView>();
            scrollView->setAlign(align::center, valign::center);
            scrollView->setSize(0.7f, 0.7f);
            scrollView->setBackgroundColor(0.f, 0.f, 0.5f, 1.f);

            glm::vec4 bgColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.f);
            int chHeight = 40;

            for (int i = 0; i < 5; i++) {
                auto ch = scrollView->addChild<Div>();
                ch->setPos(10, (chHeight +10) * i );
                ch->setSize(30, chHeight);
                ch->setBackgroundColor(bgColor);
            }
        }, [&](UIApplication* app){
             compareFrameBufferToImage(filesystem::current_path() / "scrollview_test_ref.png",
                                      app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
        }, 600, 400);
    }

    TEST(GLSceneGraphTest, ScrollViewTestScrollbarVisible) {
        Texture tex;
        UIApplication app;
        appBody([&](UIApplication* app){
            auto rootNode = app->getMainWindow()->getRootNode();

            auto scrollView = rootNode->addChild<ScrollView>();
            scrollView->setAlign(align::center, valign::center);
            scrollView->setSize(0.7f, 0.7f);
            scrollView->setBackgroundColor(0.f, 0.f, 0.5f, 1.f);

            glm::vec4 bgColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.f);
            int chHeight = 40;

            for (int i = 0; i < 10; i++) {
                auto ch = scrollView->addChild<Div>();
                ch->setPos(10, (chHeight +10) * i );
                ch->setSize(30, chHeight);
                ch->setBackgroundColor(bgColor);
            }
        }, [&](UIApplication* app){
            compareFrameBufferToImage(filesystem::current_path() / "scrollview_test_ref2.png",
                                      app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
        }, 600, 400);
    }

    TEST(GLSceneGraphTest, ScrollViewTestScrollBarMoved) {
        Texture tex;
        UIApplication app;
        appBody([&](UIApplication* app){
            auto mainWin = app->getMainWindow();
            auto rootNode = mainWin->getRootNode();

            auto scrollView = rootNode->addChild<ScrollView>();
            scrollView->setAlign(align::center, valign::center);
            scrollView->setSize(0.7f, 0.7f);
            scrollView->setBackgroundColor(0.f, 0.f, 0.5f, 1.f);

            glm::vec4 bgColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.f);
            int chHeight = 40;

            for (int i = 0; i < 10; i++) {
                auto ch = scrollView->addChild<Div>();
                ch->setPos(10, (chHeight +10) * i );
                ch->setSize(30, chHeight);
                ch->setBackgroundColor(bgColor);
            }

            app->getWinBase()->draw(0, 0, 0);
            app->getMainWindow()->swap();

            // simulate dragging
            mainWin->onMouseDownLeft(500, 150, false, false, false);
            mainWin->onMouseMove(500, 152, 0);
            mainWin->onMouseMove(500, 200, 0);
            mainWin->onMouseUpLeft();

        }, [&](UIApplication* app){
            compareFrameBufferToImage(filesystem::current_path() / "scrollview_test_ref3.png",
                                      app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
        }, 600, 400);
    }
}