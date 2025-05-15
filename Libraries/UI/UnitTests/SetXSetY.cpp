//
// Created by sven on 11/15/20.
//

#include "TestCommon.h"

#include "UIApplication.h"
#include <UIElements/Div.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::SetXSetY{

TEST(UITest, SetXSetY)
{
    UIApplication app;
    app.setEnableMenuBar(false);
    app.setEnableWindowResizeHandles(false);
    app.setMultisample(false);

    // implicitly creates a window and runs the lambda before calling the draw function for the first time
    app.init([&](){
        // get the root node of the UI scenegraph
        auto rootNode = app.getRootNode();

        // create a Div (simplest Element)
        auto div = rootNode->addChild<Div>();
        div->setName("test");

        int width = 200, height = 100;
        int posX = 120, posY = 310;
        div->setPos(posX, posY);
        div->setSize(width, height);

        vec4 col = { 1.f, 0.f, 0.f, 1.f };
        div->setBackgroundColor(col);

        EXPECT_EQ( postGLError(), GL_NO_ERROR); // be sure that there is no opengl error

        // ----------------------------------------------------------------------------------

        // manually draw - it would be done automatically after this function exits
        // but since we want to do our checks, and keep things simple - that is doing everything
        // right here - we do it explicitly
        app.getWinBase()->draw(0, 0, 0);
        auto mainWin = app.getMainWindow()->getWinHandle();
        mainWin->swap();

        checkQuad(mainWin, { posX, posY }, { width, height }, col, {});
    });

    //app.startEventLoop(); // for debugging comment in this line in order to have to window stay

    // the above method will return when the draw function was executed once
    // exit immediately!
    app.exit();
}

}