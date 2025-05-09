//
// Created by sven on 11/15/20.
//

#include "../test_common.h"

#include "UIApplication.h"
#include <UIElements/Button/Button.h>


using namespace std;

namespace ara::SceneGraphUnitTest::GetPixelPosition_Absolute {

TEST(UITest, GetPixelPosition_Absolute) {
    UIApplication app;
    app.setEnableMenuBar(false); app.setEnableWindowResizeHandles(false);

    glm::vec4 fgColor(0.f, 0.f, 1.f, 1.f);
    glm::vec4 bgColor(0.2f, 0.2f, 0.2f, 1.f);

    // implicitly creates a window and runs the lambda before calling the draw function for the first time
    app.init([&](){
        // get the root node of the UI scenegraph
        auto rootNode = app.getRootNode();

        // create a Div, standard alignment is top, left (align::left, valign::top, with Pivot: pivotX::left, PX_TOP
        glm::ivec2 div0_pos(20, 20);
        glm::ivec2 div0_size(600, 500);
        auto div0 = rootNode->addChild<Div>(div0_pos.x, div0_pos.y, div0_size.x, div0_size.y, &bgColor[0], nullptr);

        // create a Div inside the first div (same left / top alignment)
        glm::ivec2 div1_pos(25, 25);
        auto div1 = div0->addChild<Div>(div1_pos.x, div1_pos.y, 100, 100, &fgColor[0], nullptr);

        // create a Div inside the first div (same left / top alignment) position in RELATIVE coords
        glm::vec2 div2_pos(0.3f, 0.2f);
        auto div2 = div0->addChild<Div>(div2_pos.x, div2_pos.y, 0.2f, 0.2f);
        div2->setBackgroundColor(fgColor);

        // by calling draw, the UINode tree gets iterated and all matrices are calculated
        // alternatively there is a function to explicitly iterate the node tree and ONLY calculate matrices
        rootNode->updateMatrix();

        // ----------------------------------------------------------------------------------

        // read back the x and y position in relation to its parent.
        // if alignment von align::left, valign::top, this is in relation to the window top left corner
        ASSERT_EQ(div0->getWinPos().x, div0_pos.x);
        ASSERT_EQ(div0->getWinPos().y, div0_pos.y);

        ASSERT_EQ(div1->getWinPos().x, div0_pos.x + div1_pos.x);
        ASSERT_EQ(div1->getWinPos().y, div0_pos.y + div1_pos.y);

        ASSERT_EQ(div2->getWinPos().x, std::round(static_cast<float>(div0_size.x) * div2_pos.x + div0_pos.x));
        ASSERT_EQ(div2->getWinPos().y, std::round(static_cast<float>(div0_size.y) * div2_pos.y + div0_pos.y));

    });

    // the above method will return when the draw function was executed once exit immediately!
    app.exit();
}

}