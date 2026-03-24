//
// Created by sven on 10-04-25.
//

#include "TestCommon.h"
#include "UIApplication.h"
#include "UIElements/Image.h"

using namespace std;

namespace ara::UiUnitTest::ImageTests {

Image* addImage(const UIApplication &app, const std::string& imageFile, const glm::ivec2& size={400,400},
    const glm::vec4& bgCol = {0.5f, 0.2f, 0.2f, 1.f}, const int mipMap=1) {
    auto& img = app.getMainWindow()->getRootNode()->push<Image>();
    img.setImg(( std::filesystem::path("test") / imageFile).string(), mipMap);
    img.setSize(size.x, size.y);
    img.setBackgroundColor(bgCol.r, bgCol.g, bgCol.b, bgCol.a);
    img.setAlign(align::center, valign::center);
    return &img;
}

TEST(UITest, ImageSingle) {
    appBody([&](const UIApplication &app) {
        addImage(app, "test_img.jpg", {200,200}, {0.f, 0.f, 0.f, 0.f});
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 300);
}

/* mipmap calculations leads to pretty different results depending on hardware and OS
TEST(UITest, ImageSingleLod) {
    Image* img;
    appBody([&](const UIApplication &app) {
        img = addImage(app, "test_img.jpg", {400,400}, {0.f, 0.f, 0.f, 0.f}, 8);
        img->setLod(8);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_lod8.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
        img->setLod(3);

        auto procSteps = app.getMainWindow()->getProcSteps();
        procSteps->at(winProcStep::Draw).active = true;
        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();

        compareFrameBufferToImage(filesystem::current_path() / "image_single_lod3.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);

        img->setLod(0);

        procSteps->at(winProcStep::Draw).active = true;
        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();

        compareFrameBufferToImage(filesystem::current_path() / "image_single_lod0.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 4);
    }, 500, 500);
}
*/

TEST(UITest, ImageSingleFill) {
    appBody([&](const UIApplication &app) {
        addImage(app, "test-tex.png")->setImgFlags(imgFlags::fill);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_fill.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 2);
    }, 500, 500);
}

TEST(UITest, ImageSingleScale) {
    appBody([&](const UIApplication &app) {
        auto img = addImage(app, "test-tex.png");
        img->setImgFlags(imgFlags::scale);
        img->setImgScale(0.3f);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_scale_0_3.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 500, 500);
}

TEST(UITest, ImageSingleHFlip) {
    appBody([&](const UIApplication &app) {
        addImage(app, "test-tex.png")->setImgFlags(imgFlags::hflip);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_hflip.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 500, 500);
}

TEST(UITest, ImageSingleVFlip) {
    appBody([&](const UIApplication &app) {
        addImage(app, "test-tex.png")->setImgFlags(imgFlags::vflip);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_vflip.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 500, 500);
}

TEST(UITest, ImageSingleNoAspect) {
    appBody([&](const UIApplication &app) {
        addImage(app, "test-tex.png")->setImgFlags(imgFlags::noAspect);
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_no_aspect.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 500, 500);
}

TEST(UITest, ImageReloadTest) {
    Image* img = nullptr;
    appBody([&](const UIApplication &app) {
        img = addImage(app, "test_img.jpg", {200,200}, {0.f, 0.f, 0.f, 0.f});

        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();

        compareFrameBufferToImage(filesystem::current_path() / "image_single.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);

        img->setImg(( std::filesystem::path("test") / "test-tex.png").string());
        img->reload();

        app.getWinBase()->draw(0, 0, 0);
        app.getMainWindow()->swap();
    }, [&](const UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_reloaded.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 300);
}

}