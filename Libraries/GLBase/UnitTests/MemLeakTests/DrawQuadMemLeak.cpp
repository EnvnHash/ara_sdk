#include "GLBaseUnitTestCommon.h"
#include <GeoPrimitives/Quad.h>

#ifdef _WIN32
#include <crtdbg.h>
#endif

using namespace std;

namespace ara::GLBaseUnitTest::DrawQuadMemLeak {
    ShaderCollector shCol;  // create a ShaderCollector
    GLFWWindow gwin;
    Shaders *colShader;
    glWinPar gp;             // define Parameters for windows instanciating

    bool didFindMemLeak = false;

    TEST(GLBaseTest, DrawQuadMemLeak) {
        // direct window creation
        // gp.debug = true;
        gp.width = 1920;            // set the windows width
        gp.height = 1080;            // set the windows height
        gp.shiftX = 100;            // x offset relative to OS screen canvas
        gp.shiftY = 100;            // y offset relative to OS screen canvas
        gp.scaleToMonitor = false;  // maintain pixels to canvas 1:1 if set to true, on windows scaling according to the monitor system scaling accours
        gp.createHidden = false;  // maintain pixels to canvas 1:1 if set to true, on windows scaling according to the monitor system scaling accours

        ASSERT_TRUE(gwin.create(gp));    // now pass the arguments and create the window

        // init glew
        ara::initGLEW();

#ifdef _WIN32
        _CrtMemState sOld, sNew, sDiff;
        _CrtMemCheckpoint(&sOld); //take a snapshot
#endif

        for (int i = 0; i < 10; i++) {
            auto quad = make_unique<Quad>(-1.f, -1.f, 2.f, 2.f,
                                          glm::vec3(0.f, 0.f, 1.f),
                                          1.f, 0.f, 0.f,
                                          1.f);  // create a Quad, standard width and height (normalized into -1|1), static red
            gwin.swap();
            quad.reset();

#ifdef _WIN32
            didFindMemLeak = checkMemLeak(sNew, sOld, sDiff);
#endif
        }

        EXPECT_FALSE(didFindMemLeak);
    }
}

