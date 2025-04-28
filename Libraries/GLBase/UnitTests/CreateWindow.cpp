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
            .shift = { 100, 100 },  //  offset relative to OS screen canvas
            .size = { 1920, 1080 }  // set the windows size
        }));

        ASSERT_EQ(true, initGLEW());

        // start a draw loop
        gwin.runLoop(staticDrawFunc);

        EXPECT_TRUE(didRun);
    }
}
