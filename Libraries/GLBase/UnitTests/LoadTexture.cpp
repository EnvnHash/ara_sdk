#include "GLBaseUnitTestCommon.h"
#include <Utils/Texture.h>
#include <GeoPrimitives/Quad.h>
#include <Utils/Typo/TypoGlyphMap.h>
#include "Asset/AssetManager.h"

using namespace std;
using fs = std::filesystem::path;

namespace ara::GLBaseUnitTest::LoadTexture {
    GLFWWindow gwin;
    unique_ptr<Quad> quad;
    Shaders *texShader;
    GLBase m_glbase;
    Texture tex(&m_glbase);
    glWinPar gp;             // define Parameters for windows instantiating

    bool didRun = false;

    void staticDrawFunc() {
        // set some OpenGL parameters
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);              // clear the screen
        glViewport(0, 0, gp.size.x, gp.size.y);        // set the drawable area

        texShader->begin();                             // bind the shader
        texShader->setIdentMatrix4fv("m_pvm");      // set the model-view-projection matrix to an indent matrix
        texShader->setUniform1i("tex", 0);      // set the texture unit we are using
        tex.bind(0);                                   // bind the texture to that unit
        quad->draw();                                   // draw the quad, the quad will be white which is the standard color of a Quad

        ASSERT_EQ(postGLError(), GL_NO_ERROR);          // be sure that there are no GL errors, just for testing, normally not needed
        gwin.swap();                                    // execute the opengl command queue by swapping the frame buffers, runLoop() does this automatically, but we want to quit immediately so do the swap manually here

        // that's all to draw a quad, now we are reading back the framebuffer and check that everything was drawn correctly

        // read back
        vector<GLubyte> data(gp.size.x * gp.size.y * 4);    // make some space to download
        glReadBuffer(GL_FRONT);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glReadPixels(0, 0, gp.size.x, gp.size.y, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);    // synchronous, blocking command, no swap() needed
        ASSERT_EQ(postGLError(), GL_NO_ERROR);

        int segStep = gp.size.x / 4;
        int segStepHalf = segStep / 2;

        // define colors to expect and their position
        vector<tuple<int, int, glm::ivec3>> checkColors;
        checkColors.emplace_back(segStepHalf, segStepHalf, glm::ivec3(255, 0, 0));
        checkColors.emplace_back(segStep + segStepHalf, segStepHalf, glm::ivec3(0, 255, 0));
        checkColors.emplace_back(segStep * 2 + segStepHalf, segStepHalf, glm::ivec3(0, 0, 255));
        checkColors.emplace_back(segStep * 3 + segStepHalf, segStepHalf, glm::ivec3(0, 255, 255));

        checkColors.emplace_back(segStepHalf, segStep + segStepHalf, glm::ivec3(0, 0, 255));
        checkColors.emplace_back(segStep + segStepHalf, segStep + segStepHalf, glm::ivec3(0, 255, 255));
        checkColors.emplace_back(segStep * 2 + segStepHalf, segStep + segStepHalf, glm::ivec3(255, 0, 255));
        checkColors.emplace_back(segStep * 3 + segStepHalf, segStep + segStepHalf, glm::ivec3(255, 255, 0));

        checkColors.emplace_back(segStepHalf, segStep * 2 + segStepHalf, glm::ivec3(255, 0, 255));
        checkColors.emplace_back(segStep + segStepHalf, segStep * 2 + segStepHalf, glm::ivec3(255, 255, 0));
        checkColors.emplace_back(segStep * 2 + segStepHalf, segStep * 2 + segStepHalf, glm::ivec3(255, 0, 0));
        checkColors.emplace_back(segStep * 3 + segStepHalf, segStep * 2 + segStepHalf, glm::ivec3(0, 255, 0));

        checkColors.emplace_back(segStepHalf, segStep * 3 + segStepHalf, glm::ivec3(255, 0, 0));
        checkColors.emplace_back(segStep + segStepHalf, segStep * 3 + segStepHalf, glm::ivec3(0, 255, 0));
        checkColors.emplace_back(segStep * 2 + segStepHalf, segStep * 3 + segStepHalf, glm::ivec3(0, 0, 255));
        checkColors.emplace_back(segStep * 3 + segStepHalf, segStep * 3 + segStepHalf, glm::ivec3(0, 255, 255));

        for (auto &it: checkColors) {
            size_t ptr = (std::get<0>(it) + std::get<1>(it) * gp.size.x) * 4;
            for (int i = 0; i < 3; i++) {
                ASSERT_EQ(static_cast<int>(data[ptr + i]), std::get<2>(it)[i]);
            }
        }

        didRun = true;
        gwin.destroy();
    }

    TEST(GLBaseTest, LoadTexture) {
        // direct window creation
        // gp.debug = true;
        gp.size.x = 64;              // set the windows width
        gp.size.y = 64;             // set the windows height
        gp.scaleToMonitor = false;  // maintain pixels to canvas 1:1 if set to true, on windows scaling according to the monitor system scaling
        gp.createHidden = false;    // maintain pixels to canvas 1:1 if set to true, on windows scaling according to the monitor system scaling

        ASSERT_TRUE(gwin.create(gp));    // now pass the arguments and create the window
        ASSERT_EQ(true, initGLEW());

        quad = make_unique<Quad>(QuadInitParams{-1.f, -1.f, 2.f, 2.f,
                                                glm::vec3(0.f, 0.f, 1.f),
                                                1.f, 0.f, 0.f,
                                                1.f});  // create a Quad, standard width and height (normalized into -1|1), static red
        texShader = m_glbase.shaderCollector().getStdTex(); // get a simple standard color shader
        tex.loadTexture2D("loadTexTest.png", 1);
        staticDrawFunc();

        EXPECT_TRUE(didRun);
    }
}

