//
// Created by sven on 13-05-25.
//

#include "TestCommon.h"

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::ReloadGLContextTests {

TEST(UITest, ReloadGLContextTests) {
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

}