#include <gtest/gtest.h>
#include <GLBase.h>

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
        gp.doInit = true;            // GWindow needs GLFW library to be m_inited. GWindow can do this itself. Standard is true
        gp.shiftX = 100;            // x offset relative to OS screen canvas
        gp.shiftY = 100;            // y offset relative to OS screen canvas
        gp.scaleToMonitor = false;  // maintain pixels to canvas 1:1 if set to true, on windows scaling according to the monitor system scaling accours
        gp.createHidden = false;  // maintain pixels to canvas 1:1 if set to true, on windows scaling according to the monitor system scaling accours

        ASSERT_TRUE(gwin.create(gp));    // now pass the arguments and create the window

        //DWORDLONG virtualMemUsedInit = memInfo.ullTotalPageFile - memInfo.ullAvailPageFile;

        // init glew
        ara::initGLEW();

#ifdef _WIN32
        _CrtMemState sOld;
        _CrtMemState sNew;
        _CrtMemState sDiff;
        _CrtMemCheckpoint(&sOld); //take a snapchot
#endif

        for (int i = 0; i < 10; i++) {
            //auto m_vao = make_unique<VAO>("position:4f,normal:4f,color:4f", GL_STATIC_DRAW);
            auto quad = make_unique<Quad>(-1.f, -1.f, 2.f, 2.f,
                                          glm::vec3(0.f, 0.f, 1.f),
                                          1.f, 0.f, 0.f,
                                          1.f);  // create a Quad, standard width and height (normalized into -1|1), static red
            gwin.swap();
            quad.reset();

#ifdef _WIN32
            _CrtMemCheckpoint(&sNew); //take a snapchot
            if (_CrtMemDifference(&sDiff, &sOld, &sNew)) // if there is a difference
            {
                LOG << "-----------_CrtMemDumpStatistics ---------";
                _CrtMemDumpStatistics(&sDiff);
                OutputDebugString("-----------_CrtMemDumpAllObjectsSince ---------");
                _CrtMemDumpAllObjectsSince(&sOld);
                OutputDebugString("-----------_CrtDumpMemoryLeaks ---------");
                _CrtDumpMemoryLeaks();
                didFindMemLeak = true;
            }
#endif
        }

        EXPECT_FALSE(didFindMemLeak);
    }
}

