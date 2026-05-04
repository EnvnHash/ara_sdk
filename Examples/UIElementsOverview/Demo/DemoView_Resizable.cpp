//
// Created by sven on 04-05-26.
//

#include "DemoView.h"
#include "UIElements/Resizable.h"

using namespace ara;
using namespace glm;
using namespace std;

DemoView_Resizable::DemoView_Resizable() : DemoView("Resizable Demo",vec4(.15f,.15f,.15f,1.f)) {
    setName(getTypeName<DemoView_Resizable>());
}

void DemoView_Resizable::init() {
    auto& resizable = push<Resizable>({
        .pos = ivec2{0,50},
        .size = vec2{0.5f, 0.5f},
        .align = align::center,
        .valign = valign::center,
        .borderWidth = 1,
        .borderColor = vec4{1.f, 1.f, 1.f, 1.f,},
    });

    auto& img = resizable.push<Image>();
    img.setImg("test/test_img.jpg", 1);
    img.setImgFlags(imgFlags::fill);
}