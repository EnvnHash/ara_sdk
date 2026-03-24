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

TEST(UITest, NodeEditBasicTest) {
    TestNode testNode;

    appBody([&](const UIApplication &app) {
        auto& ne = app.getRootNode()->push<NodeEdit>();
        ne.setLineHeight(22);
        ne.setSpacing(10);
        ne.setLabelWidth(100);
        ne.setNode(testNode);
    }, [&](const UIApplication &app) {
        //Texture::saveFrontBuffer(filesystem::current_path() / "data_binding_basic.png", FIF_PNG,
//                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 4);

        compareFrameBufferToImage(filesystem::current_path() / "data_binding_basic.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 400, 200);
}

}