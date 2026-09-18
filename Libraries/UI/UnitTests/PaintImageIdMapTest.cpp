#include "TestCommon.h"
#include "UIApplication.h"
#include "UIElements/PaintImageIdMap.h"

using namespace std;

namespace ara::UiUnitTest::PaintImageIdMapTests {

PaintImageIdMap& addImageBase(const UIApplication &app) {
    auto& img = app.getMainWindow()->getRootNode()->push<PaintImageIdMap>();
    img.setSize(200, 200);
    img.setAlign(align::center, valign::center);
    img.setBackgroundColor(0.f, 0.f, 0.f, 1.f);
    return img;
}

PaintImageIdMap& addImage(const UIApplication &app) {
    auto& img = addImageBase(app);
    img.setImg((filesystem::current_path() / "black.png").string(), 1);
    return img;
}

PaintImage::Brush getStdBrush(float id = 0.0f) {
    return PaintImage::Brush {
        .size = 25.f,
        .hardness = 0.2f,
        .color = glm::vec4(id, 0.f, 0.f, 1.f),
        .opacity = 1.0f
    };
}

TEST(UITest, PaintImageIdMapBasic) {
    appBody([&](const UIApplication &app) {
        auto& img = addImage(app);
        const auto brush = getStdBrush(0.0f);
        img.setBrush(brush);
        img.setVisualizationID(0);
        iterate(app);
        simulateMouseClick(app, 150, 150);
    }, [&](const UIApplication &app) {
        // Point (150, 150) corresponds to center of window (300, 300)
        // Check pixel at center is white (255, 255, 255, 255)
        auto pixels = getPixels(150, 150, 1, 1);
        EXPECT_EQ(pixels[0], 255);
        EXPECT_EQ(pixels[1], 255);
        EXPECT_EQ(pixels[2], 255);
        EXPECT_EQ(pixels[3], 255);
    }, 300, 300);
}

TEST(UITest, PaintImageIdMapVisMask) {
    appBody([&](const UIApplication &app) {
        auto& img = addImage(app);
        // Paint with ID 1
        img.setBrush(getStdBrush(1.0f));
        iterate(app);
        simulateMouseClick(app, 150, 150);

        // When visualizationID is 0, center pixel should not have ID 0 bit set (it's black background)
        img.setVisualizationID(0);
        iterate(app);
    }, [&](const UIApplication &app) {
        auto pixels = getPixels(150, 150, 1, 1);
        EXPECT_EQ(pixels[0], 0);
        EXPECT_EQ(pixels[1], 0);
        EXPECT_EQ(pixels[2], 0);
    }, 300, 300);
}

TEST(UITest, PaintImageIdMapMultipleIds) {
    appBody([&](const UIApplication &app) {
        auto& img = addImage(app);
        // Paint ID 0 at (100, 100) (window coords)
        img.setBrush(getStdBrush(0.0f));
        iterate(app);
        simulateMouseClick(app, 100, 100);

        // Paint ID 5 at (200, 200)
        img.setBrush(getStdBrush(5.0f));
        iterate(app);
        simulateMouseClick(app, 200, 200);

        // Visualize only ID 5
        img.setVisualizationID(5);
        iterate(app);
    }, [&](const UIApplication &app) {
        // (200, 200) in window (OpenGL y: 200)
        auto pixels5 = getPixels(200, 200, 1, 1);
        EXPECT_EQ(pixels5[0], 255);
        EXPECT_EQ(pixels5[1], 255);
        EXPECT_EQ(pixels5[2], 255);

        // (100, 100) in window (OpenGL y: 100) should be 0 because only ID 5 is visualized
        auto pixels0 = getPixels(100, 100, 1, 1);
        EXPECT_EQ(pixels0[0], 0);
        EXPECT_EQ(pixels0[1], 0);
        EXPECT_EQ(pixels0[2], 0);
    }, 300, 300);
}

} // namespace ara::UiUnitTest::PaintImageIdMapTests
