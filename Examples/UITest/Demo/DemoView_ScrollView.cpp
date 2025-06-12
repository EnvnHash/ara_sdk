#include "DemoView.h"
#include <UIElements/Image.h>

using namespace ara;
using namespace glm;
using namespace std;

DemoView_ScrollView::DemoView_ScrollView() : DemoView("Scroll View demo / Many elements exceeding the view's boundaries",glm::vec4(.15f,.15f,.15f,1.f)) {
    setName(getTypeName<DemoView_ScrollView>());
}

void DemoView_ScrollView::init() {
	ui_SV = addChild<ScrollView>(UINodePars{
        .bgColor = vec4{.1f, .1f, .1f, 1.f},
        .padding = vec4{10.f, 10.f, 10.f, 10.f}
    });

	// Add a bunch of children (as images in this case) that will exceed the size of the view
	ui_SV->addChild<Image>({
        .size = ivec2{300, 300}
    })->setImg("trigrid.png", 1);

    ui_SV->addChild<Image>({
        .pos = ivec2{120, 200+20},
        .size = ivec2{300, 300}
    })->setImg("checkerboard_small.png", 1);

    ui_SV->addChild<Image>({
        .pos = ivec2{400, 500},
        .size = ivec2{600, 600}
    })->setImg("test/test_img.jpg", 1);

	std::srand(static_cast<unsigned>(std::time(nullptr)));

	for (int i=0; i<16; i++) {
        for (int j=0; j<12; j++) {
            ui_SV->addChild<Image>({
                .pos = ivec2{120 + j * 130, 1200 + i * 130},
                .size = ivec2{110, 110}
            })->setImg(std::rand()&1 ? "Icons/icon-arrow-down.png" : "Icons/MenuBar/restore.png",1);
        }
    }
}
