//
// Created by sven on 03-04-25.
//
//

#include "TestCommon.h"

#include "UIApplication.h"
#include <UIElements/Text/Label.h>
#include <Transitions/Carrousel.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::CarrouselTest {

Carrousel* addCarrousel(const UIApplication& app, const CarrouselMode cm, const int32_t spacing, const int32_t padding) {
    const auto root = app.getRootNode();
    auto& carrousel = root->push<Carrousel>({
        .padding = vec4{padding, padding, padding, padding}
    });
    carrousel.setMode(cm);
    carrousel.showSelector(false);
    carrousel.setSpacing(spacing);
    if (cm == CarrouselMode::leftAlign) {
        carrousel.showArrows(true);
    }

    constexpr std::array bgcolor { vec4{1.f, 0.f, 0.f, 1.f},
        vec4{0.1f, 0.9f, 0.f, 1.f},
        vec4{0.1f, 0.2f, 0.8f, 1.f},
        vec4{0.7f, 0.3f, 0.2f, 1.f},
        vec4{0.6f, 0.4f, 0.3f, 1.f} };

    constexpr auto col = vec4{0.f, 0.f, 0.f, 1.f};

    for (int i=0; i<5; i++) {
        const auto slide = carrousel.add();
        if (cm == CarrouselMode::leftAlign) {
            constexpr std::array slideWidth { 100, 160, 80, 200, 120 };
            slide->setWidth(slideWidth[i]);
            slide->setName("slide_"+std::to_string(i));
        }

        auto& lbl = slide->push<Label>();
        lbl.setFont("regular", 80, align::center, valign::center, col);
        lbl.setText(std::to_string(i));
        lbl.setBackgroundColor(bgcolor[i]);
    }
    return &carrousel;
}

static void simulateClickLeft(UIApplication& app, /*ImageButton* butt,*/ const vec2& pos, bool expVal) {
    const auto mainWin = app.getMainWindow();
    mainWin->onMouseDownLeft(pos.x, pos.y, false, false, false);
    drawAndSwap(app);

    mainWin->onMouseUpLeft();

    for (int i=0;i<35;++i) {
        drawAndSwap(app);
    }
}

void swipeLeft(const UIApplication& app, const float initXPos) {
    app.getMainWindow()->onMouseDownLeft(initXPos, 50, false, false, false);

    for (int i=0;i<20;++i) {
        app.getMainWindow()->onMouseMove(initXPos - static_cast<float>(i) *10, 50, false);
        drawAndSwap(app);
    }

    app.getMainWindow()->onMouseUpLeft();
    for (int i=0;i<35;++i) {
        drawAndSwap(app);
    }
}

TEST(UITest, CarrouselFitOneTest) {
    appBody([&](const UIApplication& app){
        addCarrousel(app, CarrouselMode::fitOneSlideOnScreen, 0, 0);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test1_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitOneTestWithSpacing) {
    appBody([&](const UIApplication& app){
        addCarrousel(app, CarrouselMode::fitOneSlideOnScreen, 20, 0);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test1_space.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitOneTestWithSpacingAndPadding) {
    appBody([&](const UIApplication& app){
        addCarrousel(app, CarrouselMode::fitOneSlideOnScreen, 20, 10);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test1_space_padding.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitOneRotateTest) {
    bool entryOne = false;
    appBody([&](const UIApplication& app){
        const auto carrousel = addCarrousel(app, CarrouselMode::fitOneSlideOnScreen, 0, 0);
        carrousel->show(1, false);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test3_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitOneRotateWithSpacingTest) {
    appBody([&](const UIApplication& app){
        const auto carrousel = addCarrousel(app, CarrouselMode::fitOneSlideOnScreen, 20, 0);
        carrousel->show(1, false);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test3_spacing_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitOneRotateWithSpacingAndPaddingTest) {
    appBody([&](const UIApplication& app){
        const auto carrousel = addCarrousel(app, CarrouselMode::fitOneSlideOnScreen, 20, 10);
        carrousel->show(1, false);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test3_spacing_padding_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitOneSwipeTest) {
    appBody([&](const UIApplication& app){
        addCarrousel(app, CarrouselMode::fitOneSlideOnScreen, 0, 0);
        swipeLeft(app, 300);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test5_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllTest) {
    appBody([&](const UIApplication& app){
        addCarrousel(app, CarrouselMode::fitAllOnScreen, 0, 0);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test2_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllWithSpacingTest) {
    appBody([&](const UIApplication& app){
        addCarrousel(app, CarrouselMode::fitAllOnScreen, 20, 0);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test2_spacing_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllWithSpacingAndPaddingTest) {
    appBody([&](const UIApplication& app){
        addCarrousel(app, CarrouselMode::fitAllOnScreen, 20, 10);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test2_spacing_padding_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllRotateTest) {
    appBody([&](const UIApplication& app){
        const auto caru = addCarrousel(app, CarrouselMode::fitAllOnScreen, 0, 0);
        caru->show(1, false);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test4_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllRotateWithSpacingTest) {
    appBody([&](const UIApplication& app){
        const auto caru = addCarrousel(app, CarrouselMode::fitAllOnScreen, 20, 0);
        caru->show(1, false);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test4_spacing_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllRotateWithSpacingAndPaddingTest) {
    appBody([&](const UIApplication& app){
        const auto caru = addCarrousel(app, CarrouselMode::fitAllOnScreen, 20, 0);
        caru->show(1, false);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test4_spacing_padding_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselFitAllSwipeTest) {
    appBody([&](const UIApplication& app){
        addCarrousel(app, CarrouselMode::fitAllOnScreen, 0, 0);
        swipeLeft(app, 300);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_test6_ref.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselLeftAlignTest) {
    appBody([&](const UIApplication& app){
        addCarrousel(app, CarrouselMode::leftAlign, 0, 0);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_leftAlign.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselLeftAlignWithSpacingTest) {
    appBody([&](const UIApplication& app){
        addCarrousel(app, CarrouselMode::leftAlign, 20, 0);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_leftAlign_spacing.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselLeftAlignWithSpacingPaddingTest) {
    appBody([&](const UIApplication& app){
        addCarrousel(app, CarrouselMode::leftAlign, 20, 10);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_leftAlign_spacing_padding.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

/*
TEST(UITest, CarrouselLeftAlignClickCountUpTest) {
    appBody([&](const UIApplication& app){
        addCarrousel(app, CarrouselMode::leftAlign, 10, 0);
        simulateClickLeft(app, vec2{578, 198}, true);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_leftAlign_right_click.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}

TEST(UITest, CarrouselLeftAlignSwipeTest) {
    appBody([&](const UIApplication& app){
        addCarrousel(app, CarrouselMode::leftAlign, 0, 0);
        swipeLeft(app, 370);
    }, [&](const UIApplication& app){
        Texture::saveFrontBuffer(filesystem::current_path() / "carrousel_leftAlign_swipe.png", app.getWinBase()->getWidth(),
                                 app.getWinBase()->getHeight(), 4);
        compareFrameBufferToImage(filesystem::current_path() / "carrousel_leftAlign_swipe.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 1);
    }, 600, 400);
}
*/
}