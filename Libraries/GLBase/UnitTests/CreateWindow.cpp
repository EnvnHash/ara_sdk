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
        ASSERT_TRUE(gwin.init(glWinPar{
            .shiftX = 100,        // x offset relative to OS screen canvas
            .shiftY = 100,        // y offset relative to OS screen canvas
            .width = 1920,        // set the windows width
            .height = 1080,       // set the windows height
        }));

        ASSERT_EQ(true, initGLEW());

        // start a draw loop
        gwin.runLoop(staticDrawFunc);

        EXPECT_TRUE(didRun);
    }
}
