//
// Created by sven on 24-06-25.
//

#include "DemoView.h"
#include "UIElements/ZoomView.h"

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
    zv->setBorderColor(1.f, 1.f, 1.f, 1.f);
    zv->setBackgroundColor(0.2f, 0.2f, 0.2f, 0.4f);

    for (int i=0; i<30; i++) {
        zv->addChild<Div>({
            .pos = vec2{getRandF(0.f, 1.f), getRandF(0.f, 1.f)},
            .size = ivec2{ static_cast<int>(getRandF(10.f, 80.f)), static_cast<int>(getRandF(10.f, 80.f)) },
            .bgColor = vec4{ getRandF(0.f, 1.f), getRandF(0.f, 1.f), getRandF(0.f, 1.f), 1.f }
        });
    }
}