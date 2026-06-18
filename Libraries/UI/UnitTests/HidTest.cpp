//
// Created by sven on 07-04-25.
//

#include "TestCommon.h"
#include <UIElements/Div.h>

using namespace std;

namespace ara::UiUnitTest::HidTest {

class HidNode :  public Div {
public:
    void init() override {
        Div::init();
        resetClickFlags();
    }

    void resetClickFlags() {
        for (auto i=0; i<static_cast<int32_t>(hidEvent::Size);i++) {
            m_clicked[static_cast<hidEvent>(i)] = false;
        }
    }

    void mouseDown(hidData& data) override {
        m_clicked[hidEvent::MouseUpLeft] = false;
        m_clicked[hidEvent::MouseDownLeft] = true;
        if (m_consume) {
            data.consumed = true;
        }
    }

    void mouseUp(hidData& data) override {
        m_clicked[hidEvent::MouseDownLeft] = false;
        m_clicked[hidEvent::MouseUpLeft] = true;
        if (m_consume) {
            data.consumed = true;
        }
    }

    void mouseDownRight(hidData& data) override {
        m_clicked[hidEvent::MouseUpRight] = false;
        m_clicked[hidEvent::MouseDownRight] = true;
        if (m_consume) {
            data.consumed = true;
        }
    }

    void mouseUpRight(hidData& data) override {
        m_clicked[hidEvent::MouseUpRight] = true;
        m_clicked[hidEvent::MouseDownRight] = false;
        if (m_consume) {
            data.consumed = true;
        }
    }

    void mouseIn(hidData &data) override {
        m_mouseIn++;
    }

    void mouseOut(hidData &data) override {
        m_mouseOut++;
    }

    std::unordered_map<hidEvent, bool> m_clicked{};
    bool m_consume = false;
    int32_t m_mouseIn = 0;
    int32_t m_mouseOut = 0;
};

static HidNode* addDiv(const UIApplication& app) {
    const auto rootNode = app.getMainWindow()->getRootNode();
    auto& div = rootNode->push<HidNode>();
    div.setPos(50,50);
    div.setSize(200,100);
    div.setBackgroundColor(1.f, 0.f, 0.f, 1.f);
    return &div;
}

static void simulateClickLeftRight(UIWindow* mainWin, HidNode* div, const glm::vec2& pos, const bool expVal) {
    mainWin->onMouseDownLeft(pos.x, pos.y, false, false, false);
    EXPECT_EQ(div->m_clicked[hidEvent::MouseDownLeft], expVal);
    mainWin->onMouseUpLeft();
    EXPECT_EQ(div->m_clicked[hidEvent::MouseUpLeft], expVal);

    mainWin->onMouseDownRight(pos.x, pos.y, false, false, false);
    EXPECT_EQ(div->m_clicked[hidEvent::MouseDownRight], expVal);
    mainWin->onMouseUpRight();
    EXPECT_EQ(div->m_clicked[hidEvent::MouseUpRight], expVal);
}

static std::unordered_map<hidEvent, bool> initCallbacks() {
    std::unordered_map<hidEvent, bool> cbCalled{};
    for (auto i=0; i<static_cast<int32_t>(hidEvent::Size);i++) {
        cbCalled[static_cast<hidEvent>(i)] = false;
    }
    return cbCalled;
}

static void setHidCallbacks(HidNode* div, std::unordered_map<hidEvent, bool>& cbCalled) {
    div->addMouseClickCb([&](hidData& data){
        cbCalled[hidEvent::MouseDownLeft] = true;
    });
    div->addMouseUpCb([&](hidData& data){
        cbCalled[hidEvent::MouseUpLeft] = true;
    });
    div->addMouseClickRightCb([&](hidData& data){
        cbCalled[hidEvent::MouseDownRight] = true;
    });
    div->addMouseUpRightCb([&](hidData& data){
        cbCalled[hidEvent::MouseUpRight] = true;
    });
}

static void checkCbCalled(std::unordered_map<hidEvent, bool>& cbCalled, const bool expVal) {
    EXPECT_EQ(cbCalled[hidEvent::MouseDownLeft], expVal);
    EXPECT_EQ(cbCalled[hidEvent::MouseUpLeft], expVal);
    EXPECT_EQ(cbCalled[hidEvent::MouseDownRight], expVal);
    EXPECT_EQ(cbCalled[hidEvent::MouseUpRight], expVal);
}

static void simulateAndCheckClick(const UIApplication& app, HidNode* div, std::unordered_map<hidEvent, bool>& cb,
                                  const glm::vec2 pos, const bool expectedValue) {
    const auto mainWin = app.getMainWindow();
    simulateClickLeftRight(mainWin, div, pos, expectedValue);
    iterate(app);

    checkCbCalled(cb, expectedValue);
}

TEST(UITest, HidDefaultTest) {
    HidNode* div = nullptr;
    appBody([&](const UIApplication& app){
        div = addDiv(app);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "hid_node_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight());

        app.getMainWindow()->onMouseDownLeft(150, 100, false, false, false);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseDownLeft]);         // div should not be clickable
    }, 600, 400);
}

