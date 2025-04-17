#include "DemoView.h"
#include "../../../../Libraries/GLSceneGraph/src/UI/Image.h"

using namespace ara;
using namespace glm;
using namespace std;
using namespace ara;

DemoView_ScrollView::DemoView_ScrollView() : DemoView("Scroll View demo / Many elements exceeding the view's boundaries",glm::vec4(.15f,.15f,.15f,1.f))
{
    setName(getTypeName<DemoView_ScrollView>());
}

void DemoView_ScrollView::init()
{
	ui_SV = addChild<ScrollView>();
	ui_SV->setPadding(10.f);
    ui_SV->setBackgroundColor(.1f, .1f, .1f, 1.f);

	// Add a bunch of childs (as images in this case) that will exceed the size of the view

	ui_SV->addChild<Image>(0, 0, 300, 300, nullptr, nullptr)->setImg("Icons/layer1_on.png",1);
	ui_SV->addChild<Image>(120, 200+20, 300, 300, nullptr, nullptr)->setImg("Icons/layer2_on.png",1);
    ui_SV->addChild<Image>(400, 500, 600, 600, nullptr, nullptr)->setImg("Icons/cam_icon.jpg",1);

	std::srand((unsigned) std::time(nullptr));

	for (int i=0; i<16; i++)
		for (int j=0; j<12; j++)
			ui_SV->addChild<Image>(120+j*130, 1200+i*130, 110, 110, nullptr, nullptr)->setImg(std::rand()&1 ? "Icons/layer1_on.png" : "Icons/layer2_on.png",1);
}
