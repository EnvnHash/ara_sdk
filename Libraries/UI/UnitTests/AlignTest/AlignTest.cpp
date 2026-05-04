//
// Created by sven on 11/15/20.
//

#include "../TestCommon.h"
#include <UIElements/Div.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::AlignTest {

void drawQuadAndCheck(const align ax, const valign ay) {
    constexpr ivec2 size = { 200, 100 };
    constexpr vec4 col = { 1.f, 0.f, 0.f, 1.f };
    appBody([&](UIApplication& app){
        auto& div = app.getMainWindow()->getRootNode()->push<Div>();
        div.setSize(size.x, size.y);
        div.setBackgroundColor(col);
        div.setAlign(ax, ay);
    },
    [&](UIApplication& app){
        const auto mainWin = app.getWinBase()->getWinHandle();
        checkQuad(mainWin,
          { ax == align::left ? 0 : ax == align::center ? (mainWin->getWidth()/2 - size.x/2) : mainWin->getWidth() -size.x,
            ay == valign::top ? 0 : ay == valign::center ? (mainWin->getHeight()/2 - size.y/2) : mainWin->getHeight() -size.y },
          size,
          col,
          {});
    }, 800, 600);
}

TEST(UITest, AlignLeftBottom) {
    drawQuadAndCheck(align::left, valign::bottom);
}

TEST(UITest, AlignRightBottom) {
    drawQuadAndCheck(align::right, valign::bottom);
}

TEST(UITest, AlignLeftTop) {
    drawQuadAndCheck(align::left, valign::top);
}

TEST(UITest, AlignRightTop) {
    drawQuadAndCheck(align::right, valign::top);
}

TEST(UITest, AlignCenter) {
    drawQuadAndCheck(align::center, valign::center);
}

TEST(UITest, BorderRadiusOutOfBoundsLimit) {
    constexpr ivec2 size = { 200, 200 };
    constexpr vec4 col = { 1.f, 0.f, 0.f, 1.f };
    appBody([&](const UIApplication& app){
        const auto win = app.getMainWindow();
        auto& div = win->getRootNode()->push<Div>();
        div.setPos(-100, 100);
        div.setSize(size.x, size.y);
        div.setBackgroundColor(col);
        div.setAlign(align::left, valign::bottom);
        div.setBorderRadius(40);
        div.setBorderWidth(20);
        div.setBorderColor(0.f, 0.5f, 1.f, 1.f);
    }, [&](const UIApplication& app){
        const auto mainWin = app.getWinBase();
        compareFrameBufferToImage(filesystem::current_path() / "border_radius_oob.png",
                                  mainWin->getWidth(),
                                  mainWin->getHeight(),
                                  1);
    }, 200, 200);
}

}