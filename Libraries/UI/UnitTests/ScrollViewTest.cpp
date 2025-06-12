//
// Created by sven on 11/15/20.
//
#include "TestCommon.h"

#include "UIApplication.h"
#include "UIElements/UITable.h"
#include <UIElements/ScrollView.h>
#include <UIElements/Div.h>
#include <UIElements/Label.h>

using namespace glm;
using namespace std;

namespace ara::SceneGraphUnitTest::ScrollViewTest{

UITable* addTable(UINode* rootNode) {
    auto taux = rootNode->addChild<UITable>(UITableParameters{
        .fgColor = vec4{.2f, 0.2f, 0.2f, 1.f},
        .bgColor = vec4{.0f, .0f, .0f, 1.f},
        .alignY = valign::bottom,
        .margin = ivec2{0, 0},
        .spacing = ivec2{8, 8},
    });

    taux->insertRow(-1,1,40,false,true);						// fixed row
    taux->insertRow(-1,1,0,false,false);
    taux->insertRow(-1,1,40,false,false);

    taux->insertColumn(-1,1,100,false,false,50,150);			// column with size limits [50..150]
    taux->insertColumn(-1,1,0);
    taux->insertColumn(-1,1,50);

    return taux;
}

UITable* addNestedTable(ScrollView* scrollView) {
    auto nt = scrollView->addChild<UITable>(UITableParameters{
        .size = vec2{600.f, 200.f},
        .fgColor = vec4{.2f, .2f, .2f, 1.f},
        .bgColor = vec4{.1f, .1f, .2f, 1.0f},
        .margin = ivec2{2,2},
        .spacing = ivec2{8,8},
    });

    nt->insertColumn(-1,1,80,false,false,50,150);			// column with size limits [50..150]
    nt->insertColumn(-1,1,300,false,false);
    nt->insertColumn(-1,1,25);

    return nt;
}

void addLabels(UITable* nt) {
    int i;
    vec4 color_bg(.1f,.2f,.3f,1.f);
    vec4 color_text(1.f);

    for (i = 0; i < 20; i++) {
        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << i;

        nt->insertRow(-1, 1, 100, false, false);						// fixed row
        auto l = nt->setCell<Label>(i, 0);
        l->setFont("regular", 22,  align::center, valign::center, color_text);
        l->setBackgroundColor(color_bg);
        l->setText(ss.str());
    }
}

ScrollView* addScrollView(UINode* rootNode, int nrSubElements ) {
    auto scrollView = rootNode->addChild<ScrollView>(UINodePars{
        .size = vec2{0.7f, 0.7f},
        .bgColor = vec4{0.f, 0.f, 0.5f, 1.f},
        .alignX = align::center,
        .alignY = valign::center,
    });

    int chHeight = 40;
    for (int i = 0; i < nrSubElements; i++) {
        scrollView->addChild<Div>(UINodePars{
            .pos = ivec2{10, (chHeight +10) * i },
            .size = ivec2{30, chHeight},
            .bgColor = vec4{0.7f, 0.7f, 0.7f, 1.f}
        });
    }

    return scrollView;
}

TEST(UITest, ScrollViewTestNoScrollbar) {
    appBody([&](UIApplication& app){
        auto rootNode = app.getMainWindow()->getRootNode();
        addScrollView(rootNode, 5);
    }, [&](UIApplication& app){
         compareFrameBufferToImage(filesystem::current_path() / "scrollview_test_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, ScrollViewTestScrollbarVisible) {
    appBody([&](UIApplication& app){
        auto rootNode = app.getMainWindow()->getRootNode();
        addScrollView(rootNode, 10);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "scrollview_test_ref2.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, ScrollViewTestScrollBarMoved) {
    appBody([&](UIApplication& app){
        auto mainWin = app.getMainWindow();
        auto rootNode = mainWin->getRootNode();
        auto scrollView = addScrollView(rootNode, 10);

        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();

        // simulate dragging
        mainWin->onMouseDownLeft(500, 150, false, false, false);
        mainWin->onMouseMove(500, 152, 0);
        mainWin->onMouseMove(500, 200, 0);
        mainWin->onMouseUpLeft();

    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "scrollview_test_ref3.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, ScrollViewIntable) {
    appBody([&](UIApplication& app){
        auto mainWin = app.getMainWindow();
        auto rootNode = mainWin->getRootNode();
        auto taux = addTable(rootNode);

        auto ui_SV =  taux->setCell<ScrollView>(1, 1);
        ui_SV->setBackgroundColor(.1f, .1f, .1f, 1.f);

        auto nt = addNestedTable(ui_SV);
        nt->setDynamicWidth(true);
        nt->setDynamicHeight(true);

        addLabels(nt);

    }, [&](UIApplication& app){
        auto mainWin = app.getMainWindow();
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

}