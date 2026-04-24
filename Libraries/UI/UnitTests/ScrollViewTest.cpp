//
// Created by sven on 11/15/20.
//
#include "TestCommon.h"

#include "UIApplication.h"
#include "UIElements/UITable.h"
#include <UIElements/ScrollView.h>
#include <UIElements/Div.h>
#include <UIElements/Text/Label.h>

using namespace glm;
using namespace std;

namespace ara::UiUnitTest::ScrollViewTest{

UITable* addTable(UINode* rootNode) {
    auto& taux = rootNode->push<UITable>(UITableParameters{
        .fgColor = vec4{.2f, 0.2f, 0.2f, 1.f},
        .bgColor = vec4{.0f, .0f, .0f, 1.f},
        .alignY = valign::bottom,
        .margin = ivec2{0, 0},
        .spacing = ivec2{8, 8},
    });

    taux.insertRow(-1,1,40,false,true);						// fixed row
    taux.insertRow(-1,1,0,false,false);
    taux.insertRow(-1,1,40,false,false);

    taux.insertColumn(-1,1,100,false,false,50,150);			// column with size limits [50.150]
    taux.insertColumn(-1,1,0);
    taux.insertColumn(-1,1,50);

    return &taux;
}

UITable* addNestedTable(ScrollView* scrollView) {
    auto& nt = scrollView->push<UITable>(UITableParameters{
        .size = vec2{600.f, 200.f},
        .fgColor = vec4{.2f, .2f, .2f, 1.f},
        .bgColor = vec4{.1f, .1f, .2f, 1.0f},
        .margin = ivec2{2,2},
        .spacing = ivec2{8,8},
    });

    nt.insertColumn(-1,1,80,false,false,50,150);			// column with size limits [50..150]
    nt.insertColumn(-1,1,300,false,false);
    nt.insertColumn(-1,1,25);

    return &nt;
}

void addLabels(UITable* nt) {
    constexpr vec4 color_bg(.1f,.2f,.3f,1.f);
    constexpr vec4 color_text(1.f);

    for (int i = 0; i < 20; i++) {
        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << i;

        nt->insertRow(-1, 1, 100, false, false);						// fixed row
        const auto l = nt->setCell<Label>(i, 0);
        l->setFont("regular", 22,  align::center, valign::center, color_text);
        l->setBackgroundColor(color_bg);
        l->setText(ss.str());
    }
}

ScrollView* addScrollView(UINode* rootNode, const int nrSubElements ) {
    auto& scrollView = rootNode->push<ScrollView>(UINodePars{
        .size = vec2{0.7f, 0.7f},
        .bgColor = vec4{0.f, 0.f, 0.5f, 1.f},
        .align = align::center,
        .valign = valign::center,
    });

    for (int i = 0; i < nrSubElements; i++) {
        constexpr int chHeight = 40;
        scrollView.push<Div>({
            .pos = ivec2{10, (chHeight +10) * i },
            .size = ivec2{30, chHeight},
            .bgColor = vec4{0.7f, 0.7f, 0.7f, 1.f}
        });
    }

    return &scrollView;
}

TEST(UITest, ScrollViewTestNoScrollbar) {
    appBody([&](const UIApplication& app){
        const auto rootNode = app.getMainWindow()->getRootNode();
        addScrollView(rootNode, 5);
    }, [&](const UIApplication& app){
         compareFrameBufferToImage(filesystem::current_path() / "scrollview_test_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, ScrollViewTestScrollbarVisible) {
    appBody([&](const UIApplication& app){
        const auto rootNode = app.getMainWindow()->getRootNode();
        addScrollView(rootNode, 10);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "scrollview_test_ref2.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, ScrollViewTestScrollBarMoved) {
    appBody([&](const UIApplication& app){
        const auto mainWin = app.getMainWindow();
        const auto rootNode = mainWin->getRootNode();
        addScrollView(rootNode, 10);

        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();

        // simulate dragging
        mainWin->onMouseDownLeft(500, 150, false, false, false);
        mainWin->onMouseMove(500, 160, 0);
        mainWin->onMouseMove(500, 200, 0);
        mainWin->onMouseUpLeft();

    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "scrollview_test_ref3.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, ScrollViewIntable) {
    appBody([&](const UIApplication& app){
        const auto mainWin = app.getMainWindow();
        const auto rootNode = mainWin->getRootNode();
        const auto taux = addTable(rootNode);

        const auto ui_SV =  taux->setCell<ScrollView>(1, 1);
        ui_SV->setBackgroundColor(.1f, .1f, .1f, 1.f);

        const auto nt = addNestedTable(ui_SV);
        nt->setDynamicWidth(true);
        nt->setDynamicHeight(true);

        addLabels(nt);

    }, [&](const UIApplication& app){
        const auto mainWin = app.getMainWindow();
        compareFrameBufferToImage(filesystem::current_path() / "scrollview_in_table.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);

        // simulate dragging
        mainWin->onMouseDownLeft(1215, 120, false, false, false);
        mainWin->onMouseMove(1215, 134, 0);
        mainWin->onMouseMove(1215, 473, 0);
        mainWin->onMouseUpLeft();

        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();

        compareFrameBufferToImage(filesystem::current_path() / "scrollview_in_table_moved.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 1280, 720);
}

TEST(UITest, ScrollViewInfiniteUIMatrItLoopCheck) {
    constexpr auto checkPadding = vec4{10.f, 10.f, 26.f, 26.f};
    ScrollView* scrollView = nullptr;
    appBody([&](const UIApplication& app){
        const auto rootNode = app.getRootNode();
        scrollView = &rootNode->push<ScrollView>(UINodePars{
            .size = vec2{0.9f, 0.9f},
            .bgColor = vec4{0.f, 0.f, 0.5f, 1.f},
            .align = align::center,
            .valign = valign::center,
            .padding = vec4{10.f, 10.f, 10.f, 10.f},
        });

        iterate(app);

        for (int i = 0; i < 4; i++) {
            EXPECT_EQ(scrollView->getPadding()[i], 10.f);
        }

        scrollView->push<Div>({
            .size = ivec2{500,500},
            .bgColor = vec4{1.f, 0.f, 0.f, 1.f},
        });

        iterate(app);

        for (int i = 0; i < 4; i++) {
            EXPECT_EQ(scrollView->getPadding()[i], checkPadding[i]);
        }
    }, [&](const UIApplication& app){
        // if feedback loop error, padding will be different on each iteration and reqRebuildCustomStyle will be set
        EXPECT_FALSE(scrollView->getReqRebuildCustomStyle());
        for (int i = 0; i < 4; i++) {
            EXPECT_EQ(scrollView->getPadding()[i], checkPadding[i]);
        }
    }, 400, 400);
}

}