#include "DemoView.h"
#include <Image.h>

using namespace ara;
using namespace glm;
using namespace std;

DemoView_ScrollView_3::DemoView_ScrollView_3() : DemoView("Scroll View demo / Horizontal arrange",glm::vec4(.15f,.15f,.15f,1.f)) {
    setName(getTypeName<DemoView_ScrollView_3>());
}

void DemoView_ScrollView_3::init() {
    ui_SV = addChild<ScrollView>();
    ui_SV->setY(80);
    ui_SV->setHeight(240);
    ui_SV->setBackgroundColor(.1f, .1f, .1f, 1.f);
    ui_SV->setPadding(10.f);

    char se[256];

    for (int i=0; i<8; i++) {
        auto unit = ui_SV->addChild<DemoView_ScrollView_3::Unit>();
        sprintf(se,"Item %02d",i+1);
        unit->m_Title = se;
        unit->setX(i * (200+10));
        unit->setAlignY(valign::center);
        unit->setSize(200, 200);
        unit->setColor(.8f,.8f,.6f,1.f);
        unit->setBackgroundColor(.2f,.2f,.5f,1.f);
    }
}

void DemoView_ScrollView_3::Unit::init() {
    setPadding(10);

    addChild(make_unique<Label>(0, 0, 180, 24, m_fontColor, glm::vec4(.1f, .1f, .2f, 1.f), m_Title,
                                align::center, valign::center, "bold",22));

    auto img = addChild<Image>();
    img->setSize(110, 110);
    img->setImg(std::rand()&1 ? "trigrid.png" : "FullHD_Pattern.png",1);
    img->setAlign(align::center, valign::center);

    auto l2=(Label*) addChild( make_unique<Label>(0, 0, 180, 24, glm::vec4(.4f, .4f, .4f, 1.f), getBackgroundColor(), "More text here",
                                                  align::center, valign::center, "regular",22));
    l2->setAlign(align::center, valign::bottom);

}
