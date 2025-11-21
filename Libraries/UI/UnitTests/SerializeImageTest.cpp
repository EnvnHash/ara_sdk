//
// Created by sven on 17-11-25.
//

#include "TestCommon.h"
#include <UIElements/Image.h>
#include <UINodeFactory.h>

using namespace std;
using namespace glm;

namespace ara::UiUnitTest::ImageTest {

Image& addImage(UIApplication &app, const std::string& imageFile, const ivec2& size={400,400},
    const vec4& bgCol = {0.5f, 0.2f, 0.2f, 1.f}, int mipMap=1) {
    auto& img = app.getMainWindow()->getRootNode()->push<Image>();
    img.setImg(( std::filesystem::path("test") / imageFile).string(), mipMap);
    img.setSize(size.x, size.y);
    img.setBackgroundColor(bgCol.r, bgCol.g, bgCol.b, bgCol.a);
    img.setAlign(align::center, valign::center);
    return img;
}

TEST(UITest, SerializeImageSaveAndReload) {
    registerDefaultUITypes();

    appBody([&](UIApplication &app) {
        auto& img = addImage(app, "test_img.jpg", {200,200}, {0.f, 0.f, 0.f, 0.f});
        auto root = app.getMainWindow()->getRootNode();
        root->saveAs("test.json");
        root->remove(img);
        root->load("test.json");
    }, [&](UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 300);
}


TEST(UITest, SerializeLoadImageTest) {
    registerDefaultUITypes();

    appBody([&](UIApplication &app) {
        auto root = app.getMainWindow()->getRootNode();
        root->load("SerializeLoadImage.json");
    }, [&](UIApplication &app) {
        compareFrameBufferToImage(filesystem::current_path() / "image_single.png",
                                  app.getWinBase()->getWidth(), app.getWinBase()->getHeight(), 3);
    }, 300, 300);
}


}