//
// Created by sven on 11/15/20.
//

#include "test_common.h"

#include "UIApplication.h"
#include <UIElements/Div.h>

using namespace std;

namespace ara::SceneGraphUnitTest::SetXSetY{

    void checkRed(size_t ptr, std::vector<GLubyte>& data)
    {
        ASSERT_EQ(data[ptr], 255);
        ASSERT_EQ(data[ptr +1], 0);
        ASSERT_EQ(data[ptr +2], 0);
        ASSERT_EQ(data[ptr +3], 255);
    }

    void checkBlack(size_t ptr, std::vector<GLubyte>& data)
    {
        ASSERT_EQ(data[ptr], 0);
        ASSERT_EQ(data[ptr +1], 0);
        ASSERT_EQ(data[ptr +2], 0);
        ASSERT_EQ(data[ptr +3], 0);
    }

    size_t getPtr(size_t xPos, size_t yPos, GLFWWindow* win)
    {
        return (win->getWidthReal() * yPos + xPos ) * 4;
    }

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
            div->setBackgroundColor(1.f, 0.f, 0.f, 1.f);

            EXPECT_EQ( postGLError(), GL_NO_ERROR); // be sure that there is no opengl error

            // ----------------------------------------------------------------------------------

            // manually draw - it would be done automatically after this function exits
            // but since we want to do our checks, and keep things simple - that is doing everything
            // right here - we do it explicitly
            app.getWinBase()->draw(0, 0, 0);
            auto mainWin = app.getMainWindow()->getWinHandle();
            mainWin->swap();

            // read back from the framebuffer
            vector<GLubyte> data((int)mainWin->getWidthReal() * (int)mainWin->getHeightReal() * 4);	// make some space to download
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            // read the whole framebuffer
            glReadPixels(0, 0, (int)mainWin->getWidthReal(), (int)mainWin->getHeightReal(), GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);	// synchronous, blocking command, no swap() needed
            ASSERT_EQ(postGLError(), GL_NO_ERROR);

            // check if left lower corner is red
            size_t leftLower = getPtr(posX, mainWin->getHeight() - posY - height, mainWin);
            checkRed(leftLower, data);

            // move one pixel left and check that it's black
            leftLower -= 4;
            checkBlack(leftLower, data);

            // move one pixel down and check that it's black
            leftLower -= mainWin->getWidth() * 4;
            checkBlack(leftLower, data);


            // check if right top corner is red
            size_t rightTop = getPtr(posX + width -1, mainWin->getHeight() -posY -1, mainWin);
            checkRed(rightTop, data);

            // move one pixel right and check that it's black
            rightTop += 4;
            checkBlack(rightTop, data);

            // move one pixel up and check that it's black
            rightTop += (app.getMainWindow()->getWidth() - 1) * 4;
            checkBlack(rightTop, data);
        });

        //app.startEventLoop(); // for debugging comment in this line in order to have to window stay

        // the above method will return when the draw function was executed once
        // exit immediately!
        app.exit();
    }
}