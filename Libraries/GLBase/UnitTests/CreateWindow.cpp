#include "GLBaseUnitTestCommon.h"


using namespace std;

namespace ara::GLBaseUnitTest::CreateWindowTest {
    GLFWWindow gwin;           // create an instance, this will do nothing
    bool didRun = false;

    bool staticDrawFunc(double, double, int) {
        didRun = true;
        gwin.close();
        return false;
    }

    TEST(GLBaseTest, CreateWindowTest) {
        // direct window creation
        glWinPar gp;             // define Parameters for windows instanciating
        gp.width = 1920;        // set the windows width
        gp.height = 1080;       // set the windows height
        gp.doInit = true;       // GWindow needs GLFW library to be m_inited. GWindow can do this itself. Standard is true
        gp.shiftX = 100;        // x offset relative to OS screen canvas
        gp.shiftY = 100;        // y offset relative to OS screen canvas

        ASSERT_TRUE(gwin.init(gp));    // now pass the arguments and create the window
        ASSERT_EQ(true, initGLEW());

        // start a draw loop
        gwin.runLoop(staticDrawFunc);

        EXPECT_TRUE(didRun);
    }
}
