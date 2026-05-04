//
// Created by sven on 04-05-26.
//

#include "TestCommon.h"

#include "UIApplication.h"
#include <UIElements/Div.h>

using namespace glm;
using namespace std;

namespace ara::UiUnitTest::DivTest {

TEST(UITest, DivBorderOnePixTest) {
    Div* div = nullptr;
    appBody([&](const UIApplication& app) {
        const auto rootNode = app.getMainWindow()->getRootNode();
        div = &rootNode->push<Div>({
            .pos = ivec2{ 20, 20 },
            .size = ivec2{ 100, 100 },
            .borderWidth = 1,
            .borderColor = vec4{1.f, 1.f, 1.f, 1.f}
        });
    }, [&](const UIApplication& app){
        for (int i=1; i<4; i++) {
            auto fn = "Div_border_test_"+std::to_string(i)+".png";
            compareFrameBufferToImage(filesystem::current_path() / fn,
                                      app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
            div->setPos(ivec2{ 20+i, 20+i });
            iterate(app);
        }
    }, 200, 200);
}

TEST(UITest, DivBorderOnePixFloatTest) {
    appBody([&](const UIApplication& app) {
        const auto rootNode = app.getMainWindow()->getRootNode();
        auto& div = rootNode->push<Div>({
            .pos = vec2{ 0.1025f, 0.1025f }, // half-pix offset
            .size = ivec2{ 100, 100 },
            .borderWidth = 1,
            .borderColor = vec4{1.f, 1.f, 1.f, 1.f}
        });
        div.setSnapToHwPix(true);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "Div_border_test_2.png",
                                      app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 200, 200);
}

}