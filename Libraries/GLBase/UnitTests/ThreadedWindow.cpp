#include <GLBaseUnitTestCommon.h>
#include <WindowManagement/GLFWWindow.h>
#include <Shaders/ShaderCollector.h>
#include <GeoPrimitives/Quad.h>

using namespace std;

namespace ara::GLBaseUnitTest::ThreadedWindow {
    TEST(GLBaseTest, ThreadedWindow) {
        // init glfw
        ASSERT_TRUE(glfwInit());

        int nrThreads = 4;
        vector<thread> threads;
        auto windows = vector<GLFWWindow>(nrThreads);
        createThreads(nrThreads, windows);

        // make no context current
        glfwMakeContextCurrent(nullptr);

        // create drawing threads for each windows
        for (int i = 0; i < nrThreads; i++)
            threads.emplace_back([&windows, i]() {

                windows[i].makeCurrent();

                ShaderCollector shCol;  // create a ShaderCollector
                unique_ptr<Quad> quad;
                Shaders *colShader;

                quad = make_unique<Quad>(QuadInitParams{.color = { 1.f, 0.f, 0.f, 1.f} });  // create a Quad, standard width and height (normalized into -1|1), static red
                colShader = shCol.getStdCol(); // get a simple standard color shader

                // set some OpenGL parameters
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);                    // clear the screen
                glViewport(0, 0, (GLsizei) windows[i].getWidth(), (GLsizei) windows[i].getHeight());    // set the drawable arean

                colShader->begin();                     // bind the shader
                colShader->setIdentMatrix4fv("m_pvm");  // set the model-view-projection matrix to an indent matrix
                quad->draw();                           // draw the quad, the quad will be white which is the standard color of a Quad

                ASSERT_EQ(postGLError(), GL_NO_ERROR);    // be sure that there are no GL errors, just for testing, normally not needed
                windows[i].swap();                        // execute the opengl command queue by swapping the framebuffers, runLoop() does this automatically, but we want to quit immediately so do the swap manually here

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

            });

        for (int i = 0; i < nrThreads; i++) {
            threads[i].join();
            windows[i].destroy(false);
        }

        glfwTerminate();
    }
}