TEST(UITest, HidDivClickTest) {
    HidNode* div = nullptr;
    appBody([&](const UIApplication& app){
        div = addDiv(app);
    }, [&](const UIApplication& app){
        simulateClickLeftRight(app.getMainWindow(), div, {150, 100}, true);
    }, 600, 400);
}

TEST(UITest, HidDivClickNegTest) {
    HidNode* div = nullptr;
    appBody([&](const UIApplication& app){
        div = addDiv(app);
    }, [&](const UIApplication& app){
        simulateClickLeftRight(app.getMainWindow(), div, {260, 100}, false);
    }, 600, 400);
}

TEST(UITest, HidDivClickCallbackTest) {
    HidNode* div = nullptr;
    auto cbCalled = initCallbacks();

    appBody([&](const UIApplication& app){
        div = addDiv(app);
        setHidCallbacks(div, cbCalled);
    }, [&](const UIApplication& app){
        simulateAndCheckClick(app, div, cbCalled, {150, 100}, true);
    }, 600, 400);
}

TEST(UITest, HidDivClickCallbackNegTest) {
    HidNode* div = nullptr;
    auto cbCalled = initCallbacks();

    appBody([&](const UIApplication& app){
        div = addDiv(app);
        setHidCallbacks(div, cbCalled);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "hid_node_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight());

        simulateAndCheckClick(app, div, cbCalled, {260, 100}, false);
    }, 600, 400);
}

TEST(UITest, HidDivNestedClick) {
    HidNode* div = nullptr;
    HidNode* childDiv = nullptr;

    appBody([&](const UIApplication& app){
        div = addDiv(app);
        div->setName("div1");

        childDiv = &div->push<HidNode>();
        childDiv->setName("div2");
        childDiv->setSize(50,50);
        childDiv->setPos(20,20);
        childDiv->setBackgroundColor(0.f, 0.f, 1.f, 1.f);

    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "hid_node_test2.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight());

        const auto mainWin = app.getMainWindow();

        mainWin->onMouseDownLeft(95, 95, false, false, false);
        EXPECT_TRUE(childDiv->m_clicked[hidEvent::MouseDownLeft]);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseDownLeft]);

        mainWin->onMouseUpLeft();
        EXPECT_TRUE(childDiv->m_clicked[hidEvent::MouseUpLeft]);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseUpLeft]);

        mainWin->onMouseDownLeft(175, 95, false, false, false);
        EXPECT_FALSE(childDiv->m_clicked[hidEvent::MouseDownLeft]);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseDownLeft]);

        mainWin->onMouseUpLeft();

        mainWin->onMouseDownRight(95, 95, false, false, false);
        EXPECT_TRUE(childDiv->m_clicked[hidEvent::MouseDownRight]);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseDownRight]);

        mainWin->onMouseUpRight();
        EXPECT_TRUE(childDiv->m_clicked[hidEvent::MouseUpRight]);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseUpRight]);

        mainWin->onMouseDownRight(175, 95, false, false, false);
        EXPECT_FALSE(childDiv->m_clicked[hidEvent::MouseDownRight]);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseDownRight]);

    }, 600, 400);
}

