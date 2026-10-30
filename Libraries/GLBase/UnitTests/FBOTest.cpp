//
// Created by sven on 30-10-26.
//

#include "GLBaseUnitTestCommon.h"

using namespace std;

namespace ara::GLBaseUnitTest::Fbo {
    static GLBase glbase;
    static GLFWWindow gwin;
    static Texture tex(&glbase);

    TEST(GLBaseTest, FboSaveToFile) {
        const std::string testFileName = "FboSaveFileTest.png";
        constexpr glm::ivec2 testImageSize = {4,4};

        createGlBaseCtx(gwin);

        tex.allocate2D(testImageSize.x, testImageSize.y, GL_RGBA8, GL_RGBA, GL_TEXTURE_2D, GL_UNSIGNED_BYTE, 1, 0);

        const auto fbo = std::make_unique<FBO>();
        fbo->setGlbase(&glbase);
        fbo->fromTex(tex.getId(), testImageSize.x, testImageSize.y, GL_RGBA8, 1, GL_LINEAR, GL_LINEAR);
        fbo->saveToFile(testFileName, 0, GL_RGBA8);

        const auto bitmap = FreeImage::Load(testFileName, nullptr);
        EXPECT_TRUE(bitmap);
        const auto data = FreeImage_GetBits(bitmap);

        for (uint32_t y = 0; y < testImageSize.x * testImageSize.y *4; y++) {
            EXPECT_EQ(static_cast<int32_t>(data[y]), 0);
        }
    }
}