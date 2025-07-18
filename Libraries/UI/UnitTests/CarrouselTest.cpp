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

Carrousel* addCarrousel(UIApplication& app, CarrouselMode cm, int32_t spacing, int32_t padding) {
    auto root = app.getRootNode();
    auto caru = root->addChild<Carrousel>({
        .padding = vec4{padding, padding, padding, padding}
    });
    caru->setMode(cm);
    caru->showSelector(false);
    caru->setSpacing(spacing);
    if (cm == CarrouselMode::leftAlign) {
        caru->showArrows(true);
    }

    std::array bkColor { vec4{1.f, 0.f, 0.f, 1.f}, vec4{0.1f, 0.9f, 0.f, 1.f}, vec4{0.1f, 0.2f, 0.8f, 1.f},
                         vec4{0.7f, 0.3f, 0.2f, 1.f}, vec4{0.6f, 0.4f, 0.3f, 1.f} };
    vec4 col = vec4{0.f, 0.f, 0.f, 1.f};
    std::array<int32_t, 5> slideWidth { 100, 160, 80, 200, 120 };

    for (int i=0; i<5; i++) {
        auto slide = caru->add();
        if (cm == CarrouselMode::leftAlign) {
            slide->setWidth(slideWidth[i]);
        }

        auto lbl = slide->addChild<Label>();
        lbl->setFont("regular", 80, align::center, valign::center, col);
        lbl->setText(std::to_string(i));
        lbl->setBackgroundColor(bkColor[i]);
    }
    return caru;
}

static void simulateClickLeft(UIApplication& app, /*ImageButton* butt,*/ const glm::vec2& pos, bool expVal) {
    auto mainWin = app.getMainWindow();
    mainWin->onMouseDownLeft(pos.x, pos.y, false, false, false);
    drawAndSwap(app);

    mainWin->onMouseUpLeft();

    for (int i=0;i<35;++i) {
        drawAndSwap(app);
    }
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
        addCarrousel(app, CarrouselMode::fitOneSlideOnScreen, 0, 0);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test1_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitOneTestWithSpacing) {
    appBody([&](UIApplication& app){
        addCarrousel(app, CarrouselMode::fitOneSlideOnScreen, 20, 0);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test1_space.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitOneTestWithSpacingAndPadding) {
    appBody([&](UIApplication& app){
        addCarrousel(app, CarrouselMode::fitOneSlideOnScreen, 20, 10);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test1_space_padding.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitOneRotateTest) {
    bool entryOne = false;
    appBody([&](UIApplication& app){
        auto caru = addCarrousel(app, CarrouselMode::fitOneSlideOnScreen, 0, 0);
        caru->show(1, false);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test3_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitOneRotateWithSpacingTest) {
    appBody([&](UIApplication& app){
        auto caru = addCarrousel(app, CarrouselMode::fitOneSlideOnScreen, 20, 0);
        caru->show(1, false);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test3_spacing_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitOneRotateWithSpacingAndPaddingTest) {
    appBody([&](UIApplication& app){
        auto caru = addCarrousel(app, CarrouselMode::fitOneSlideOnScreen, 20, 10);
        caru->show(1, false);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test3_spacing_padding_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitOneSwipeTest) {
    appBody([&](UIApplication& app){
        addCarrousel(app, CarrouselMode::fitOneSlideOnScreen, 0, 0);
        swipeLeft(app, 300);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test5_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllTest) {
    appBody([&](UIApplication& app){
        addCarrousel(app, CarrouselMode::fitAllOnScreen, 0, 0);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test2_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllWithSpacingTest) {
    appBody([&](UIApplication& app){
        addCarrousel(app, CarrouselMode::fitAllOnScreen, 20, 0);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test2_spacing_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllWithSpacingAndPaddingTest) {
    appBody([&](UIApplication& app){
        addCarrousel(app, CarrouselMode::fitAllOnScreen, 20, 10);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test2_spacing_padding_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllRotateTest) {
    appBody([&](UIApplication& app){
        auto caru = addCarrousel(app, CarrouselMode::fitAllOnScreen, 0, 0);
        caru->show(1, false);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test4_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllRotateWithSpacingTest) {
    appBody([&](UIApplication& app){
        auto caru = addCarrousel(app, CarrouselMode::fitAllOnScreen, 20, 0);
        caru->show(1, false);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test4_spacing_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllRotateWithSpacingAndPaddingTest) {
    appBody([&](UIApplication& app){
        auto caru = addCarrousel(app, CarrouselMode::fitAllOnScreen, 20, 0);
        caru->show(1, false);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test4_spacing_padding_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllSwipeTest) {
    appBody([&](UIApplication& app){
        addCarrousel(app, CarrouselMode::fitAllOnScreen, 0, 0);
        swipeLeft(app, 300);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test6_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselLeftAlignTest) {
    appBody([&](UIApplication& app){
        addCarrousel(app, CarrouselMode::leftAlign, 0, 0);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_leftAlign.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselLeftAlignWithSpacingTest) {
    appBody([&](UIApplication& app){
        addCarrousel(app, CarrouselMode::leftAlign, 20, 0);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_leftAlign_spacing.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselLeftAlignWithSpacingPaddingTest) {
    appBody([&](UIApplication& app){
        addCarrousel(app, CarrouselMode::leftAlign, 20, 10);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_leftAlign_spacing_padding.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

/*
TEST(UITest, CarrouselLeftAlignClickCountUpTest) {
    appBody([&](UIApplication& app){
        addCarrousel(app, CarrouselMode::leftAlign, 10, 0);
        simulateClickLeft(app, vec2{578, 198}, true);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_leftAlign_right_click.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselLeftAlignSwipeTest) {
    appBody([&](UIApplication& app){
        addCarrousel(app, CarrouselMode::leftAlign, 0, 0);
        swipeLeft(app, 370);
    }, [&](UIApplication& app){
        Texture::saveFrontBuffer(filesystem::current_path() / "carrousel_leftAlign_swipe.png", app.getWinBase()->getWidth(),
                                 app.getWinBase()->getHeight(), 4);
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_leftAlign_swipe.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}
*/
}