#include "GLBaseUnitTestCommon.h"

#include <GeoPrimitives/Quad.h>
#include <Utils/Typo/TypoGlyphMap.h>
#include <Asset/AssetManager.h>

using namespace std;
using namespace std::chrono;

namespace ara::GLBaseUnitTest::GLBaseStopStartResources {

TEST(GLBaseTest, GLBaseStopStartResources) {
    // create and destroy the GLBase context - there should be no timeouts, deadlocks or memleaks
    int nrIterations = 50;
    for (int i = 0; i < nrIterations; i++) {
        GLBase m_glbase;
        EXPECT_EQ(m_glbase.init(), true);

        m_glbase.startGlCallbackProcLoop(); // blocks until the loop is really running

        // push something into the queue to process
        Conditional sema;
        m_glbase.addGlCb([&] {

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
        }, &sema);
        sema.wait(0);

        m_glbase.stopProcCallbackLoop();                                       // blocks until the loop is really finished
        m_glbase.destroy(true);

        std::cout << "." << std::flush;
    }
}

}