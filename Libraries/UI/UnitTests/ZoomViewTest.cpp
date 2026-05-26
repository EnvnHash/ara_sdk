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
bool clicked = false;
constexpr ivec2 winSize = { 600, 600 };
constexpr ivec2 imgButtonStdPos = { 50, 50 };
std::array quadSize { ivec2{100, 100}, ivec2{50, 50} };
std::array quadPos { ivec2{0, 0}, winSize/2 - quadSize[1]/2 };
std::array quadCol { vec4{1.f, 0.f, 0.f, 1.f}, vec4{0.f, 1.f, 0.f, 1.f} };

static ZoomView& addZoomView(const UIApplication& app, const bool showControls, UINode* root=nullptr) {
    ZoomView* zv = nullptr;
    if (root) {
        zv = &root->push<ZoomView>();
    } else {
        zv = &app.getRootNode()->push<ZoomView>();
    }
    zv->setZoomRange(50.f, 600.f);
    zv->keepContentWithinBoundaries(true);
    zv->showSlider(showControls);
    zv->showResetButton(showControls);
    return *zv;
}

static ZoomView& addTestDivs(const UIApplication& app, const bool showControls) {
    auto& zv = addZoomView(app, showControls);
    for (int i=0; i<nrTestQuads; ++i) {
        zv.push<Div>({
            .pos = quadPos[i],
            .size = quadSize[i],
            .bgColor = quadCol[i]
        });
    }
    return zv;
}

static void addImageButton(const UIApplication& app, const ivec2& pos, const ivec2& size) {
    auto& zv = addZoomView(app, false);
    auto& imgBut = zv.push<ImageButton>(UINodePars{
        .pos = pos,
        .size = size,
        .bgColor = vec4{0.36f, 0.36f, 0.36f, 1.f }
    });
    imgBut.setImgAlign(align::center, valign::center);
    imgBut.setImg("Icons/invert_icon.png");
    imgBut.setOnStateImg("Icons/invert_icon_dark.png", 1);
    imgBut.setIsToggle(true);
    imgBut.setClickedCb([&]{
        clicked = true;
    });
}

static void zoomToCenter(const UIApplication& app) {
    app.getMainWindow()->onMouseMove(static_cast<float>(winSize.x) * 0.5f, static_cast<float>(winSize.y)*0.5f, 0);
    const auto mainWin = app.getWinBase()->getWinHandle();
    mainWin->onScroll(0, 2);
}

static void checkZoomedQuad(const UIApplication& app) {
    const auto mainWin = app.getWinBase()->getWinHandle();
    checkQuad(mainWin, quadPos[0]*2, quadSize[0]*2, quadCol[0], {});
    checkQuad(mainWin, quadPos[1]*2, {50,50}, quadCol[1], {});
}

static void checkTestQuads(const UIApplication& app) {
    const auto mainWin = app.getWinBase()->getWinHandle();
    for (int i=0; i<nrTestQuads; ++i) {
        checkQuad(mainWin, quadPos[i], quadSize[i], quadCol[i], {});
    }
}

TEST(UITest, ZoomViewAddContentTest) {
    appBody([&](const UIApplication& app){
        addTestDivs(app, false);
    }, [&](const UIApplication& app){
        checkTestQuads(app);
    }, winSize.x, winSize.y);
}

TEST(UITest, ZoomViewBasicTest) {
    appBody([&](const UIApplication& app){
        auto& zv = addTestDivs(app, false);
        iterate(app);
        zv.setZoom(200.f);
    }, [&](const UIApplication& app){
        checkZoomedQuad(app);
    }, winSize.x, winSize.y);
}

TEST(UITest, ZoomViewWheelTest) {
    appBody([&](const UIApplication& app){
        addTestDivs(app, false);
    }, [&](const UIApplication& app){
        app.getWinBase()->getWinHandle()->onScroll(0, 10);
        iterate(app);
        checkZoomedQuad(app);
    }, winSize.x, winSize.y);
}

TEST(UITest, ZoomViewWheelCenterTest) {
    ZoomView* zv;
    appBody([&](const UIApplication& app){
        zv = &addTestDivs(app, false);
        zoomToCenter(app);
    }, [&](const UIApplication& app){
        const auto mainWin = app.getWinBase()->getWinHandle();
        checkQuad(mainWin, {}, {60,60}, quadCol[0], {});

        const auto zoomFact = zv->getZoomProp()() * 0.01f;
        const auto zoomedSize = vec2(quadSize[1]) * zoomFact;
        checkQuad(mainWin, winSize/2 - ivec2(zoomedSize)/2, zoomedSize, quadCol[1], {});
    }, winSize.x, winSize.y);
}

TEST(UITest, ZoomViewImageButtonTest) {
    appBody([&](const UIApplication& app){
        addImageButton(app, imgButtonStdPos, quadSize[0]);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "zoomview_imagebutton_test.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 2);
    }, winSize.x, winSize.y);
}

TEST(UITest, ZoomViewImageButtonZoomedTest) {
    appBody([&](const UIApplication& app){
        addImageButton(app, imgButtonStdPos, quadSize[0]);
        zoomToCenter(app);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "zoomview_imagebutton_test2.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 2);
    }, winSize.x, winSize.y);
}

