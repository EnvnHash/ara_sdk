#include "GLBaseUnitTestCommon.h"
#include <WindowManagement/GLWindow.h>

using namespace std;

namespace ara::GLBaseUnitTest::NativeWindow {
    TEST(GLBaseTest, NativeWindow) {
        GLWindow gwin;                  // create an instance, this will do nothing

        // set a draw function otherwise the window will be transparent
        gwin.setDrawFunc([](double time, double dt, int ctxNr) {
            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            return true;
        });

        // create the window
        gwin.create(glWinPar{
            .decorated = true,
            .createHidden = false,
            .debug = false,
            .shift = { 100, 100 },
            .size = { 1920, 1080 }
        });

        gwin.draw();    // execute the draw function (normally the startDrawThread method would be used)
        gwin.destroy();

        EXPECT_TRUE(true);
    }
}

