//
// Created by sven on 24-03-26.
//

#include "TestCommon.h"
#include "UIApplication.h"
#include <UIElements/DataBinding/NodeEdit.h>

using namespace std;

namespace ara::UiUnitTest::DataBindingTests {

class TestNode : public Node {
public:
    ARA_NODE_ADD_SERIALIZE_FUNCTIONS(Node, m_value)
    TestNode() {
        setTypeName<TestNode>();
    }
private:
    int32_t m_value=0;
};

void addNodeEdit(const UIApplication &app, TestNode& node) {
    auto& ne = app.getRootNode()->push<NodeEdit>();
    ne.setLineHeight(22);
    ne.setSpacing(10);
    ne.setLabelWidth(100);
    ne.setNode(node);

    app.getWinBase()->draw(0, 0, 0);
    app.getMainWindow()->swap();
}

TEST(UITest, NodeEditBasicTest) {
    TestNode testNode;

    appBody([&](const UIApplication &app) {
        addNodeEdit(app, testNode);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "data_binding_basic.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 400, 200);
}

TEST(UITest, NodeEditChangeValTest) {
    TestNode testNode;

    appBody([&](const UIApplication &app) {
        addNodeEdit(app, testNode);

        const auto mainWin = app.getMainWindow();
        mainWin->onMouseMove(255, 11, 0);
        mainWin->onWheel(1.f);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "data_binding_change_val.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 400, 200);
}

}