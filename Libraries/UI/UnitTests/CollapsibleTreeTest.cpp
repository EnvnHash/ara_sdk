//
// Created by sven on 03-04-25.
//
//

#include "TestCommon.h"

#include "UIElements/Menu/TreeCollapsible.h"

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::CollapsibleTreeTest {

Node m_node;

TreeCollapsible& addCollapsibleTree(UIApplication& app) {
    // Tree View, must use the ara sdk Node class or a derivative
    std::string str = R"({"children":[{"children":[{"name":"sub1_1_1","uuid":"1"}],"name":"sub1_1","uuid":"0"},{"children":[{"name":"sub1_2_1","uuid":"3"},{"name":"sub1_2_2","uuid":"4"}],"name":"sub1_2","uuid":"2"}],"name":"root","uuid":"10"})";
    m_node.loadFromString(str);

    auto& tree = app.getRootNode()->push<TreeCollapsible>(UINodePars{
        .pos = ivec2{10,10},
        .size = ivec2 {200, 160},
        .fgColor = vec4{ 1.f, 1.f, 1.f, 1.f },
        .bgColor = vec4{ .1f, .1f, .1f, 1.f },
        .borderWidth = 2,
        .borderRadius = 5,
        .borderColor = vec4{ .1f, .1f, 1.f, 1.f },
        .padding = vec4{ 5, 5, 5,5 },
    });

    tree.setNode(&m_node);
    return tree;
}

TEST(UITest, CollapsibleTreeTest) {
    appBody([&](UIApplication& app){
        addCollapsibleTree(app);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "collapse_list_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 250, 200);
}

TEST(UITest, CollapsibleTreeExp1Test) {
    TreeCollapsible* tree;
    appBody([&](UIApplication& app){
        tree = &addCollapsibleTree(app);
    }, [&](UIApplication& app){
        auto mainWin = app.getMainWindow();
        mainWin->onMouseDownLeft(32, 25, false, false, false);
        mainWin->onMouseUpLeft();

        app.getWinBase()->draw(0, 0, 0);
        mainWin->swap();

        Texture::saveFrontBuffer(filesystem::current_path() / "collapse_list_exp1_test.png", FIF_PNG,
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 4);

        compareFrameBufferToImage(filesystem::current_path() / "collapse_list_exp1_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 250, 200);
}

}