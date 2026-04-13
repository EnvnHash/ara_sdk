//
// Created by sven on 13-04-26.
//

#include "TestCommon.h"
#include "UIApplication.h"
#include <UIElements/DataBinding/NodeEdit.h>

using namespace std;

namespace ara::UiUnitTest::StylesTests {

TEST(UITest, StylesTest_Basic) {
    TestNode<int32_t> testNode;

    appBody([&](const UIApplication &app) {
        addNodeEdit(app, testNode, arrange::horizontal, std::nullopt, "node_edit");
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "styles_test_basic.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 400, 200, nullptr, false, "test_res.txt");
}

TEST(UITest, StylesTest_NodeReference) {
    TestNode<int32_t> testNode;

    appBody([&](const UIApplication &app) {
        addNodeEdit(app, testNode, arrange::horizontal, std::nullopt, "reference_node");
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "styles_test_basic.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 400, 200, nullptr, false, "test_res.txt");
}

}