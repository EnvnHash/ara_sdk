//
// Created by sven on 17-11-25.
//

#include "TestCommon.h"
#include <UIElements/Image.h>
#include <UINodeFactory.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::LabelTest {

auto& addLabel(UINode* root, const int32_t fontSize) {
    return root->push<Label>(LabelPars{
         .pos = ivec2{ 10, 10 },
         .size = ivec2{ 200, 100 },
         .align = align::center,
         .valign = valign::center,
         .color = vec4{ 0.8f, 0.6f, 1.f, 1.f },
         .bgColor = vec4{ 0.2f, 0.2f, 0.2f, 1.f },
         .text = "!7DK67/ú",
         .textAlignX = align::center,
         .textAlignY = valign::center,
         .fontType = "regular",
         .fontHeight = fontSize
    });
}

TEST(UITest, LabelTest) {
    registerDefaultUITypes();

    std::array fontSizeList { 8, 16, 24, 32, 40, 48 };
    for (auto& fsz: fontSizeList) {
        auto fn = "label_test_"+std::to_string(fsz)+".png";
        appBody([&](const UIApplication &app) {
            const auto root = app.getMainWindow()->getRootNode();
            addLabel(root, fsz);
        }, [&](const UIApplication &app) {
            compareFrameBufferToImage(filesystem::current_path() / fn,
                                      app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        }, 300, 300);
    }
}

}