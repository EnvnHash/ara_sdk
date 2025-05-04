//
// Created by sven on 11/15/20.
//
#include "test_common.h"

#include "UIApplication.h"
#include "UITable.h"
#include <ScrollView.h>
#include <Div.h>
#include <Label.h>


using namespace glm;
using namespace std;

namespace ara::SceneGraphUnitTest::ScrollViewTest{

UITable* addTable(UINode* rootNode) {
    auto taux = rootNode->addChild<UITable>();
    taux->setAlignY(valign::bottom);
    taux->t_setSpacing(8, 8);
    taux->t_setMargins(0, 0);
    taux->setColor(.2f, 0.2f, 0.2f, 1.f);
    taux->setBackgroundColor(.0f, .0f, .0f, 1.0f);

    taux->insertRow(-1,1,40,false,true);						// fixed row
    taux->insertRow(-1,1,0,false,false);
    taux->insertRow(-1,1,40,false,false);

    taux->insertColumn(-1,1,100,false,false,50,150);			// column with size limits [50..150]
    taux->insertColumn(-1,1,0);
    taux->insertColumn(-1,1,50);

    return taux;
}

UITable* addNestedTable(UINode* node) {
    auto nt = dynamic_cast<UITable *>(node->addChild(make_unique<UITable>(vec2{}, vec2{600.f, 200.f}, vec2{})));
    nt->t_setSpacing(8, 8);
    nt->t_setMargins(2, 2);
    nt->setColor(.2f, .2f, .2f, 1.f);
    nt->setBackgroundColor(.1f, .1f, .2f, 1.0f);

    nt->insertColumn(-1,1,80,false,false,50,150);			// column with size limits [50..150]
    nt->insertColumn(-1,1,300,false,false);
    nt->insertColumn(-1,1,25);

    return nt;
}

void addLabels(UITable* nt) {
    int i;
    glm::vec4 color_bg(.1f,.2f,.3f,1.f);
    glm::vec4 color_text(1.f);

    for (i = 0; i < 20; i++) {
        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << i;

        nt->insertRow(-1, 1, 100, false, false);						// fixed row
        auto l = nt->setCell(i, 0, make_unique<Label>() );
        l->setFont("regular", 22,  align::center, valign::center, color_text);
        l->setBackgroundColor(color_bg);
        l->setText(ss.str());
    }
}

ScrollView* addScrollView(UINode* rootNode, int nrSubElements ) {
    auto scrollView = rootNode->addChild<ScrollView>();
    scrollView->setAlign(align::center, valign::center);
    scrollView->setSize(0.7f, 0.7f);
    scrollView->setBackgroundColor(0.f, 0.f, 0.5f, 1.f);

    vec4 bgColor = {0.7f, 0.7f, 0.7f, 1.f};
    int chHeight = 40;

    for (int i = 0; i < nrSubElements; i++) {
        auto ch = scrollView->addChild<Div>();
        ch->setPos(10, (chHeight +10) * i );
        ch->setSize(30, chHeight);
        ch->setBackgroundColor(bgColor);
    }

    return scrollView;
}

TEST(UITest, ScrollViewTestNoScrollbar) {
    appBody([&](UIApplication* app){
        auto rootNode = app->getMainWindow()->getRootNode();
        addScrollView(rootNode, 5);
    }, [&](UIApplication* app){
         compareFrameBufferToImage(filesystem::current_path() / "scrollview_test_ref.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 600, 400);
}

TEST(UITest, ScrollViewTestScrollbarVisible) {
    appBody([&](UIApplication* app){
        auto rootNode = app->getMainWindow()->getRootNode();
        addScrollView(rootNode, 10);
    }, [&](UIApplication* app){
        compareFrameBufferToImage(filesystem::current_path() / "scrollview_test_ref2.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 600, 400);
}

TEST(UITest, ScrollViewTestScrollBarMoved) {
    appBody([&](UIApplication* app){
        auto mainWin = app->getMainWindow();
        auto rootNode = mainWin->getRootNode();
        auto scrollView = addScrollView(rootNode, 10);

        app->getWinBase()->draw(0, 0, 0);
        app->getMainWindow()->swap();

        // simulate dragging
        mainWin->onMouseDownLeft(500, 150, false, false, false);
        mainWin->onMouseMove(500, 152, 0);
        mainWin->onMouseMove(500, 200, 0);
        mainWin->onMouseUpLeft();

    }, [&](UIApplication* app){
        compareFrameBufferToImage(filesystem::current_path() / "scrollview_test_ref3.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 600, 400);
}

TEST(UITest, ScrollViewIntable) {
    appBody([&](UIApplication* app){
        auto mainWin = app->getMainWindow();
        auto rootNode = mainWin->getRootNode();
        auto taux = addTable(rootNode);

        auto ui_SV =  taux->setCell<ScrollView>(1, 1);
        ui_SV->setPos(0, 0);
        ui_SV->setBackgroundColor(.1f, .1f, .1f, 1.f);

        auto nt = addNestedTable(ui_SV);
        nt->setDynamicWidth(true);
        nt->setDynamicHeight(true);

        addLabels(nt);

    }, [&](UIApplication* app){
        auto mainWin = app->getMainWindow();
        compareFrameBufferToImage(filesystem::current_path() / "scrollview_in_table.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());

        // simulate dragging
        mainWin->onMouseDownLeft(1215, 120, false, false, false);
        mainWin->onMouseMove(1215, 134, 0);
        mainWin->onMouseMove(1215, 473, 0);
        mainWin->onMouseUpLeft();

        app->getWinBase()->draw(0, 0, 0);
        app->getMainWindow()->swap();

        compareFrameBufferToImage(filesystem::current_path() / "scrollview_in_table_moved.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 1280, 720);
}

}