//
// Created by sven on 24-03-26.
//

#include "TestCommon.h"
#include "UIApplication.h"
#include <UIElements/DataBinding/NodeEdit.h>

using namespace std;

namespace ara::UiUnitTest::DataBindingTests {

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
        simulateKeyPress(ARA_KEY_ENTER, app);
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

        iterate(app);

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

        iterate(app);

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
          unordered_map<string, VariableEditOption<>>{
            { "testVal", { arrange::horizontal } },
            {"testVal2", { arrange::vertical } } }
        );
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "data_binding_change_mixedAlign.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 400, 200);
}

}