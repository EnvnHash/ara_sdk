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
            .debug = false,
            .width = 1920,      // set the windows width
            .height = 1080,     // set the windows height
            .shiftX = 100,      // x offset relative to OS screen canvas
            .shiftY = 100,      // y offset relative to OS screen canvas
            .createHidden = false,
            .decorated = true
        });

        gwin.draw();                    // execute the draw function (normally the startDrawThread method would be used)
        gwin.destroy();

        EXPECT_TRUE(true);
    }
}