TEST(UITest, HidDivDoubleNestedClick) {
    HidNode* div = nullptr;
    HidNode* childDiv = nullptr;
    HidNode* childChildDiv = nullptr;

    appBody([&](const UIApplication& app){
        div = addDiv(app);

        childDiv = &div->push<HidNode>();
        childDiv->setSize(150,50);
        childDiv->setPos(20,20);
        childDiv->setBackgroundColor(0.f, 0.f, 1.f, 1.f);

        childChildDiv = &childDiv->push<HidNode>();
        childChildDiv->setSize(30,30);
        childChildDiv->setPos(10,10);
        childChildDiv->setBackgroundColor(0.f, 1.f, 0.f, 1.f);

    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "hid_node_test3.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight());

        auto mainWin = app.getMainWindow();
        mainWin->onMouseDownLeft(95, 95, false, false, false);
        EXPECT_TRUE(childChildDiv->m_clicked[hidEvent::MouseDownLeft]);
        EXPECT_TRUE(childDiv->m_clicked[hidEvent::MouseDownLeft]);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseDownLeft]);

        mainWin->onMouseUpLeft();
        EXPECT_TRUE(childChildDiv->m_clicked[hidEvent::MouseUpLeft]);
        EXPECT_TRUE(childDiv->m_clicked[hidEvent::MouseUpLeft]);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseUpLeft]);

        mainWin->onMouseDownLeft(175, 95, false, false, false);
        EXPECT_FALSE(childChildDiv->m_clicked[hidEvent::MouseDownLeft]);
        EXPECT_TRUE(childDiv->m_clicked[hidEvent::MouseDownLeft]);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseDownLeft]);
        mainWin->onMouseUpLeft();

        mainWin->onMouseDownRight(95, 95, false, false, false);
        EXPECT_TRUE(childChildDiv->m_clicked[hidEvent::MouseDownRight]);
        EXPECT_TRUE(childDiv->m_clicked[hidEvent::MouseDownRight]);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseDownRight]);

        mainWin->onMouseUpRight();
        EXPECT_TRUE(childChildDiv->m_clicked[hidEvent::MouseUpRight]);
        EXPECT_TRUE(childDiv->m_clicked[hidEvent::MouseUpLeft]);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseUpLeft]);

        mainWin->onMouseDownRight(175, 95, false, false, false);
        EXPECT_FALSE(childChildDiv->m_clicked[hidEvent::MouseDownRight]);
        EXPECT_TRUE(childDiv->m_clicked[hidEvent::MouseDownRight]);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseDownRight]);

    }, 600, 400);
}

