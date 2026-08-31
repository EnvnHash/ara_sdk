
#include <gtest/gtest.h>
#include <ImageIO/FreeImageHandler.h>
#include <filesystem>
#include <iostream>
#include <vector>

namespace ara::FreeImage::Test {

// Minimal implementation of pixel comparison since we can't easily include GLBaseUnitTestCommon.h here
static void compareImages(const std::filesystem::path& path1, const std::filesystem::path& path2) {
    FIBITMAP* bmp1 = Load(path1.string(), nullptr);
    FIBITMAP* bmp2 = Load(path2.string(), nullptr);

    ASSERT_TRUE(bmp1) << "Failed to load " << path1;
    ASSERT_TRUE(bmp2) << "Failed to load " << path2;

    const auto sz1 = GetSizeFromBitmap(bmp1);
    const auto sz2 = GetSizeFromBitmap(bmp2);

    EXPECT_EQ(sz1[0], sz2[0]);
    EXPECT_EQ(sz1[1], sz2[1]);

    if (sz1[0] == sz2[0] && sz1[1] == sz2[1]) {
        const uint32_t width = sz1[0];
        const uint32_t height = sz1[1];
        const uint32_t pitch1 = FreeImage_GetPitch(bmp1);
        const uint32_t pitch2 = FreeImage_GetPitch(bmp2);
        const uint8_t* bits1 = FreeImage_GetBits(bmp1);
        const uint8_t* bits2 = FreeImage_GetBits(bmp2);
        const uint32_t bpp = FreeImage_GetBPP(bmp1);

        EXPECT_EQ(bpp, FreeImage_GetBPP(bmp2));

        for (uint32_t y = 0; y < height; ++y) {
            const auto line1 = bits1 + y * pitch1;
            const auto line2 = bits2 + y * pitch2;
            for (uint32_t x = 0; x < width * (bpp / 8); ++x) {
                ASSERT_EQ(line1[x], line2[x]) << "Mismatch at y=" << y << " byte=" << x;
            }
        }
    }

    Unload(bmp1);
    Unload(bmp2);
}

TEST(FreeImageHandlerTest, ExpandImage) {
    const auto inputPath = std::filesystem::current_path() / "expand_test_input.png";
    const auto goldenPath = std::filesystem::current_path() / "expand_test_golden.png";
    const auto origSize = GetSize(inputPath.string());
    const auto newWidth = origSize[0] + 100;
    const auto newHeight = origSize[1] + 100;

    // Perform expansion
    ExpandImage(inputPath, newWidth, newHeight, true);

    // Verify size
    const auto resultSize = GetSize(inputPath.string());
    EXPECT_EQ(resultSize[0], newWidth);
    EXPECT_EQ(resultSize[1], newHeight);

    // Compare to golden image
    ASSERT_TRUE(std::filesystem::exists(goldenPath)) << "Golden image not found at " << goldenPath;

    // Use internal compareImages
    compareImages(inputPath, goldenPath);
}

} // namespace ara::FreeImage::Test
