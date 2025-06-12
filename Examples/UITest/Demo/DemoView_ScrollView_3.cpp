#include "DemoView.h"
#include <UIElements/Image.h>

using namespace ara;
using namespace glm;
using namespace std;

DemoView_ScrollView_3::DemoView_ScrollView_3() : DemoView("Scroll View demo / Horizontal arrange",glm::vec4(.15f,.15f,.15f,1.f)) {
    setName(getTypeName<DemoView_ScrollView_3>());
}

void DemoView_ScrollView_3::init() {
    ui_SV = addChild<ScrollView>(UINodePars{
        .pos = ivec2{ 0, 80 },
        .fgColor = vec4{ .1f, .1f, .1f, 1.f },
        .padding = vec4{ 10.f, 10.f, 10.f, 10.f }
    });
    ui_SV->setHeight(240);

    for (int i=0; i<8; i++) {
        auto unit = ui_SV->addChild<DemoView_ScrollView_3::Unit>(UINodePars{
            .size = ivec2{200, 200},
            .fgColor = vec4{.8f,.8f,.6f,1.f},
            .bgColor = vec4{ .2f,.2f,.5f,1.f},
            .alignY = valign::center,
        });
        std::stringstream ss;
        ss << "Item "<< std::fixed << std::setprecision(2) << i+1;
        unit->m_Title = ss.str();
        unit->setX(i * (200+10));
    }
}

void DemoView_ScrollView_3::Unit::init() {
    setPadding(10);

    addChild<Label>(LabelInitData{
        .pos = {0, 0},
        .size = {180, 24},
        .alignY = valign::top,
        .text_color = getColor(),
        .bg_color = {.1f, .1f, .2f, 1.f},
        .text = m_Title,
        .text_align_x = align::center,
        .text_align_y = valign::center,
        .font_type = "bold",
        .font_height = 22
    });

    addChild<Image>(UINodePars{
        .size = ivec2{110, 110},
        .alignX = align::center,
        .alignY = valign::center
    })->setImg(std::rand() & 1 ? "trigrid.png" : "FullHD_Pattern.png",1);

    addChild<Label>(LabelInitData{
        .pos = {0, 0},
        .size = {180, 24},
        .alignX = align::center,
        .alignY = valign::bottom,
        .text_color = {.4f, .4f, .4f, 1.f},
        .bg_color = getBackgroundColor(),
        .text = "More text here",
        .text_align_x = align::center,
        .text_align_y = valign::center,
        .font_type = "regular",
        .font_height = 22
    });

}
