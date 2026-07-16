//
// Created by sven on 01-07-25.
//

#include "GLBaseUnitTestCommon.h"

#include <ImageIO/FreeImageHandler.h>
#include "FreeImageTestBitmap.h"

using namespace std;
using namespace glm;
using fs = std::filesystem::path;

namespace ara::GLBaseUnitTest::FreeImage {

static void compareBitmap(const uint8_t* pixels, const std::array<uint32_t, 2> sz) {
    for (int y=0; y<static_cast<int32_t>(sz[1]); ++y) {
        for (int x=0; x<static_cast<int32_t>(sz[0]); ++x) {
            for (int c=0; c<4; ++c) {
                EXPECT_EQ(static_cast<int32_t>(*pixels++), refBitmap[y][x][c]);
            }
        }
    }
}

static void checkBitmap(FIBITMAP* bitmap) {
    const auto sz = ara::FreeImage::GetSizeFromBitmap(bitmap);
    EXPECT_EQ(sz[0], 32);
    EXPECT_EQ(sz[1], 32);

    const auto pixels = ara::FreeImage::GetBits(bitmap);
    compareBitmap(pixels, sz);
}

TEST(GLBaseTest, LoadImgSuccess) {
    const auto bitmap = ara::FreeImage::Load("freeimage_test_img.png", 0);
    EXPECT_TRUE(bitmap);
    checkBitmap(bitmap);
    ara::FreeImage::Unload(bitmap);
}

TEST(GLBaseTest, LoadImgFromMem) {
    std::vector<uint8_t> vp;
    AssetLoader::setAssetPath("resdata");
    auto sample = AssetLoader::loadAssetToMem(vp, filesystem::path("test") / "freeimage_test_img.png");
    const auto bitmap = ara::FreeImage::Load(vp);
    EXPECT_TRUE(bitmap);
    checkBitmap(bitmap);
    ara::FreeImage::Unload(bitmap);
}

TEST(GLBaseTest, CropAndScale) {
    ara::FreeImage::CropAndScale("FullHD_Pattern_24bit.png", { 100,100 }, { 400, 400 }, { 400, 400 }, "FullHD_cropped_rgba.png");
    compareTwoFiles("FullHD_cropped_rgba.png", "FullHD_cropped_rgba_ref.png");
}

}