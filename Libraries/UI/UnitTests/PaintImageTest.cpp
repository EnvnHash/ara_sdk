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
    img.setImg((filesystem::current_path() / "black.png").string(), 1);
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

TEST(UITest, PaintImageSubtrTest) {
    appBody([&](const UIApplication &app) {
        auto& img = addImageBase(app);
        img.setImg((filesystem::current_path() / "white.png").string(), 1);

        const auto brush = getStdBrush();
        img.setBrush(brush);
        img.setMode(PaintImage::mode::subtract);

        iterate(app);
        simulateMouseClick(app, 150, 150);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "brush_hardness_subtr.png",
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
        GLBaseUnitTest::compareTwoFiles(filesystem::current_path() / "PaintImageSubImageTest_ref.png",
            filesystem::current_path() / "PaintImageSubImageTest.png", 3);
    }, 300, 300);
}

TEST(UITest, PaintImageUndoRedoSequence) {
    appBody([&](const UIApplication &app) {
        auto& img = addImage(app);
        auto brush = getStdBrush();
        brush.hardness = 1.0f;
        img.setBrush(brush);
        iterate(app);

        // Sequence: paint, paint, undo, undo, clear, paint, paint, undo, undo, undo
        // Initial state has 1 backup copy (initial state)
        EXPECT_EQ(img.getBackupCopies().size(), 1);

        // Undoing at initial state should be safely ignored
        img.undo();
        EXPECT_EQ(img.getBackupCopies().size(), 1);

        // paint 1: at (100, 100)
        simulateMouseClick(app, 100, 100);
        img.saveState();
        iterate(app);
        EXPECT_EQ(img.getBackupCopies().size(), 2);

        // paint 2: at (200, 200)
        simulateMouseClick(app, 200, 200);
        img.saveState();
        iterate(app);
        EXPECT_EQ(img.getBackupCopies().size(), 3);

        // undo 1
        img.undo();
        iterate(app);

        // undo 2
        img.undo();
        iterate(app);

        // extra undo at boundary
        img.undo();
        iterate(app);

        // clear
        img.clear();
        iterate(app);
        // S1 and S2 should have been erased, S_clear added: [S0, S_clear]
        EXPECT_EQ(img.getBackupCopies().size(), 2);

        // paint 1 (after clear): at (100, 100)
        simulateMouseClick(app, 100, 100);
        img.saveState();
        iterate(app);
        EXPECT_EQ(img.getBackupCopies().size(), 3);

        // paint 2 (after clear): at (200, 200)
        simulateMouseClick(app, 200, 200);
        img.saveState();
        iterate(app);
        EXPECT_EQ(img.getBackupCopies().size(), 4);

        // undo 1: undoes paint 2
        img.undo();
        iterate(app);

        // undo 2: undoes paint 1
        img.undo();
        iterate(app);

        // undo 3: undoes clear (restores S0)
        img.undo();
        iterate(app);

        // extra undo should be safe
        img.undo();
        iterate(app);
    }, [&](const UIApplication &app) {
        // Since all operations are undone back to initial state, image at (100, 100) and (200, 200) should be black
        auto pixels1 = getPixels(100, 100, 1, 1);
        EXPECT_EQ(pixels1[0], 0);
        EXPECT_EQ(pixels1[1], 0);
        EXPECT_EQ(pixels1[2], 0);

        auto pixels2 = getPixels(200, 200, 1, 1);
        EXPECT_EQ(pixels2[0], 0);
        EXPECT_EQ(pixels2[1], 0);
        EXPECT_EQ(pixels2[2], 0);
    }, 300, 300);
}

TEST(UITest, PaintImageRedoAndHistoryBranch) {
    appBody([&](const UIApplication &app) {
        auto& img = addImage(app);
        auto brush = getStdBrush();
        brush.hardness = 1.0f;
        img.setBrush(brush);
        iterate(app);

        // Initial state: redo should do nothing
        img.redo();
        EXPECT_EQ(img.getBackupCopies().size(), 1);

        // Paint at (150, 150)
        simulateMouseClick(app, 150, 150);
        img.saveState();
        iterate(app);
        EXPECT_EQ(img.getBackupCopies().size(), 2);

        // Undo
        img.undo();
        iterate(app);

        // Redo
        img.redo();
        iterate(app);

        // Redo again beyond end should do nothing
        img.redo();
        iterate(app);
    }, [&](const UIApplication &app) {
        // Redone state should have white pixel at center
        auto pixels = getPixels(150, 150, 1, 1);
        EXPECT_EQ(pixels[0], 255);
        EXPECT_EQ(pixels[1], 255);
        EXPECT_EQ(pixels[2], 255);
    }, 300, 300);
}

} // namespace ara::UiUnitTest::PaintImageTests
