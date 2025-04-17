#include <gtest/gtest.h>
#include <GLBase.h>
#include <GeoPrimitives/Quad.h>
#include <Utils/FBO.h>
#include <Utils/Texture.h>
#include <Utils/TypoGlyphMap.h>
#include "Res/ResInstance.h"

using namespace std;
using namespace std::chrono;

namespace ara::GLBaseUnitTest::CreateFbo {
TEST(GLBaseTest, CreateFbo) {
    GLBase m_glbase;

    // take the static GLBase instance
    m_glbase.init(false);
    m_glbase.makeCurrent();

    int nrIterations = 40;
    double sum = 0.0;

    for (int i = 0; i < nrIterations; i++) {
        //auto startTime = system_clock::now();

        // initialize and s_fbo on the singleton GL-context
        //std::cout << "." << std::flush;

        FBO fbo(&m_glbase, 3840, 2160, GL_RGBA8, GL_TEXTURE_2D, false, 1, 1, 1, GL_CLAMP_TO_EDGE, false);

        //std::this_thread::sleep_for(std::chrono::milliseconds(10));

        EXPECT_EQ(postGLError(), GL_NO_ERROR);

        glFinish();
    }
}
}