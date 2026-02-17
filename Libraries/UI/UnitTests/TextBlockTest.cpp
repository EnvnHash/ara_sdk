//
// Created by sven on 17-11-25.
//

#include "TestCommon.h"
#include <UIElements/Text/TextBlock.h>
#include <UINodeFactory.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::TextBlockTest {

std::string testText = "HEADLINE:\n\nLorem ipsum dolor sit amet, consectetur adipiscing elit. Sed neque ligula, tristique euismod scelerisque ut, finibus id libero. Praesent sagittis consectetur consequat. Integer et elit sed lorem finibus placerat in sit amet libero. Praesent sed nibh nec magna auctor aliquam quis ultrices sapien. ";

auto& createStdTextBlock(UIApplication &app) {
    auto root = app.getMainWindow()->getRootNode();
    return root->push<TextBlock>(UINodePars{
        .pos = ivec2{10,10},
        .size = ivec2{250,250},
        .bgColor = vec4{.15f, .15f, .15f, 1.f},
        .padding = vec4{2.f, 2.f, 2.f, 2.f},
    });
}

TEST(UITest, DrawTextBlock) {
    registerDefaultUITypes();

    auto compareList = {
        pair{align::left, std::string("textblock_left_test.png")},
        pair{align::center, std::string("textblock_center_test.png")},
        pair{align::right, std::string("textblock_right_test.png")},
        pair{align::justify, std::string("textblock_just_test.png")}
        //pair{align::justify_ex, std::string("textblock_just_ex_test.png")} at the moment the same as justify
    };

    for (auto& [al, fl] : compareList) {
        appBody([&](UIApplication &app) {
            auto& tb = createStdTextBlock(app);
            tb.setText(testText);
            tb.setTextAlignX(al);
        }, [&](UIApplication &app) {
            compareFrameBufferToImage(filesystem::current_path() / fl,
                                      app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        }, 300, 300);
    }
}

}