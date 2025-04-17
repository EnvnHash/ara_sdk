#include <gtest/gtest.h>
#include <GLBase.h>
#include <GLRenderer.h>
#include <GeoPrimitives/Quad.h>
#include <Utils/Texture.h>
#include "Res/ResInstance.h"

using namespace std;

namespace ara::GLBaseUnitTest::GLRendererTest {

TEST(GLBaseTest, GLRendererTest) {
    GLBase m_glbase;

    ASSERT_TRUE(m_glbase.init()); // needs the resources for TestPattern to work
    m_glbase.startRenderLoop();

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
        int nrIt = 10;
        for (int i = 0; i < nrIt; i++) {
            m_glbase.runOnMainThread([&rend, &finSenma, i, nrIt] {
                rend.init("renderer", glm::ivec2(0, 0), glm::ivec2(1920, 1080), true);
                rend.close(true);
                std::cout << "." << std::flush;
                if (i == (nrIt - 1))
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

    m_glbase.stopRenderLoop();
    m_glbase.destroy();
}

}

