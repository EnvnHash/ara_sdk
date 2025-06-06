#include "GLBaseUnitTestCommon.h"
#include <GLRenderer.h>
#include <GeoPrimitives/Quad.h>
#include "Asset/AssetManager.h"

using namespace std;

namespace ara::GLBaseUnitTest::GLRendererTest {

TEST(GLBaseTest, GLRendererTest) {
    GLBase m_glbase;

    ASSERT_TRUE(m_glbase.init()); // needs the resources for TestPattern to work
    m_glbase.startGlCallbackProcLoop();

    int nrIt = 10;
    for (int i = 0; i < nrIt; i++) {
        GLRenderer rend;
        rend.setGlBase(&m_glbase);
        rend.init("renderer", glm::ivec2(0, 0), glm::ivec2(1920, 1080), true);
        rend.close(true);
        std::cout << "." << std::flush;
    }
    std::cout << std::endl;

    // same game but via EventLoop
    auto t = std::thread([&] {
        Conditional finSenma;
        GLRenderer rend;
        rend.setGlBase(&m_glbase);
        int nrItr = 10;
        for (int i = 0; i < nrItr; i++) {
            m_glbase.runOnMainThread([&rend, &finSenma, i, nrItr] {
                rend.init("renderer", glm::ivec2(0, 0), glm::ivec2(1920, 1080), true);
                rend.close(true);
                std::cout << "." << std::flush;
                if (i == (nrItr - 1))
                    finSenma.notify();
                return true;
            });
        }

        finSenma.wait(0);
        std::cout << std::endl;

        m_glbase.getWinMan()->stopEventLoop();
    });
    t.detach();

    m_glbase.getWinMan()->startEventLoop(); // blocking
    std::cout << std::endl;

    m_glbase.stopProcCallbackLoop();
    m_glbase.destroy();
}

}

