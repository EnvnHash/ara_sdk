#include "GLBaseUnitTestCommon.h"

#include <Utils/FBO.h>
#include <Utils/Texture.h>
#include <GeoPrimitives/Quad.h>
#include <Utils/Typo/TypoGlyphMap.h>
#include "Res/AssetManager.h"

using namespace std;

namespace ara::GLBaseUnitTest::ContextSharingFbo {
TEST(GLBaseTest, ContextSharingFbo) {
    GLBase m_glbase;

    // init the base context
    ASSERT_TRUE(m_glbase.init(false));
    m_glbase.makeCurrent();

    auto quad = std::make_unique<Quad>(QuadInitParams{-1.f, -1.f, 2.f, 2.f, glm::vec3(0.f, 0.f, 1.f), 1.f, 1.f, 1.f, 1.f});

    // load a test texture
    Texture texMan(&m_glbase);
    texMan.loadFromFile("FullHD_Pattern.png", GL_TEXTURE_2D, 1);

    // create an FBO on the main gl-context
    FBO sharedFBO(&m_glbase, 512, 512, 1);

    // draw the texture to the FBO
    Shaders *stdTex = m_glbase.shaderCollector().getStdTex();

    // write to s_fbo
    sharedFBO.bind();
    sharedFBO.clear();

    stdTex->begin();
    stdTex->setIdentMatrix4fv("m_pvm");
    stdTex->setUniform1i("tex", 0);
    texMan.bind(0);
    quad->draw();

    sharedFBO.unbind();

    m_glbase.getWin()->swap();    // execute the drawing to the FBO

    texMan.releaseTexture();

    // -----------------------------------------------------------

    // prepare separate rendering threads
    int nrThreads = 4;
    vector<thread> threads;
    vector<GLFWWindow> windows = vector<GLFWWindow>(nrThreads);

    // create windows, must be not threaded
    for (int i = 0; i < nrThreads; i++) {
        ASSERT_TRUE(windows[i].init(glWinPar{
                .doInit = false,                  // don't init glfw, this needs to be done on the main thread only once
                .shiftX = 300 * i,                // x offset relative to OS screen canvas
                .shiftY = 100,                    // y offset relative to OS screen canvas
                .width = 200,                     // set the window's width
                .height = 200,                    // set the window's height
                .scaleToMonitor = false,          // maintain pixels to canvas 1:1 if set to true, on windows scaling according to the monitor system scaling
                .shareCont = m_glbase.getGlfwHnd()
            }));   // now pass the arguments and create the window, this make the windows gl context current
        ASSERT_EQ(true, initGLEW());
    }

    // make no context current
    glfwMakeContextCurrent(nullptr);

    // create drawing threads for each window
    for (int i = 0; i < nrThreads; i++)
        threads.emplace_back([&windows, &m_glbase, &sharedFBO, &texMan, i]() {

            windows[i].makeCurrent();

            // since we are using the same resource, we need to aquire a lock to the GLBase
            unique_lock<mutex> lock(*m_glbase.glMtx());

            Shaders *texShader = m_glbase.shaderCollector().getStdTex(); // get the shared color shader

            FBO localFBO;
            localFBO.setGlbase(&m_glbase);
            localFBO.fromShared(&sharedFBO);
            ASSERT_EQ(GL_NO_ERROR, postGLError());

            // unfortunately, VAOs can't be shared since they are just a set of states, but don't contain actual data
            // create a Quad, standard width and height (normalized into -1|1), static red
            unique_ptr<Quad> quad = make_unique<Quad>(QuadInitParams{-1.f, -1.f, 2.f, 2.f,
                                                                    glm::vec3(0.f, 0.f, 1.f),
                                                                    1.f, 0.f, 0.f, 1.f});
            // set some OpenGL parameters
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);                    // clear the screen
            glViewport(0, 0, (GLsizei) windows[i].getWidth(),
                       (GLsizei) windows[i].getHeight());        // set the drawable arean

            texShader->begin();                                // bind the shader
            texShader->setIdentMatrix4fv("m_pvm");    // set the model-view-projection matrix to an indent matrix
            texShader->setUniform1i("tex", 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, localFBO.getColorImg());

            quad->draw();                                    // draw the quad, the quad will be white which is the standard color of a Quad

            ASSERT_EQ(GL_NO_ERROR, postGLError());

            windows[i].swap();                            // execute the opengl command queue by swapping the framebuffers, runLoop() does this automatically, but we want to quit immediately so do the swap manually here

            // that's all to draw a quad, now we are reading back the framebuffer and check that everything was drawn correctly
            // read back
            vector<GLubyte> data(windows[i].getWidth() * 20 * 4);    // make some space to download
            glReadBuffer(GL_FRONT);
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glReadPixels(0, 0, windows[i].getWidth(), 20, GL_RGBA, GL_UNSIGNED_BYTE,
                         &data[0]);    // synchronous, blocking command, no swap() needed

            int offs = (windows[i].getWidth() * 19 + windows[i].getWidth() / 2) * 4;

            // check that it's really red
            ASSERT_EQ(postGLError(), GL_NO_ERROR);
            ASSERT_EQ((int) data[offs], 160);
            ASSERT_EQ((int) data[offs + 1], 160);
            ASSERT_EQ((int) data[offs + 2], 160);
            ASSERT_EQ((int) data[offs + 3], 255);
        });

    for (int i = 0; i < nrThreads; i++)
        threads[i].join();

    m_glbase.makeCurrent();
    sharedFBO.remove();
    glfwMakeContextCurrent(nullptr);

    for (int i = 0; i < nrThreads; i++) {
        windows[i].destroy(false);
    }
}

}

