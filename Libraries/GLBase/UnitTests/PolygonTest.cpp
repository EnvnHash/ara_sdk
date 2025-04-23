#include "GLBaseUnitTestCommon.h"
#include <GeoPrimitives/Polygon.h>

using namespace std;

namespace ara::GLBaseUnitTest::PolygonTest {
    GLFWWindow gwin;           // create an instance, this will do nothing
    glWinPar gp;             // define Parameters for windows instantiating
    ShaderCollector shCol;  // create a ShaderCollector
    Shaders *colShader;
    std::unique_ptr<Polygon> poly;

    bool didRun = false;

    bool staticDrawFunc(double, double, int) {
        initGLEW();

        colShader = shCol.getStdCol(); // get a simple standard color shader
        poly = make_unique<Polygon>(&shCol);
        poly->setColor(1.f, 0.f, 0.f, 1.f);

        // create a test polygon
        poly->addPoint(0, -0.7f, -0.7f);
        poly->addPoint(0, 0.7f, -0.8f);
        poly->addPoint(0, 0.f, 0.7f);

        poly->addHole();
        poly->addPoint(1, -0.1f, -0.1f);
        poly->addPoint(1, 0.1f, -0.1f);
        poly->addPoint(1, 0.f, 0.1f);

        poly->tesselate();

        EXPECT_EQ(postGLError(), GL_NO_ERROR);

        // set some OpenGL parameters
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);                    // clear the screen
        glViewport(0, 0, (GLsizei) gp.width, (GLsizei) gp.height);        // set the drawable arean

        EXPECT_EQ(postGLError(), GL_NO_ERROR);

        colShader->begin();                        // bind the shader
        colShader->setIdentMatrix4fv("m_pvm");  // set the model-view-projection matrix to an indent matrix
        poly->draw();
        poly->drawOutline();

        EXPECT_EQ(postGLError(),
                  GL_NO_ERROR);    // be sure that there are no GL errors, just for testing, normally not needed
        gwin.swap();                            // execute the opengl command queue by swapping the framebuffers, runLoop() does this automatically, but we want to quit immediately so do the swap manually here

        // read back the framebuffer and check that everything was drawn correctly

        // read back
        GLubyte data[4];    // make some space to download
        glReadBuffer(GL_FRONT);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        int ypos = static_cast<int>(static_cast<float>(gwin.getHeight()) * (0.65f / 2.f + 0.5f));
        glReadPixels(static_cast<int>(static_cast<float>(gwin.getWidth()) * 0.5f), ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                     data);    // synchronous, blocking command, no swap() needed

        // check that it's really red
        EXPECT_EQ(postGLError(), GL_NO_ERROR);
        EXPECT_EQ(data[0], 255);
        EXPECT_EQ(data[1], 0);
        EXPECT_EQ(data[2], 0);
        EXPECT_EQ(data[3], 255);

        ypos = static_cast<int>(static_cast<float>(gwin.getHeight()) * 0.5f);
        glReadPixels(static_cast<int>(static_cast<float>(gwin.getWidth()) * 0.5f), ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE,
                     data);    // synchronous, blocking command, no swap() needed

        // check that it's really red
        EXPECT_EQ(postGLError(), GL_NO_ERROR);
        EXPECT_EQ(data[0], 0);
        EXPECT_EQ(data[1], 0);
        EXPECT_EQ(data[2], 0);
        EXPECT_EQ(data[3], 255);

        // check the hole in the middle
        didRun = true;
        gwin.close();
        return false;
    }

    TEST(GLBaseTest, PolygonTest) {
        // direct window creation
        // gp.debug = true;
        gp.width = 1024;            // set the windows width
        gp.height = 768;            // set the windows height
        gp.shiftX = 100;            // x offset relative to OS screen canvas
        gp.shiftY = 100;            // y offset relative to OS screen canvas
        gp.scaleToMonitor = false;  // maintain pixels to canvas 1:1 if set to true, on windows scaling according to the monitor system scaling accours
        gp.transparent = false;

        ASSERT_TRUE(gwin.init(gp));    // now pass the arguments and create the window

        // start a draw loop
        gwin.runLoop(std::bind(staticDrawFunc, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        EXPECT_TRUE(didRun);
    }
}