TEST(UITest, ZoomViewImageButtonZoomedClickTest) {
    clicked = false;
    appBody([&](const UIApplication& app){
        addImageButton(app, imgButtonStdPos, quadSize[0]);
        zoomToCenter(app);
        iterate(app);
        app.getMainWindow()->onMouseDownLeft(75, 75, false, false, false);
        app.getMainWindow()->onMouseUpLeft();
    }, [&](const UIApplication& app){
        EXPECT_TRUE(clicked);
    }, winSize.x, winSize.y);
}

TEST(UITest, ZoomViewImageButtonZoomedUncenteredClickTest) {
    clicked = false;
    appBody([&](const UIApplication& app){
        const auto mainWin = app.getMainWindow();
        addImageButton(app,
                       { static_cast<int32_t>(winSize.x/2 - quadSize[0].x), static_cast<int32_t>(winSize.y/2 - quadSize[0].y) },
                       ivec2{5, 5});
        mainWin->onMouseMove(static_cast<float>(winSize.x - 20), 20, 0);

        app.getWinBase()->getWinHandle()->onScroll(0, 3);
        iterate(app);
        mainWin->onMouseDownLeft(91, 260, false, false, false);
        mainWin->onMouseUpLeft();
    }, [&](const UIApplication& app){
        EXPECT_TRUE(clicked);
    }, winSize.x, winSize.y);
}

TEST(UITest, ZoomViewExcludeSizeTest) {
    appBody([&](const UIApplication& app){
        auto& zv = addZoomView(app, false);
        iterate(app);
        zoomToCenter(app);
        zv.setZoom(200.f);
        auto& div = zv.push<Div>( {
            .pos = ivec2{0,0},
            .size = ivec2{40,40},
            .bgColor =  vec4(0.f, 0.f, 1.f, 1.f),
            .align = align::center,
            .valign = valign::center,
            .borderWidth = 1,
            .borderColor = vec4(1.f, 1.f, 1.f, 1.f)
        });
        div.setName("test");
        div.excludeSizeFromParentContentTrans(true);
        div.setSnapToHwPix(true);
    }, [&](const UIApplication& app){
        compareFrameBufferToImage(filesystem::current_path() / "zoomview_test_exclude_size_test.png",
                          app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, winSize.x, winSize.y);
}

TEST(UITest, ZoomViewAddContentAligned) {
    appBody([&](const UIApplication& app){
        auto& zv = addZoomView(app, false);
        zv.push<Div>({
            .pos = quadPos[0],
            .size = quadSize[1],
            .bgColor = quadCol[0],
            .align = align::center,
            .valign = valign::center
        });
    }, [&](const UIApplication& app){
        const auto mainWin = app.getWinBase()->getWinHandle();
        checkQuad(mainWin, quadPos[1], quadSize[1], quadCol[0], {});
    }, winSize.x, winSize.y);
}

TEST(UITest, ZoomViewResetTest) {
    appBody([&](const UIApplication& app){
        auto& zv = addZoomView(app, false);
        zv.setCenterAndScaleOnReset(true);
        zv.push<Div>({
            .pos = ivec2{ 30, 30 },
            .size = quadSize[0],
            .bgColor = quadCol[0],
            .align = align::left,
            .valign = valign::top
        });
        iterate(app);
        zv.resetZoom();
    }, [&](const UIApplication& app){
        const auto mainWin = app.getWinBase()->getWinHandle();
        checkQuad(mainWin, ivec2{15, 15}, ivec2{570, 570}, quadCol[0], {});
    }, winSize.x, winSize.y, nullptr, false, "res.txt",100,100);
}

TEST(UITest, ZoomViewMultiDivResetTest) {
    appBody([&](const UIApplication& app){
        auto& zv = addTestDivs(app, false);
        zv.setCenterAndScaleOnReset(true);
        iterate(app);
        zv.resetZoom();
    }, [&](const UIApplication& app){
        const auto mainWin = app.getWinBase()->getWinHandle();
        checkQuad(mainWin, ivec2{15, 15}, ivec2{175, 175}, quadCol[0], {});
        checkQuad(mainWin, ivec2{497, 497}, ivec2{88, 88}, quadCol[1], {});
    }, winSize.x, winSize.y, nullptr, false, "res.txt",100,100);
}

TEST(UITest, ZoomViewMultiDivCenterAlignedResetTest) {
    appBody([&](const UIApplication& app){
        auto& zv = addZoomView(app, false);
        zv.setCenterAndScaleOnReset(true);
        std::array qp { ivec2{-50, -80}, ivec2{ 100, 200} };

        for (int i=0; i<2; ++i) {
            zv.push<Div>({
                .pos = qp[i],
                .size = quadSize[i],
                .bgColor = quadCol[i],
                .align = align::center,
                .valign = valign::center
            });
        }
        iterate(app);
        zv.resetZoom();
    }, [&](const UIApplication& app){
        const auto mainWin = app.getWinBase()->getWinHandle();
        checkQuad(mainWin, ivec2{119, 15}, ivec2{161, 161}, quadCol[0], {});
        checkQuad(mainWin, ivec2{400, 505}, ivec2{81, 80}, quadCol[1], {});
    }, winSize.x, winSize.y, nullptr, false, "res.txt",100,100);
}

}