//
// Created by sven on 17-11-25.
//

#include "TestCommon.h"
#include <UIElements/Image.h>
#include <UINodeFactory.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::LabelTest {


TEST(UITest, LabelTest) {
    registerDefaultUITypes();

    appBody([&](UIApplication &app) {
        auto root = app.getMainWindow()->getRootNode();

        auto& lbl = root->push<Label>(LabelPars{
          .pos = ivec2{ 10, 10},
          .size = ivec2{ 200, 100 },
          .text_color = vec4{ 0.8f, 0.6f, 1.f, 1.f },
          .bg_color = vec4{ 0.2f, 0.2f, 0.2f, 1.f },
          .text = "Hello Bla",
          .text_align_x = align::center,
          .text_align_y = valign::center,
          .font_type = "regular",
          .font_height=24
        });
    }, [&](UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "label_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 300);
}

}