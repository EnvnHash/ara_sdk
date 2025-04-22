#include "GLBaseUnitTestCommon.h"
#include <WindowManagement/GLWindow.h>

using namespace std;

namespace ara::GLBaseUnitTest::NativeWindow {
    TEST(GLBaseTest, NativeWindow) {
        GLWindow gwin;                  // create an instance, this will do nothing
        glWinPar gp;                    // define Parameters for windows instanciating

        // direct window creation
        gp.debug = false;
        gp.width = 1920;                // set the windows width
        gp.height = 1080;                // set the windows height
        gp.shiftX = 100;                // x offset relative to OS screen canvas
        gp.shiftY = 100;                // y offset relative to OS screen canvas
        gp.createHidden = false;
        gp.decorated = true;

        // set a draw function otherwise the window will be transparent
        gwin.setDrawFunc([](double time, double dt, int ctxNr) {
            glClearColor(0.f, 0.f, 0.f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            return true;
        });

        gwin.create(gp);             // now pass the arguments and create the window
        gwin.draw();                    // execute the draw function (normally the startDrawThread method would be used)
        gwin.destroy();

        EXPECT_TRUE(true);
    }
}

