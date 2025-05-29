#include "GLBaseUnitTestCommon.h"

#include <GeoPrimitives/Quad.h>
#include <Utils/Typo/TypoGlyphMap.h>
#include "Asset/AssetManager.h"

#include <chrono>

using namespace std;
using namespace std::chrono;

namespace ara::GLBaseUnitTest::GLBaseCreationDestruction {

TEST(GLBaseTest, GLBaseCreationDestruction) {
    int nrIterations = 30;

    // create and destroy the GLBase context - there should be no timeouts, deadlocks or memleaks
    for (int i = 0; i < nrIterations; i++) {
        GLBase m_glbase;
        // create a GLBase instance
        EXPECT_EQ(m_glbase.init(false), true);                  // init is synchronous
        m_glbase.startGlCallbackProcLoop();                             // blocks until the loop is really running

        // push something into the queue to process
        m_glbase.addGlCbSync([&] {

            Shaders *colShader = m_glbase.shaderCollector().getStdCol(); // get the shared color shader
            auto quad = std::make_unique<Quad>(QuadInitParams{});

            // set some OpenGL parameters
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glViewport(0, 0, 50, 50);

            colShader->begin();
            colShader->setIdentMatrix4fv("m_pvm");

            quad->draw();

            // insert some timing variation
            std::this_thread::sleep_for(std::chrono::microseconds(i));

            EXPECT_EQ(postGLError(), GL_NO_ERROR);
            return true;
        });

        m_glbase.stopProcCallbackLoop();                               // blocks until the loop is really finished
        m_glbase.destroy(i == (nrIterations -
                               1));         // destroy the GLBase context. on the last iteration terminate glfw

        std::cout << "." << std::flush;
    }
}

}