TEST(UITest, HidDivNestedClickConsume) {
    HidNode* div = nullptr;
    HidNode* childDiv = nullptr;
    HidNode* childChildDiv = nullptr;

    appBody([&](const UIApplication& app){
        div = addDiv(app);

        childDiv = &div->push<HidNode>();
        childDiv->setSize(150,50);
        childDiv->setPos(20,20);
        childDiv->setBackgroundColor(0.f, 0.f, 1.f, 1.f);

        childChildDiv = &childDiv->push<HidNode>();
        childChildDiv->setSize(30,30);
        childChildDiv->setPos(10,10);
        childChildDiv->setBackgroundColor(0.f, 1.f, 0.f, 1.f);
        childChildDiv->m_consume = true;

    }, [&](const UIApplication& app){
        auto mainWin = app.getMainWindow();
        mainWin->onMouseDownLeft(95, 95, false, false, false);
        EXPECT_TRUE(childChildDiv->m_clicked[hidEvent::MouseDownLeft]);
        EXPECT_FALSE(childDiv->m_clicked[hidEvent::MouseDownLeft]);
        EXPECT_FALSE(div->m_clicked[hidEvent::MouseDownLeft]);

        mainWin->onMouseUpLeft();
        EXPECT_TRUE(childChildDiv->m_clicked[hidEvent::MouseUpLeft]);
        EXPECT_FALSE(childDiv->m_clicked[hidEvent::MouseUpLeft]);
        EXPECT_FALSE(div->m_clicked[hidEvent::MouseUpLeft]);

        mainWin->onMouseDownLeft(175, 95, false, false, false);
        EXPECT_FALSE(childChildDiv->m_clicked[hidEvent::MouseDownLeft]);
        EXPECT_TRUE(childDiv->m_clicked[hidEvent::MouseDownLeft]);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseDownLeft]);
        mainWin->onMouseUpLeft();

        mainWin->onMouseDownRight(95, 95, false, false, false);
        EXPECT_TRUE(childChildDiv->m_clicked[hidEvent::MouseDownRight]);
        EXPECT_FALSE(childDiv->m_clicked[hidEvent::MouseDownRight]);
        EXPECT_FALSE(div->m_clicked[hidEvent::MouseDownRight]);

        mainWin->onMouseUpRight();
        EXPECT_TRUE(childChildDiv->m_clicked[hidEvent::MouseUpRight]);
        EXPECT_FALSE(childDiv->m_clicked[hidEvent::MouseUpRight]);
        EXPECT_FALSE(div->m_clicked[hidEvent::MouseUpRight]);

        mainWin->onMouseDownRight(175, 95, false, false, false);
        EXPECT_FALSE(childChildDiv->m_clicked[hidEvent::MouseDownRight]);
        EXPECT_TRUE(childDiv->m_clicked[hidEvent::MouseDownRight]);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseDownRight]);
        mainWin->onMouseUpRight();

        childChildDiv->resetClickFlags();
        childChildDiv->m_consume = false;
        childDiv->resetClickFlags();
        childDiv->m_consume = true;
        div->resetClickFlags();

        mainWin->onMouseDownLeft(95, 95, false, false, false);
        EXPECT_TRUE(childChildDiv->m_clicked[hidEvent::MouseDownLeft]);
        EXPECT_TRUE(childDiv->m_clicked[hidEvent::MouseDownLeft]);
        EXPECT_FALSE(div->m_clicked[hidEvent::MouseDownLeft]);

        mainWin->onMouseUpLeft();
        EXPECT_TRUE(childChildDiv->m_clicked[hidEvent::MouseUpLeft]);
        EXPECT_TRUE(childDiv->m_clicked[hidEvent::MouseUpLeft]);
        EXPECT_FALSE(div->m_clicked[hidEvent::MouseUpLeft]);

        mainWin->onMouseDownRight(95, 95, false, false, false);
        EXPECT_TRUE(childChildDiv->m_clicked[hidEvent::MouseDownRight]);
        EXPECT_TRUE(childDiv->m_clicked[hidEvent::MouseDownRight]);
        EXPECT_FALSE(div->m_clicked[hidEvent::MouseDownRight]);

        mainWin->onMouseUpRight();
        EXPECT_TRUE(childChildDiv->m_clicked[hidEvent::MouseUpRight]);
        EXPECT_TRUE(childDiv->m_clicked[hidEvent::MouseUpRight]);
        EXPECT_FALSE(div->m_clicked[hidEvent::MouseUpRight]);
    }, 600, 400);
}

void clickNestedExpectFalseFalseTrue(HidNode* childChildDiv, HidNode* childDiv, HidNode* div, const hidEvent evt) {
    EXPECT_FALSE(childChildDiv->m_clicked[evt]);
    EXPECT_FALSE(childDiv->m_clicked[evt]);
    EXPECT_TRUE(div->m_clicked[evt]);
}

