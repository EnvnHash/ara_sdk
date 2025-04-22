#include "GLBaseUnitTestCommon.h"
#include <GeoPrimitives/Quad.h>

using namespace std;

namespace ara::GLBaseUnitTest::DrawQuad {
    ShaderCollector shCol;  // create a ShaderCollector
    GLFWWindow gwin;
    unique_ptr<Quad> quad;
    Shaders *colShader;
    glWinPar gp;             // define Parameters for windows instanciating

    bool didRun = false;

    void staticDrawFunc() {
        // set some OpenGL parameters
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);                    // clear the screen
        glViewport(0, 0, (GLsizei) gp.width, (GLsizei) gp.height);        // set the drawable arean

        colShader->begin();                        // bind the shader
        colShader->setIdentMatrix4fv("m_pvm");  // set the model-view-projection matrix to an indent matrix
        quad->draw();                            // draw the quad, the quad will be white which is the standard color of a Quad

        ASSERT_EQ(postGLError(),
                  GL_NO_ERROR);    // be sure that there are no GL errors, just for testing, normally not needed
        gwin.swap();                            // execute the opengl command queue by swapping the framebuffers, runLoop() does this automatically, but we want to quit immediately so do the swap manually here

        // that's all to draw a quad, now we are reading back the framebuffer and check that everything was drawn correctly

        // read back
        GLubyte data[4];    // make some space to download
        glReadBuffer(GL_FRONT);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);    // synchronous, blocking command, no swap() needed

        // check that it's really red
        ASSERT_EQ(postGLError(), GL_NO_ERROR);
        ASSERT_EQ(data[0], 255);
        ASSERT_EQ(data[1], 0);
        ASSERT_EQ(data[2], 0);
        ASSERT_EQ(data[3], 255);

        didRun = true;
        gwin.destroy();
    }

    TEST(GLBaseTest, DrawQuad) {
        // direct window creation
        // gp.debug = true;
        gp.width = 1920;            // set the windows width
        gp.height = 1080;            // set the windows height
        gp.doInit = true;            // GWindow needs GLFW library to be m_inited. GWindow can do this itself. Standard is true
        gp.shiftX = 100;            // x offset relative to OS screen canvas
        gp.shiftY = 100;            // y offset relative to OS screen canvas
        gp.scaleToMonitor = false;  // maintain pixels to canvas 1:1 if set to true, on windows scaling according to the monitor system scaling accours
        gp.createHidden = false;  // maintain pixels to canvas 1:1 if set to true, on windows scaling according to the monitor system scaling accours

        ASSERT_TRUE(gwin.create(gp));    // now pass the arguments and create the window

        // init glew
        initGLEW();

        quad = make_unique<Quad>(-1.f, -1.f, 2.f, 2.f,
                                 glm::vec3(0.f, 0.f, 1.f),
                                 1.f, 0.f, 0.f,
                                 1.f);  // create a Quad, standard width and height (normalized into -1|1), static red
        colShader = shCol.getStdCol(); // get a simple standard color shader

        // start a draw loop
        staticDrawFunc();

        EXPECT_TRUE(didRun);
    }
}

