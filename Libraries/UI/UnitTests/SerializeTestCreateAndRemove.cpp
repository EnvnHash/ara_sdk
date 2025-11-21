//
// Created by sven on 17-11-25.
////
// Created by sven on 11/15/20.
//

#include "TestCommon.h"
#include <UIElements/Div.h>
#include <UINodeFactory.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::SerializeTestCreateAndRemove {

void saveAndReloadQuad(UIApplication& app, const UINodePars& p) {
    auto root = app.getMainWindow()->getRootNode();
    auto& div = root->push<Div>(p);
    root->saveAs("test.json");
    root->remove(div);

    root->load(filesystem::path("test.json"));
}

void saveAndReloadQuadCasc(UIApplication& app) {
    auto root = app.getMainWindow()->getRootNode();
    auto& div = root->push<Div>(UINodePars{
        .pos = ivec2{0,0},
        .size = ivec2{400, 400},
        .bgColor = vec4{ 1.f, 0.f, 0.f, 1.f},
        .align = align::center,
        .valign = valign::center,
    });

    auto& div2 = div.push<Div>(UINodePars{
        .pos = ivec2{80,80},
        .size = ivec2{200, 200},
        .bgColor = vec4{ 0.2f, 0.3f, 0.f, 1.f},
        .align = align::left,
        .valign = valign::top,
    });

    div2.push<Div>(UINodePars{
    .pos = ivec2{0,0},
    .size = ivec2{60, 50},
    .bgColor = vec4{ 0.2f, 0.3f, 1.f, 1.f},
    .align = align::right,
    .valign = valign::bottom,
});
/*
    root->saveAs("test.json");
    root->remove(div);

    root->load(filesystem::path("test.json"));*/
}

void drawQuadAndCheck(const UINodePars& p) {
    appBody([&](UIApplication& app){
        saveAndReloadQuad(app, p);
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

void drawQuadAndCompare() {
    appBody([&](UIApplication& app){
        saveAndReloadQuadCasc(app);
    },
    [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "serialize_casc_div.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 800, 600);
}

TEST(UITest, SerializeDivCreate) {
    registerDefaultUITypes();
    drawQuadAndCheck(UINodePars{
        .pos = ivec2{0,0},
        .size = ivec2{200, 100},
        .bgColor = vec4{ 1.f, 0.f, 0.f, 1.f},
        .align = align::center,
        .valign = valign::center,
    });
}

TEST(UITest, SerializeDivCascadeCreate) {
    registerDefaultUITypes();
    drawQuadAndCompare();
}

}