void clickNestedExpectAllFalse(HidNode* childChildDiv, HidNode* childDiv, HidNode* div, const hidEvent evt) {
    EXPECT_FALSE(childChildDiv->m_clicked[evt]);
    EXPECT_FALSE(childDiv->m_clicked[evt]);
    EXPECT_FALSE(div->m_clicked[evt]);
}

TEST(UITest, HidDivNestedClickExclude) {
    HidNode* div = nullptr;
    HidNode* childDiv = nullptr;
    HidNode* childChildDiv = nullptr;

    appBody([&](const UIApplication& app){
        div = addDiv(app);
        div->setName("div1");

        childDiv = &div->push<HidNode>({
            .pos = glm::ivec2{20,20},
            .size = glm::ivec2{150,50},
            .bgColor = glm::vec4{0.f, 0.f, 1.f, 1.f}
        });
        childDiv->excludeFromObjMap(true);
        childDiv->setName("div2");

        childChildDiv = &childDiv->push<HidNode>({
            .pos = glm::ivec2{10,10},
            .size = glm::ivec2{30,30},
            .bgColor = glm::vec4{0.f, 1.f, 0.f, 1.f}
        });
        childChildDiv->setName("div3");
    }, [&](const UIApplication& app){
        const auto mainWin = app.getMainWindow();
        mainWin->onMouseDownLeft(95, 95, false, false, false);
        clickNestedExpectFalseFalseTrue(childChildDiv, childDiv, div, hidEvent::MouseDownLeft);

        mainWin->onMouseUpLeft();
        clickNestedExpectFalseFalseTrue(childChildDiv, childDiv, div, hidEvent::MouseUpLeft);

        mainWin->onMouseDownRight(95, 95, false, false, false);
        clickNestedExpectFalseFalseTrue(childChildDiv, childDiv, div, hidEvent::MouseDownRight);

        mainWin->onMouseUpRight();
        clickNestedExpectFalseFalseTrue(childChildDiv, childDiv, div, hidEvent::MouseUpRight);

        childChildDiv->resetClickFlags();
        childDiv->resetClickFlags();

        div->resetClickFlags();
        div->excludeFromObjMap(true);

        mainWin->onMouseDownLeft(95, 95, false, false, false);
        clickNestedExpectAllFalse(childChildDiv, childDiv, div, hidEvent::MouseDownLeft);

        mainWin->onMouseUpLeft();
        clickNestedExpectAllFalse(childChildDiv, childDiv, div, hidEvent::MouseUpLeft);

        mainWin->onMouseDownRight(95, 95, false, false, false);
        clickNestedExpectAllFalse(childChildDiv, childDiv, div, hidEvent::MouseDownRight);

        mainWin->onMouseUpRight();
        clickNestedExpectAllFalse(childChildDiv, childDiv, div, hidEvent::MouseUpRight);
    }, 600, 400);
}

TEST(UITest, HidDivOverlap) {
    HidNode* div = nullptr;
    HidNode* div2 = nullptr;

    appBody([&](const UIApplication& app) {
        div = addDiv(app);

        div2 = &app.getRootNode()->push<HidNode>();
        div2->setName("Div2");
        div2->setPos(50,50);
        div2->setSize(200,100);
        div2->setBackgroundColor(1.f, 0.f, 0.f, 1.f);
    }, [&](const UIApplication& app) {
        const auto mainWin = app.getMainWindow();

        mainWin->onMouseDownLeft(95, 95, false, false, false);
        EXPECT_FALSE(div->m_clicked[hidEvent::MouseDownLeft]);
        EXPECT_TRUE(div2->m_clicked[hidEvent::MouseDownLeft]);

        mainWin->onMouseUpLeft();
        EXPECT_FALSE(div->m_clicked[hidEvent::MouseUpLeft]);
        EXPECT_TRUE(div2->m_clicked[hidEvent::MouseUpLeft]);

        mainWin->onMouseDownRight(95, 95, false, false, false);
        EXPECT_FALSE(div->m_clicked[hidEvent::MouseDownRight]);
        EXPECT_TRUE(div2->m_clicked[hidEvent::MouseDownRight]);

        mainWin->onMouseUpRight();
        EXPECT_FALSE(div->m_clicked[hidEvent::MouseUpRight]);
        EXPECT_TRUE(div2->m_clicked[hidEvent::MouseUpRight]);
    });
}

