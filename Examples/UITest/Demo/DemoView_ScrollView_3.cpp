#include "DemoView.h"
#include <Image.h>
#include <Utils/PingPongFbo.h>

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

    for (int i=0; i<8; i++) {
        auto unit = ui_SV->addChild<DemoView_ScrollView_3::Unit>();
        std::stringstream ss;
        ss << "Item "<< std::fixed << std::setprecision(2) << i+1;
        unit->m_Title = ss.str();
        unit->setX(i * (200+10));
        unit->setAlignY(valign::center);
        unit->setSize(200, 200);
        unit->setColor(.8f,.8f,.6f,1.f);
        unit->setBackgroundColor(.2f,.2f,.5f,1.f);
    }
}

void DemoView_ScrollView_3::Unit::init() {
    setPadding(10);

    addChild(make_unique<Label>(LabelInitData{
        .pos = {0, 0},
        .size = {180, 24},
        .text_color = m_fontColor,
        .bg_color = {.1f, .1f, .2f, 1.f},
        .text = m_Title,
        .ax = align::center,
        .ay = valign::center,
        .font_type = "bold",
        .font_height = 22
    }));

    auto img = addChild<Image>();
    img->setSize(110, 110);
    img->setImg(std::rand() & 1 ? "trigrid.png" : "FullHD_Pattern.png",1);
    img->setAlign(align::center, valign::center);

    auto l2 = dynamic_cast<Label *>(addChild(make_unique<Label>(LabelInitData{
        .pos = {0, 0},
        .size = {180, 24},
        .text_color = {.4f, .4f, .4f, 1.f},
        .bg_color = getBackgroundColor(),
        .text = "More text here",
        .ax = align::center,
        .ay = valign::center,
        .font_type = "regular",
        .font_height = 22
    })));

    l2->setAlign(align::center, valign::bottom);

}
