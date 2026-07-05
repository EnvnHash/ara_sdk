#include "TestCommon.h"
#include "UIApplication.h"
#include "UIElements/PaintImage.h"

using namespace std;

namespace ara::UiUnitTest::PaintImageTests {

PaintImage& addImageBase(const UIApplication &app) {
    auto& img = app.getMainWindow()->getRootNode()->push<PaintImage>();
    img.setSize(200, 200);
    img.setAlign(align::center, valign::center);
    img.setBackgroundColor(0.f, 0.f, 0.f, 1.f);
    return img;
}

PaintImage& addImage(const UIApplication &app) {
    auto& img = addImageBase(app);
    img.setImg(( std::filesystem::path("test") / "black.png").string(), 1);
    return img;
}

PaintImage::Brush getStdBrush() {
    return PaintImage::Brush {
        .size = 25.f,
        .hardness = 0.2f,
        .color = glm::vec4(1.f, 1.f, 1.f, 1.f),
        .opacity = 1.0f
    };
}

TEST(UITest, PaintImageBasic) {
    appBody([&](const UIApplication &app) {
        auto& img = addImage(app);
        const auto brush = getStdBrush();
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
        simulateMouseClick(app, 249, 249);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "brush_hardness_pos2.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 300);
}

TEST(UITest, PaintImagePAddTest) {
    appBody([&](const UIApplication &app) {
        auto& img = addImage(app);
        const auto brush = getStdBrush();
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

TEST(UITest, PaintImageSubImageTest) {
    appBody([&](const UIApplication &app) {
        auto& img = addImageBase(app);
        // Load a sample Texture of 1024x1024
        img.setImg((filesystem::current_path()  / "1024x1024_black.png").string(), 1);
        img.setSectionPos({50, 50});
        img.setSectionSize({200, 200});

        const auto brush = getStdBrush();
        img.setBrush(brush);
        iterate(app);

        for (auto& it : { glm::ivec2{150,150}, glm::ivec2{50,50}, glm::ivec2{249,249}}) {
            simulateMouseClick(app, it.x, it.y);
            iterate(app);
        }

        img.saveToFile(filesystem::current_path() / "PaintImageSubImageTest.png");
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "brush_subimage.png",
                          app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        compareTwoFiles(filesystem::current_path() / "PaintImageSubImageTest_ref.png", filesystem::current_path() / "PaintImageSubImageTest.png", 3);
    }, 300, 300);
}

} // namespace ara::UiUnitTest::PaintImageTests