TEST(UITest, HidDivInvisibleOverlap) {
    HidNode* div = nullptr;
    HidNode* div2 = nullptr;

    appBody([&](const UIApplication& app) {
        div = addDiv(app);

        div2 = &app.getRootNode()->push<HidNode>();
        div2->setName("Div2");
        div2->setPos(50,50);
        div2->setSize(200,100);
        div2->setBackgroundColor(0.f, 1.f, 0.f, 1.f);
        div2->setVisibility(false);

    }, [&](const UIApplication& app) {
        const auto mainWin = app.getMainWindow();

        mainWin->onMouseDownLeft(95, 95, false, false, false);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseDownLeft]);
        EXPECT_FALSE(div2->m_clicked[hidEvent::MouseDownLeft]);

        mainWin->onMouseUpLeft();
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseUpLeft]);
        EXPECT_FALSE(div2->m_clicked[hidEvent::MouseUpLeft]);

        mainWin->onMouseDownRight(95, 95, false, false, false);
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseDownRight]);
        EXPECT_FALSE(div2->m_clicked[hidEvent::MouseDownRight]);

        mainWin->onMouseUpRight();
        EXPECT_TRUE(div->m_clicked[hidEvent::MouseUpRight]);
        EXPECT_FALSE(div2->m_clicked[hidEvent::MouseUpRight]);
    });
}

TEST(UITest, HidDivMouseInBasicTest) {
    const HidNode* div = nullptr;

    appBody([&](const UIApplication& app) {
        div = addDiv(app);
        const auto mainWin = app.getMainWindow();
        mainWin->onMouseMove(0, 0, 0);

        iterate(app);

        mainWin->onMouseMove(60, 60, 0);
    }, [&](const UIApplication& app) {
        EXPECT_EQ(div->m_mouseIn, 1);
    });
}

TEST(UITest, HidDivMouseInOutTest) {
    const HidNode* div = nullptr;

    appBody([&](const UIApplication& app) {
        div = addDiv(app);
        const auto mainWin = app.getMainWindow();
        mainWin->onMouseMove(0, 0, 0);
        iterate(app);
        mainWin->onMouseMove(60, 60, 0);
        iterate(app);
        mainWin->onMouseMove(0, 0, 0);
    }, [&](const UIApplication& app) {
        EXPECT_EQ(div->m_mouseIn, 1);
        EXPECT_EQ(div->m_mouseOut, 1);
    });
}

TEST(UITest, HidNestedDivMouseInTest) {
    HidNode* div = nullptr;
    HidNode* childDiv = nullptr;

    appBody([&](const UIApplication& app) {
        div = addDiv(app);
        div->setName("div1");

        childDiv = &div->push<HidNode>({
            .pos = glm::ivec2{20,20},
            .size = glm::ivec2{150,50},
            .bgColor = glm::vec4{0.f, 0.f, 1.f, 1.f}
        });
        childDiv->setName("childDiv");

        const auto mainWin = app.getMainWindow();
        mainWin->onMouseMove(0, 0, 0);
        iterate(app);
        mainWin->onMouseMove(60, 60, 0);
        iterate(app);
        mainWin->onMouseMove(85, 85, 0);
        iterate(app);
        mainWin->onMouseMove(60, 60, 0);
        iterate(app);
        mainWin->onMouseMove(0, 0, 0);
    }, [&](const UIApplication& app) {
        EXPECT_EQ(div->m_mouseIn, 1);
        EXPECT_EQ(childDiv->m_mouseIn, 1);
        EXPECT_EQ(div->m_mouseOut, 1);
        EXPECT_EQ(childDiv->m_mouseOut, 1);
    });
}

}
