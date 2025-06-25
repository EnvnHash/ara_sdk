//
// Created by sven on 24-06-25.
//

#include "DemoView.h"
#include "UIElements/ZoomView.h"
#include <UIElements/Image.h>

using namespace ara;
using namespace glm;
using namespace std;

DemoView_ZoomView::DemoView_ZoomView() : DemoView("Zoom view", glm::vec4(.1f,.1f,.1f,1.f)) {
    setName(getTypeName<DemoView_ZoomView>());
}

void DemoView_ZoomView::init() {
    auto zv = addChild<ZoomView>();
    zv->setSize(1.f, -40);
    zv->setAlignY(valign::bottom);
    zv->setBorderColor(1.f, 1.f, 1.f, 0.2f);
    zv->setBorderWidth(1);
    zv->setZoomRange(100.f, 600.f);
    zv->keepContentWithinBoundaries(true);

    auto img = zv->addChild<Image>();
    img->setImg("test/test_img.jpg", 1);
    img->setImgFlags(imgFlags::fill);
}