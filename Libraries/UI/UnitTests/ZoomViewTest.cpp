//
// Created by sven on 26-06-25.
//

#include "TestCommon.h"

#include "UIApplication.h"
#include <UIElements/ZoomView.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::ZoomViewTest {
constexpr int nrTestQuads = 2;
constexpr ivec2 winSize = { 600, 600 };
std::array quadSize { ivec2{100, 100}, ivec2{50, 50} };
std::array quadPos { ivec2{0, 0}, winSize/2 - quadSize[1]/2 };
std::array quadCol { vec4{1.f, 0.f, 0.f, 1.f}, vec4{0.f, 1.f, 0.f, 1.f} };

static ZoomView* addZoomView(UIApplication& app) {
    auto zv = app.getRootNode()->addChild<ZoomView>();
    zv->setZoomRange(50.f, 600.f);
    zv->keepContentWithinBoundaries(true);
    zv->showSlider(false);
    zv->showResetButton(false);
    return zv;
}

static void addTestDivs(ZoomView* zv) {
    for (int i=0; i<nrTestQuads; ++i) {
        zv->addChild<Div>({
            .pos = quadPos[i],
            .size = quadSize[i],
            .bgColor = quadCol[i]
        });
    }
}

static void addImageButton(ZoomView* zv) {
    auto imgBut = zv->addChild<ImageButton>(UINodePars{
            .pos = ivec2{50, 50},
            .size = ivec2{100, 100},
            .bgColor = vec4{0.36f, 0.36f, 0.36f, 1.f }
    });
    imgBut->setImgAlign(align::center, valign::center);
    imgBut->setImg("Icons/invert_icon.png");
    imgBut->setOnStateImg("Icons/invert_icon_dark.png", 1);
    imgBut->setIsToggle(true);
}

TEST(UITest, ZoomViewAddContentTest) {
    appBody([&](UIApplication& app){
        auto zv = addZoomView(app);
        addTestDivs(zv);
    }, [&](UIApplication& app){
        auto mainWin = app.getWinBase()->getWinHandle();
        for (int i=0; i<nrTestQuads; ++i) {
            checkQuad(mainWin, quadPos[i], quadSize[i], quadCol[i], {});
        }
    }, winSize.x, winSize.y);
}

TEST(UITest, ZoomViewBasicTest) {
    appBody([&](UIApplication& app){
        auto zv = addZoomView(app);
        addTestDivs(zv);
        drawAndSwap(app);
        zv->setZoom(200.f);
    }, [&](UIApplication& app){
        auto mainWin = app.getWinBase()->getWinHandle();
        checkQuad(mainWin, quadPos[0]*2, quadSize[0]*2, quadCol[0], {});
        checkQuad(mainWin, quadPos[1]*2, {50,50}, quadCol[1], {});
    }, winSize.x, winSize.y);
}

TEST(UITest, ZoomViewWheelTest) {
    appBody([&](UIApplication& app){
        auto zv = addZoomView(app);
        addTestDivs(zv);
    }, [&](UIApplication& app){
        auto mainWin = app.getWinBase()->getWinHandle();
        mainWin->onScroll(0, 10);
        drawAndSwap(app);
        checkQuad(mainWin, quadPos[0]*2, quadSize[0]*2, quadCol[0], {});
        checkQuad(mainWin, quadPos[1]*2, {50,50}, quadCol[1], {});
    }, winSize.x, winSize.y);
}

TEST(UITest, ZoomViewWheelCenterTest) {
    ZoomView* zv;
    appBody([&](UIApplication& app){
        zv = addZoomView(app);
        addTestDivs(zv);
        app.getMainWindow()->onMouseMove(static_cast<float>(winSize.x/2), static_cast<float>(winSize.y/2), 0);
        auto mainWin = app.getWinBase()->getWinHandle();
        mainWin->onScroll(0, 2);
    }, [&](UIApplication& app){
        auto mainWin = app.getWinBase()->getWinHandle();
        checkQuad(mainWin, {}, {60,60}, quadCol[0], {});

        auto zoomFact = zv->getZoomProp()() * 0.01f;
        auto zoomedSize = vec2(quadSize[1]) * zoomFact;
        checkQuad(mainWin, winSize/2 - ivec2(zoomedSize)/2, zoomedSize, quadCol[1], {});
    }, winSize.x, winSize.y);
}

TEST(UITest, ZoomViewImageButtonTest) {
    appBody([&](UIApplication& app){
        auto zv = addZoomView(app);
        addImageButton(zv);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "zoomview_imagebutton_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 2);
    }, winSize.x, winSize.y);
}

TEST(UITest, ZoomViewImageButtonZoomedTest) {
    appBody([&](UIApplication& app){
        auto zv = addZoomView(app);
        addImageButton(zv);
        app.getMainWindow()->onMouseMove(static_cast<float>(winSize.x/2), static_cast<float>(winSize.y/2), 0);
        auto mainWin = app.getWinBase()->getWinHandle();
        mainWin->onScroll(0, 2);
    }, [&](UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "zoomview_imagebutton_test2.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 2);

//        Texture::saveFrontBuffer("zoomview_imagebutton_test2.png", app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 4);
    }, winSize.x, winSize.y);
}

}