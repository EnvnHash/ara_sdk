//
// Created by sven on 07-04-25.
//


#include "TestCommon.h"

#include "UIApplication.h"
#include <UIElements/TabView.h>
#include <UIElements/Div.h>


using namespace std;
using namespace glm;

namespace ara::UiUnitTest::TabViewTest {

TabView* addTabView(UIApplication& app, std::array<bool, 3>& checks) {
    auto rootNode = app.getMainWindow()->getRootNode();

    auto tabView = rootNode->addChild<TabView>(UINodePars{
        .size = vec2{1.f, 1.f},
        .fgColor = vec4{.3f, .3f, .3f, 1.f},
        .bgColor = vec4{.1f, .1f, .1f, 1.f}
    });

    tabView->setPadding(10.f);

    for (auto i=0; i<3; i++) {
        auto div = tabView->addTab<Div>("Entry "+std::to_string(i));
        div->setName("TabContent"+std::to_string(i));

        auto divChild = div->addChild<Div>();
        divChild->setSize(0.8f, 0.8f);
        divChild->setPos(0.1f, 0.1f);
        divChild->setBackgroundColor(static_cast<float>(i%3), static_cast<float>((i+1)%2), static_cast<float>((i+2)%3), 1.f);
        divChild->addMouseClickCb([&checks, i](hidData& data) {
            checks[i] = true;
        });
        divChild->setName("TabContentElement"+std::to_string(i));
    }

    tabView->setActivateTab(0);
    return tabView;
}

TEST(UITest, TabViewTest) {
    TabView* tv = nullptr;
    std::array<bool, 3> m_clicked{};
    appBody([&](UIApplication& app){
        tv = addTabView(app, m_clicked);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "tab_view_test_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);

        auto mainWin = app.getMainWindow();

        // click div on tab0
        mainWin->onMouseDownLeft(280, 210, false, false, false);
        mainWin->onMouseUpLeft();
        EXPECT_TRUE(m_clicked[0]);

        // click tab1
        mainWin->onMouseDownLeft(290, 30, false, false, false);
        mainWin->onMouseUpLeft();

        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();

        compareFrameBufferToImage(filesystem::current_path() / "tab_view_test_ref2.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);

        // click div on tab1
        mainWin->onMouseDownLeft(280, 210, false, false, false);
        mainWin->onMouseUpLeft();
        EXPECT_TRUE(m_clicked[1]);

        // click tab2
        mainWin->onMouseDownLeft(490, 30, false, false, false);
        mainWin->onMouseUpLeft();

        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();

        compareFrameBufferToImage(filesystem::current_path() / "tab_view_test_ref3.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);

        // click div on tab2
        mainWin->onMouseDownLeft(280, 210, false, false, false);
        mainWin->onMouseUpLeft();
        EXPECT_TRUE(m_clicked[2]);
    }, 600, 400);
}

}
