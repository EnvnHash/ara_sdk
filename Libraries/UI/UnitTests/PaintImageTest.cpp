#include "TestCommon.h"
#include "UIApplication.h"
#include "UIElements/PaintImage.h"

using namespace std;

namespace ara::UiUnitTest::PaintImageTests {

PaintImage& addImage (const UIApplication &app) {
    auto& img = app.getMainWindow()->getRootNode()->push<PaintImage>();
    img.setSize(200, 200);
    img.setAlign(align::center, valign::center);
    img.setBackgroundColor(0.2f, 0.2f, 0.2f, 1.f);
    img.setImg(( std::filesystem::path("test") / "black.png").string(), 1);
    return img;
}

PaintImage::Brush getStdBrush() {
    return PaintImage::Brush {
        .size = 50.f,
        .hardness = 0.2f,
        .color = glm::vec4(1.f, 1.f, 1.f, 1.0f),
        .opacity = 1.0f
    };
}

TEST(UITest, PaintImageBasic) {
    appBody([&](const UIApplication &app) {
        auto& img = addImage(app);
        auto brush = getStdBrush();
        img.setBrush(brush);

        iterate(app);
        simulateMouseClick(app, 150, 150);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "brush_basic.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 300);
}

TEST(UITest, PaintImageHard1) {
    appBody([&](const UIApplication &app) {
        auto& img = addImage(app);
        auto brush = getStdBrush();
        brush.hardness = 1.0;
        img.setBrush(brush);

        iterate(app);
        simulateMouseClick(app, 150, 150);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "brush_hardness_1.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 300);
}

TEST(UITest, PaintImagePos) {
    appBody([&](const UIApplication &app) {
        auto& img = addImage(app);
        auto brush = getStdBrush();
        brush.hardness = 1.0;
        img.setBrush(brush);

        iterate(app);
        simulateMouseClick(app, 50, 50);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "brush_hardness_pos.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 300);
}

TEST(UITest, PaintImagePos2) {
    appBody([&](const UIApplication &app) {
        auto& img = addImage(app);
        auto brush = getStdBrush();
        brush.hardness = 1.0;
        img.setBrush(brush);

        iterate(app);
        simulateMouseClick(app, 250, 250);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "brush_hardness_pos2.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 300);
}

TEST(UITest, PaintImagePAddTest) {
    appBody([&](const UIApplication &app) {
        auto& img = addImage(app);
        auto brush = getStdBrush();
        img.setBrush(brush);

        iterate(app);
        simulateMouseClick(app, 100, 100);
        iterate(app);
        simulateMouseClick(app, 200, 200);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "brush_hardness_add.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 300);
}

} // namespace ara::UiUnitTest::PaintImageTests
