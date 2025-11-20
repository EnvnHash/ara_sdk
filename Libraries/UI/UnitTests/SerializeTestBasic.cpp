//
// Created by sven on 17-11-25.
////
// Created by sven on 11/15/20.
//

#include "TestCommon.h"
#include <UIElements/Div.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::AlignTest {

void saveAndReload(UIApplication& app, const UINodePars& p) {
    auto div = app.getMainWindow()->getRootNode()->push<Div>(p);
    div->saveAs("test.json");
    app.getMainWindow()->getRootNode()->remove_child(div);

    auto div2 = app.getMainWindow()->getRootNode()->addChild<Div>();
    div2->load(filesystem::path("test.json"));
}

void drawQuadAndCheck(const UINodePars& p) {
    appBody([&](UIApplication& app){
        saveAndReload(app, p);
    },
    [&](UIApplication& app){
        auto mainWin = app.getWinBase()->getWinHandle();
        checkQuad(mainWin,
          { p.align.value() == align::left ? std::get<ivec2>(p.pos).x : p.align.value() == align::center ? (mainWin->getWidth()/2 - std::get<ivec2>(p.size).x/2) : mainWin->getWidth() -std::get<ivec2>(p.size).x,
            p.valign.value() == valign::top ? std::get<ivec2>(p.pos).y : p.valign.value() == valign::center ? (mainWin->getHeight()/2 - std::get<ivec2>(p.size).y/2) : mainWin->getHeight() -std::get<ivec2>(p.size).y },
          std::get<ivec2>(p.size),
          p.bgColor.value(),
          {});
    }, 800, 600);
}

void drawQuadAndCompare(const UINodePars& p) {
    appBody([&](UIApplication& app){
        saveAndReload(app, p);
    },
    [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "serialize_div.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 800, 600);
}

TEST(UITest, SerializeQuadCenter) {
    drawQuadAndCheck(UINodePars{
        .pos = ivec2{0,0},
        .size = ivec2{200, 100},
        .bgColor = vec4{ 1.f, 0.f, 0.f, 1.f},
        .align = align::center,
        .valign = valign::center,
    });
}

TEST(UITest, SerializeBottomRight) {
    drawQuadAndCheck(UINodePars{
        .pos = ivec2{0,0},
        .size = ivec2{200, 100},
        .bgColor = vec4{ 1.f, 1.f, 0.f, 1.f},
        .align = align::right,
        .valign = valign::bottom,
    });
}

TEST(UITest, SerializeLeftTopOffset) {
    drawQuadAndCheck(UINodePars{
        .pos = ivec2{20,10},
        .size = ivec2{180, 90},
        .bgColor = vec4{ 0.5f, 1.f, 0.f, 1.f},
        .align = align::left,
        .valign = valign::top,
    });
}

TEST(UITest, SerializeBorder) {
    drawQuadAndCompare(UINodePars{
        .pos = ivec2{20,10},
        .size = ivec2{180, 90},
        .bgColor = vec4{ 0.5f, 1.f, 0.f, 1.f},
        .align = align::center,
        .valign = valign::center,
        .borderWidth = 20,
        .borderRadius = 20,
        .borderColor = vec4{1.f, 0.f, 0.2, 1.f},
    });
}


}