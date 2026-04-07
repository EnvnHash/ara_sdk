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
class TestNode2 : public Node {
public:
    ARA_NODE_ADD_SERIALIZE_FUNCTIONS(Node, m_testVal, m_testVal2)
    TestNode2() { setTypeName<TestNode2>(); }
        T m_testVal{};
        T m_testVal2{};
};

template <typename T>
auto& addNodeEdit(const UIApplication &app, T& node, const arrange ar = arrange::horizontal, const optional<unordered_map<string, arrange>> alignMap = std::nullopt) {
    auto& ne = app.getRootNode()->push<NodeEdit>();
    if (alignMap.has_value()) {
        ne.setAlignPerKey(alignMap.value());
    } else {
        ne.setEditAlign(ar);
    }
    ne.setLineHeight(22);
    ne.setSpacing({10, 10});
    ne.setLabelWidth(100);
    ne.setNode(node);

    app.getWinBase()->draw(0, 0, 0);
    app.getMainWindow()->swap();
    return ne;
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
    const std::string testString = "hello";

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

TEST(UITest, NodeEditChangeArrayTest) {
    TestNode<std::vector<int32_t>> testNode;
    testNode.m_testValue = { 0, 1, 2 };

    appBody([&](const UIApplication &app) {
        addNodeEdit(app, testNode);

        const auto mainWin = app.getMainWindow();
        mainWin->onMouseMove(158, 12, 0);
        mainWin->onWheel(1.f);

        mainWin->onMouseMove(258, 12, 0);
        mainWin->onWheel(1.f);

        mainWin->onMouseMove(351, 12, 0);
        mainWin->onWheel(1.f);

    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "data_binding_change_array.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        EXPECT_EQ(testNode.m_testValue[0],1);
        EXPECT_EQ(testNode.m_testValue[1],2);
        EXPECT_EQ(testNode.m_testValue[2],3);
    }, 400, 200);
}

TEST(UITest, NodeEditChangeGlmIvec3Test) {
    TestNode<glm::ivec3> testNode;
    testNode.m_testValue = glm::ivec3{ 0, 1, 2 };

    appBody([&](const UIApplication &app) {
        addNodeEdit(app, testNode);

        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();

        const auto mainWin = app.getMainWindow();
        mainWin->onMouseMove(158, 12, 0);
        mainWin->onWheel(1.f);

        mainWin->onMouseMove(258, 12, 0);
        mainWin->onWheel(1.f);

        mainWin->onMouseMove(351, 12, 0);
        mainWin->onWheel(1.f);

    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "data_binding_change_array.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        EXPECT_EQ(testNode.m_testValue[0],1);
        EXPECT_EQ(testNode.m_testValue[1],2);
        EXPECT_EQ(testNode.m_testValue[2],3);
    }, 400, 200);
}

TEST(UITest, NodeEditChangeGlmVec3Test) {
    TestNode<glm::vec3> testNode;
    testNode.m_testValue = glm::vec3{ 0.f, 1.f, 2.f };

    appBody([&](const UIApplication &app) {
        addNodeEdit(app, testNode);

        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();

        const auto mainWin = app.getMainWindow();
        mainWin->onMouseMove(158, 12, 0);
        mainWin->onWheel(1.f);

        mainWin->onMouseMove(258, 12, 0);
        mainWin->onWheel(1.f);

        mainWin->onMouseMove(351, 12, 0);
        mainWin->onWheel(1.f);

    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "data_binding_change_vec3.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        EXPECT_EQ(testNode.m_testValue[0],0.1f);
        EXPECT_EQ(testNode.m_testValue[1],1.1f);
        EXPECT_EQ(testNode.m_testValue[2],2.1f);
    }, 400, 200);
}

TEST(UITest, NodeEditVectorStringVertAlignTest) {
    TestNode<std::vector<std::string>> testNode;
    testNode.m_testValue = { "text1", "text2" };

    appBody([&](const UIApplication &app) {
        addNodeEdit(app, testNode, arrange::vertical);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "data_binding_change_vertAlign.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 400, 200);
}

TEST(UITest, NodeEditMultiVar) {
    TestNode2<int32_t> testNode2;

    appBody([&](const UIApplication &app) {
        addNodeEdit(app, testNode2);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "data_binding_multi.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 400, 200);
}

TEST(UITest, NodeEditMixedAlignTest) {
    TestNode2<std::vector<std::string>> testNode;
    testNode.m_testVal = { "text1", "text2" };
    testNode.m_testVal2 = { "text3", "text4", "text5", "text6" };

    appBody([&](const UIApplication &app) {
        addNodeEdit(
            app,
            testNode,
            arrange::vertical,
          unordered_map<string, arrange>{ {"testVal", arrange::horizontal}, {"testVal2", arrange::vertical} }
        );
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "data_binding_change_mixedAlign.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 400, 200);
}

}