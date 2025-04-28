#include "GLBaseUnitTestCommon.h"

#include <GeoPrimitives/Quad.h>
#include <Utils/Typo/TypoGlyphMap.h>
#include "Asset/AssetManager.h"

using namespace std;

namespace ara::GLBaseUnitTest::ContextSharing {

std::vector<GLubyte> readBack() {
    std::vector<GLubyte> data(4);    // make some space to download
    glReadBuffer(GL_FRONT);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data.data());    // synchronous, blocking command, no swap() needed
    return data;
}

std::function<void()> renderLambda(vector<GLFWWindow>& windows, GLBase& m_glBase, int i) {
    return [&windows, &m_glBase, i]() {
        windows[i].makeCurrent();

        // init glew for each context
        ASSERT_EQ(true, initGLEW());

        // since we are using the same resource, we need to acquire a lock to the GLBase
        unique_lock<mutex> lock(*m_glBase.glMtx());

        // unfortunately VAOs can't be shared since they are just a set of states, but don't contain actual data
        unique_ptr<Quad> quad = make_unique<Quad>(QuadInitParams{-1.f, -1.f, 2.f, 2.f,
                                                                 glm::vec3(0.f, 0.f, 1.f),
                                                                 1.f, 0.f, 0.f,
                                                                 1.f});  // create a Quad, standard width and height (normalized into -1|1), static red

        // set some OpenGL parameters
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);                    // clear the screen
        glViewport(0, 0, (GLsizei) windows[i].getWidth(),
                   (GLsizei) windows[i].getHeight());        // set the drawable arean

        auto colShader = m_glBase.shaderCollector().getStdCol(); // get the shared color shader
        colShader->begin();                                // bind the shader
        colShader->setIdentMatrix4fv("m_pvm");    // set the model-view-projection matrix to an indent matrix
        quad->draw();                                    // draw the quad, the quad will be white which is the standard color of a Quad

        ASSERT_EQ(GL_NO_ERROR, postGLError());

        windows[i].swap();                            // execute the opengl command queue by swapping the framebuffers, runLoop() does this automatically, but we want to quit immediately so do the swap manually here

        // that's all to draw a quad, now we are reading back the framebuffer and check that everything was drawn correctly
        auto data = readBack();

        // check that it's really red
        ASSERT_EQ(postGLError(), GL_NO_ERROR);
        ASSERT_EQ(data[0], 255);
        ASSERT_EQ(data[1], 0);
        ASSERT_EQ(data[2], 0);
        ASSERT_EQ(data[3], 255);
    };
}

TEST(GLBaseTest, ContextSharing) {
    GLBase m_glbase;

    // init the base context
    ASSERT_TRUE(m_glbase.init(false));

    // prepare separate rendering threads
    int nrThreads = 4;
    vector<thread> threads;
    vector<GLFWWindow> windows = vector<GLFWWindow>(nrThreads);

    // create windows, must be not threaded
    for (int i = 0; i < nrThreads; i++) {
        ASSERT_TRUE(windows[i].init(
            glWinPar{
                .doInit = false,            // don't init glfw, this needs to be done on the main thread only once
                .shift = { 300 * i, 100 },  //  offset relative to OS screen canvas
                .size = { 200, 200 },        // set the window's size
                .scaleToMonitor = false,    // maintain pixels to canvas 1:1 if set to true, on windows scaling according to the monitor system scaling
                .shareCont = (void *) m_glbase.getGlfwHnd(),          // share the GLBase context
            })
        );
    }

    // make no context current
    glfwMakeContextCurrent(nullptr);

    // create drawing threads for each window
    threads.reserve(nrThreads);
    for (int i = 0; i < nrThreads; i++) {
        threads.emplace_back(renderLambda(windows, m_glbase, i));
    }

    for (int i = 0; i < nrThreads; i++) {
        threads[i].join();
    }

    for (int i = 0; i < nrThreads; i++) {
        windows[i].destroy(false);
    }

    glfwTerminate();
}
}

