//
// Created by sven on 10-04-25.
//

#include "test_common.h"

#include "UIApplication.h"
#include "Image.h"

using namespace std;

namespace ara::SceneGraphUnitTest::ImageTests {

Image* addImage(UIApplication *app, const std::string& imageFile, const glm::ivec2& size={400,400},
    const glm::vec4& bgCol = {0.5f, 0.2f, 0.2f, 1.f}, int mipMap=1) {
    auto rootNode = app->getMainWindow()->getRootNode();
    auto img = rootNode->addChild<Image>();
    img->setImg((filesystem::current_path() / "resdata" / "test" / imageFile).string(), mipMap);
    img->setSize(size.x, size.y);
    img->setBackgroundColor(bgCol.r, bgCol.g, bgCol.b, bgCol.a);
    img->setAlign(align::center, valign::center);
    return img;
}

TEST(UITest, ImageSingle) {
    appBody([&](UIApplication *app) {
        addImage(app, "test_img.jpg", {200,200}, {0.f, 0.f, 0.f, 0.f});
    }, [&](UIApplication *app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 300, 300);
}

TEST(UITest, ImageSingleLod) {
    Image* img;
    appBody([&](UIApplication *app) {
        img = addImage(app, "test_img.jpg", {400,400}, {0.f, 0.f, 0.f, 0.f}, 8);
        img->setLod(8);
    }, [&](UIApplication *app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_lod8.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());

        img->setLod(3);

        auto procSteps = app->getMainWindow()->getProcSteps();
        procSteps->at(winProcStep::Draw).active = true;
        app->getWinBase()->draw(0, 0, 0);
        app->getMainWindow()->swap();

        compareFrameBufferToImage(filesystem::current_path() / "image_single_lod3.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());

        img->setLod(0);

        procSteps->at(winProcStep::Draw).active = true;
        app->getWinBase()->draw(0, 0, 0);
        app->getMainWindow()->swap();

        compareFrameBufferToImage(filesystem::current_path() / "image_single_lod0.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 500, 500);
}

TEST(UITest, ImageSingleFill) {
    appBody([&](UIApplication *app) {
        auto img = addImage(app, "test-tex.png");

        unsigned iflags = 0;
        iflags |= 1;
        img->setImgFlags(iflags);

    }, [&](UIApplication *app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_fill.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 500, 500);
}

TEST(UITest, ImageSingleScale) {
    appBody([&](UIApplication *app) {
        auto img = addImage(app, "test-tex.png");

        unsigned iflags = 0;
        iflags |= 2;
        img->setImgFlags(iflags);
        img->setImgScale(0.3f);

    }, [&](UIApplication *app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_scale_0_3.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 500, 500);
}

TEST(UITest, ImageSingleHFlip) {
    appBody([&](UIApplication *app) {
        auto img = addImage(app, "test-tex.png");

        unsigned iflags = 0;
        iflags |= 4;
        img->setImgFlags(iflags);

    }, [&](UIApplication *app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_hflip.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 500, 500);
}

TEST(UITest, ImageSingleVFlip) {
    appBody([&](UIApplication *app) {
        auto img = addImage(app, "test-tex.png");

        unsigned iflags = 0;
        iflags |= 8;
        img->setImgFlags(iflags);
    }, [&](UIApplication *app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_vflip.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 500, 500);
}

TEST(UITest, ImageSingleNoAspect) {
    appBody([&](UIApplication *app) {
        auto img = addImage(app, "test-tex.png");
        unsigned iflags = 0;
        iflags |= 32;
        img->setImgFlags(iflags);
    }, [&](UIApplication *app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_no_aspect.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 500, 500);
}

}