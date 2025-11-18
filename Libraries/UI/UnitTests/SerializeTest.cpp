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

TEST(UITest, AlignLeftBottom) {
    Div* div = nullptr;
    Div* div2 = nullptr;

    ivec2 size = { 200, 100 };
    vec4 col = { 1.f, 0.f, 0.f, 1.f };

    appBody([&](UIApplication& app){
        div = app.getMainWindow()->getRootNode()->addChild<Div>();
        div->setSize(size.x, size.y);
        div->setBackgroundColor(col);
        div->setAlign(align::center, valign::center);
        div->saveAs("test.json");
        app.getMainWindow()->getRootNode()->remove_child(div);

        div2 = app.getMainWindow()->getRootNode()->addChild<Div>();
        div2->load(filesystem::path("test.json"));
        div2->setName("Horst");

        app.getMainWindow()->getRootNode()->dump();
    },
    [&](UIApplication& app){
        auto mainWin = app.getWinBase()->getWinHandle();
        checkQuad(mainWin,
            { (mainWin->getWidth()/2 - size.x/2), (mainWin->getHeight()/2 - size.y/2) },
            size,
            col,
            {});
    }, 800, 600);
}


}