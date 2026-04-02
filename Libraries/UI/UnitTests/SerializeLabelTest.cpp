//
// Created by sven on 17-11-25.
//

#include "TestCommon.h"
#include <UIElements/Image.h>
#include <UINodeFactory.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::LabelTest {


TEST(UITest, SerializeLabelSaveAndReload) {
    registerDefaultUITypes();

    appBody([&](const UIApplication &app) {
        auto root = app.getMainWindow()->getRootNode();

        auto& lbl = root->push<Label>(LabelPars{
          .pos = ivec2{ 10, 10},
          .size = ivec2{ 200, 100 },
          .align = align::center,
          .valign = valign::center,
          .color = vec4{ 0.8f, 0.6f, 1.f, 1.f },
          .bgColor = vec4{ 0.2f, 0.2f, 0.2f, 1.f },
          .text = "SerializeLabel",
          .textAlignX = align::center,
          .textAlignY = valign::center,
          .fontType = "regular",
          .fontHeight=24
        });

        root->saveAs("test.json");
        root->remove(lbl);
        root->load("test.json");
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "label_serialization_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 300);
}

TEST(UITest, SerializeLoadLabelTest) {
    registerDefaultUITypes();

    appBody([&](const UIApplication &app) {
        const auto root = app.getMainWindow()->getRootNode();
        root->load("SerializeFont.json");
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "label_serialization_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 300);
}

}