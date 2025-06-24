//
// Created by sven on 03-04-25.
//
//

#include "TestCommon.h"

#include "UIApplication.h"
#include <UIElements/Label.h>
#include <Transitions/Carrousel.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::CarrouselTest {

Carrousel* addCarrousel(UIApplication& app, CarrouselMode cm) {
    auto root = app.getRootNode();
    auto caru = root->addChild<Carrousel>();
    caru->setMode(cm);
    caru->showSelector(false);

    std::array bkColor { vec4{1.f, 0.f, 0.f, 1.f}, vec4{0.1f, 0.9f, 0.f, 1.f}, vec4{0.1f, 0.2f, 0.8f, 1.f},
                         vec4{0.7f, 0.3f, 0.2f, 1.f}, vec4{0.6f, 0.4f, 0.3f, 1.f} };
    vec4 col = vec4{0.f, 0.f, 0.f, 1.f};

    for (int i=0; i<5; i++) {
        auto slide = caru->add();
        auto lbl = slide->addChild<Label>();
        lbl->setFont("regular", 80, align::center, valign::center, col);
        lbl->setText(std::to_string(i));
        lbl->setBackgroundColor(bkColor[i]);
    }
    return caru;
}

void swipeLeft(UIApplication& app, float initXPos) {
    app.getMainWindow()->onMouseDownLeft(initXPos, 50, false, false, false);

    for (int i=0;i<20;++i) {
        app.getMainWindow()->onMouseMove(initXPos - i *10, 50, false);
        drawAndSwap(app);
    }

    app.getMainWindow()->onMouseUpLeft();
    for (int i=0;i<35;++i) {
        drawAndSwap(app);
    }
}

TEST(UITest, CarrouselFitOneTest) {
    appBody([&](UIApplication& app){
        addCarrousel(app, CarrouselMode::fitOneSlideOnScreen);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test1_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitOneRotateTest) {
    bool entryOne = false;
    appBody([&](UIApplication& app){
        auto caru = addCarrousel(app, CarrouselMode::fitOneSlideOnScreen);
        caru->show(1, false);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test3_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitOneSwipeTest) {
    bool entryOne = false;
    appBody([&](UIApplication& app){
        addCarrousel(app, CarrouselMode::fitOneSlideOnScreen);
        swipeLeft(app, 300);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test5_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllTest) {
    appBody([&](UIApplication& app){
        addCarrousel(app, CarrouselMode::fitAllOnScreen);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test2_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllRotateTest) {
    bool entryOne = false;
    appBody([&](UIApplication& app){
        auto caru = addCarrousel(app, CarrouselMode::fitAllOnScreen);
        caru->show(1, false);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test4_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllSwipeTest) {
    bool entryOne = false;
    appBody([&](UIApplication& app){
        addCarrousel(app, CarrouselMode::fitAllOnScreen);
        swipeLeft(app, 300);
    }, [&](UIApplication& app){
        //Texture::saveFrontBuffer(filesystem::current_path() / "carrousel_test6_ref.png", app.getWinBase()->getWidth(),
          //                       app.getWinBase()->getHeight(), 4);
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test6_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

}