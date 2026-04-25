//
// Created by sven on 13-04-26.
//

#include "TestCommon.h"
#include "UIApplication.h"
#include <UIElements/DataBinding/NodeEdit.h>

using namespace glm;
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

TEST(UITest, StylesTest_NestedReferences) {
    appBody([&](const UIApplication &app) {
        const auto root = app.getRootNode();
        auto& parentNode = root->push<Div>({ .style = "parent_node" });
        auto& childNode = parentNode.push<Div>({ .style = "parent_node.child_node" });
        childNode.push<Div>({ .style = "parent_node.child_node.subchild_node" });
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "styles_test_nested.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 400, 200, nullptr, false, "test_res.txt");
}

TEST(UITest, StylesTest_WriteRuntimeChangeToDefaultStyle) {
    appBody([&](const UIApplication &app) {
        const auto root = app.getRootNode();
        auto& div = root->push<Div>({ .pos = ivec2{ 0, 10 }, .name ="myTestNode", .style = "nodeWithY" });
        iterate(app);
        div.setY(30);
        iterate(app);
        app.getMainWindow()->onMouseMove(20, 35, 0);
        iterate(app);
        app.getMainWindow()->onMouseMove(2, 2, 0);
    }, [&](const UIApplication &app) {
        checkQuad(app.getWinBase()->getWinHandle(), ivec2{ 10, 30 }, ivec2{40,40},
            vec4{ 1.f, 1.f, 0.f, 1.f}, vec4 { 0.f, 0.f, 0.f, 0.f });
    }, 400, 200, nullptr, false, "test_res.txt");
}

TEST(UITest, StylesTest_Button) {
    appBody([&](const UIApplication &app) {
        app.getRootNode()->push<Button>({ .style = "testButton" });
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "styles_testButton.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 400, 200, nullptr, false, "test_res.txt");
}

TEST(UITest, StylesTest_ButtonHover) {
    appBody([&](const UIApplication &app) {
        app.getRootNode()->push<Button>({ .name = "testButton", .style = "testButton" });
        iterate(app);
        app.getMainWindow()->onMouseMove(80, 20, 0);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "styles_testButtonHover.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 400, 200, nullptr, false, "test_res.txt");
}

TEST(UITest, StylesTest_ButtonSelected) {
    appBody([&](const UIApplication &app) {
        auto& butt = app.getRootNode()->push<Button>({ .name = "testButton", .style = "testButton" });
        butt.setIsToggle(true);
        iterate(app);
        simulateMouseClick(app, 100, 20);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "styles_testButtonSelected.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 400, 200, nullptr, false, "test_res.txt");
}

TEST(UITest, StylesTest_ButtonSelectedDeselect) {
    appBody([&](const UIApplication &app) {
        auto& butt = app.getRootNode()->push<Button>({ .name = "testButton", .style = "testButton" });
        butt.setIsToggle(true);
        iterate(app);
        simulateMouseClick(app, 100, 20);
        iterate(app);
        simulateMouseClick(app, 100, 20);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "styles_testButton.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 400, 200, nullptr, false, "test_res.txt");
}

}