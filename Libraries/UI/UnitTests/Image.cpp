//
// Created by sven on 10-04-25.
//

#include "test_common.h"

#include <UIApplication.h>
#include "UI/Image.h"

using namespace std;

namespace ara::GLSceneGraphUnitTest::ImageTests {

TEST(GLSceneGraphTest, ImageSingle) {
    glm::ivec2 buttSize{200, 100};

    appBody([&](UIApplication *app) {
        auto rootNode = app->getMainWindow()->getRootNode();

        auto img = rootNode->addChild<Image>();
        img->setImg((filesystem::current_path() / "resdata" / "FullHD_Pattern.png").string(), 1);
        img->setSize(200, 200);
        img->setAlign(align::center, valign::center);

    }, [&](UIApplication *app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 300, 300);
}

TEST(GLSceneGraphTest, ImageSingleLod) {
    Image* img;

    appBody([&](UIApplication *app) {
        auto rootNode = app->getMainWindow()->getRootNode();
        img = rootNode->addChild<Image>();
        img->setImg((filesystem::current_path() / "resdata" / "FullHD_Pattern.png").string(), 8);
        img->setSize(400, 400);
        img->setAlign(align::center, valign::center);
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

TEST(GLSceneGraphTest, ImageSingleFill) {
    Image* img;

    appBody([&](UIApplication *app) {
        auto rootNode = app->getMainWindow()->getRootNode();
        img = rootNode->addChild<Image>();
        img->setImg((filesystem::current_path() / "resdata" / "test" / "test-tex.png").string());
        img->setSize(400, 400);
        img->setBackgroundColor(0.5f, 0.2f, 0.2f, 1.f);
        img->setAlign(align::center, valign::center);

        unsigned iflags = 0;
        iflags |= 1;
        img->setImgFlags(iflags);

    }, [&](UIApplication *app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_fill.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 500, 500);
}

TEST(GLSceneGraphTest, ImageSingleScale) {
    Image* img;

    appBody([&](UIApplication *app) {
        auto rootNode = app->getMainWindow()->getRootNode();
        img = rootNode->addChild<Image>();
        img->setImg((filesystem::current_path() / "resdata" / "test" / "test-tex.png").string());
        img->setSize(400, 400);
        img->setBackgroundColor(0.5f, 0.2f, 0.2f, 1.f);
        img->setAlign(align::center, valign::center);

        unsigned iflags = 0;
        iflags |= 2;
        img->setImgFlags(iflags);
        img->setImgScale(0.3f);

    }, [&](UIApplication *app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_scale_0_3.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 500, 500);
}

TEST(GLSceneGraphTest, ImageSingleHFlip) {
    appBody([&](UIApplication *app) {
        auto rootNode = app->getMainWindow()->getRootNode();
        auto img = rootNode->addChild<Image>();
        img->setImg((filesystem::current_path() / "resdata" / "test" / "test-tex.png").string());
        img->setSize(400, 400);
        img->setBackgroundColor(0.5f, 0.2f, 0.2f, 1.f);
        img->setAlign(align::center, valign::center);

        unsigned iflags = 0;
        iflags |= 4;
        img->setImgFlags(iflags);

    }, [&](UIApplication *app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_hflip.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 500, 500);
}

TEST(GLSceneGraphTest, ImageSingleVFlip) {
    appBody([&](UIApplication *app) {
        auto rootNode = app->getMainWindow()->getRootNode();
        auto img = rootNode->addChild<Image>();
        img->setImg((filesystem::current_path() / "resdata" / "test" / "test-tex.png").string());
        img->setSize(400, 400);
        img->setBackgroundColor(0.5f, 0.2f, 0.2f, 1.f);
        img->setAlign(align::center, valign::center);

        unsigned iflags = 0;
        iflags |= 8;
        img->setImgFlags(iflags);

    }, [&](UIApplication *app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_vflip.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 500, 500);
}

TEST(GLSceneGraphTest, ImageSingleNoAspect) {
    appBody([&](UIApplication *app) {
        auto rootNode = app->getMainWindow()->getRootNode();
        auto img = rootNode->addChild<Image>();
        img->setImg((filesystem::current_path() / "resdata" / "test" / "test-tex.png").string());
        img->setSize(400, 400);
        img->setBackgroundColor(0.5f, 0.2f, 0.2f, 1.f);
        img->setAlign(align::center, valign::center);

        unsigned iflags = 0;
        iflags |= 32;
        img->setImgFlags(iflags);

    }, [&](UIApplication *app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single_no_aspect.png",
                                  app->getWinBase()->getWidth(), app->getWinBase()->getHeight());
    }, 500, 500);
}

}