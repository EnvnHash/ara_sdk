//
// Created by sven on 13-05-25.
//

#include "TestCommon.h"
#include <UIElements/Label.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::ReloadGLContextTests {

TEST(UITest, ReloadGLContextQuadTests) {
    vec4 col = { 1.f, 0.f, 0.f, 1.f };
    ivec2 size = { 200, 200 };

    appRestartGL([&](UIApplication& app){
        auto div = app.getMainWindow()->getRootNode()->addChild<Div>();
        div->setSize(size.x, size.y);
        div->setBackgroundColor(col);
    }, [&](UIApplication& app){
        checkQuad(app.getWinBase()->getWinHandle(), { 0, 0 }, size, col, {});
        EXPECT_EQ(ara::postGLError(), GL_NO_ERROR);
    }, 400, 400);
}

TEST(UITest, ReloadGLContextImageTests) {
    appRestartGL([&](UIApplication& app){
        app.getMainWindow()->getRootNode()->addChild<Image>({ .size = ivec2{ 300, 300 } })->setImg("checkerboard_small.png", 1);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "image_reload.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        EXPECT_EQ(ara::postGLError(), GL_NO_ERROR);
    }, 400, 400);
}

TEST(UITest, ReloadGLContextLabelTests) {
    appRestartGL([&](UIApplication& app){
        auto label = app.getMainWindow()->getRootNode()->addChild<Label>({ .pos = ivec2{10, 10}, .size = ivec2{400, 100} });
        label->setFont("regular", 21, align::left, valign::top, {1.f, 1.f, 1.f, 1.f});
        label->setText("Test Test Test Test");
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "label_reload.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        EXPECT_EQ(ara::postGLError(), GL_NO_ERROR);
    }, 400, 400);
}

}