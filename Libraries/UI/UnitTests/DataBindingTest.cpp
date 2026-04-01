//
// Created by sven on 24-03-26.
//

#include "TestCommon.h"
#include "UIApplication.h"
#include <UIElements/DataBinding/NodeEdit.h>

using namespace std;

namespace ara::UiUnitTest::DataBindingTests {

template <typename T>
class TestNode : public Node {
public:
    ARA_NODE_ADD_SERIALIZE_FUNCTIONS(Node, m_testValue)
    TestNode() { setTypeName<TestNode>(); }
    T m_testValue{};
};

template <typename T>
void addNodeEdit(const UIApplication &app, TestNode<T>& node) {
    auto& ne = app.getRootNode()->push<NodeEdit>();
    ne.setLineHeight(22);
    ne.setSpacing(10);
    ne.setLabelWidth(100);
    ne.setNode(node);

    app.getWinBase()->draw(0, 0, 0);
    app.getMainWindow()->swap();
}

TEST(UITest, NodeEditBasicTest) {
    TestNode<int32_t> testNode;

    appBody([&](const UIApplication &app) {
        addNodeEdit(app, testNode);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "data_binding_basic.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 400, 200);
}

TEST(UITest, NodeEditChangeFloatTest) {
    TestNode<int32_t> testNode;

    appBody([&](const UIApplication &app) {
        addNodeEdit(app, testNode);

        const auto mainWin = app.getMainWindow();
        mainWin->onMouseMove(255, 11, 0);
        mainWin->onWheel(1.f);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "data_binding_change_val.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        EXPECT_EQ(testNode.m_testValue, 1);
    }, 400, 200);
}

TEST(UITest, NodeEditChangeStringTest) {
    TestNode<std::string> testNode;
    std::string testString = "hello";

    appBody([&](const UIApplication &app) {
        addNodeEdit(app, testNode);

        const auto mainWin = app.getMainWindow();
        mainWin->onMouseDownLeft(255, 11, false, false, false);
        mainWin->onMouseUpLeft();
        for (const auto &ch : testString) {
            mainWin->onChar(ch);
        }
        mainWin->onKeyDown(ARA_KEY_ENTER, false, false, false);
        mainWin->onKeyUp(ARA_KEY_ENTER, false, false, false);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "data_binding_change_string.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        EXPECT_EQ(testNode.m_testValue, "hello");
    }, 400, 200);
}

}