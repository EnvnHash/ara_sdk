#include "GLBaseUnitTestCommon.h"
#include <GeoPrimitives/Quad.h>
#include <Utils/FBO.h>
#include <Utils/Texture.h>
#include <Utils/Typo/TypoGlyphMap.h>
#include "Asset/AssetManager.h"
#include <chrono>

using namespace std;
using namespace glm;
using namespace std::chrono;

namespace ara::GLBaseUnitTest::TypoGlyphMapTest {
    GLFWWindow gwin;           // create an instance, this will do nothing
    glWinPar gp;             // define Parameters for windows instantiating
    bool didRun = false;
    std::unique_ptr<TypoGlyphMap> typo;
    GLBase glbase;
    Texture m_texture(&glbase);
    ShaderCollector shCol;  // create a ShaderCollector
    glm::vec4 white(1.f, 1.f, 1.f, 1.f);
    FIBITMAP *pBitmap;

    bool staticDrawFunc(double, double, int) {
        didRun = true;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);       // clear the screen
        glViewport(0, 0, static_cast<GLsizei>(gp.size.x), static_cast<GLsizei>(gp.size.y)); // set the drawable arean
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // this is how to get the size of the rendered Text in pixel - just for demo reasons
        auto textSize = typo->getPixTextWidth("Hello World", 45);
        // LOG << glm::to_string(textSize);

        std::string str = "Hello World";
        typo->print(0, 25, str, 45, &white[0]);
        gwin.swap();

        EXPECT_EQ(postGLError(),
                  GL_NO_ERROR);    // be sure that there are no GL errors, just for testing, normally not needed

        // read back
        auto data = readBack(gp.size);

        // compare generated and reference image pixel by pixel
        uint8_t *texData = &data[0];
        auto *refTex = (uint8_t *) FreeImage_GetBits(pBitmap);

        for (int y = 0; y < gp.size.y; y++) {
            for (int x = 0; x < gp.size.x; x++) {
                for (int i = 0; i < 3; i++)
                    EXPECT_EQ(*(refTex++), *(texData++));

                ++texData;
                ++refTex;
            }
        }

        gwin.close();
        return false;
    }

    TEST(GLBaseTest, TypoGlyphMapTest) {
        // direct window creation
        gp.size.x = 168;        // set the windows width
        gp.size.y = 25;       // set the windows height
        gp.shift.x = 100;        // x offset relative to OS screen canvas
        gp.shift.y = 100;        // y offset relative to OS screen canvas

        ASSERT_TRUE(gwin.init(gp));    // now pass the arguments and create the window
        ASSERT_EQ(true, initGLEW());

        pBitmap = m_texture.ImageLoader((filesystem::current_path() / "typoGlyphTest.png").string().c_str(), 0);
        ASSERT_TRUE(pBitmap);

        typo = make_unique<TypoGlyphMap>(gp.size.x, gp.size.y);
        typo->loadFont((filesystem::current_path() / "OpenSans-Light.ttf").string().c_str(), &shCol);
        gwin.swap();

        // start a draw loop
        gwin.runLoop(staticDrawFunc);

        EXPECT_TRUE(didRun);
    }
}