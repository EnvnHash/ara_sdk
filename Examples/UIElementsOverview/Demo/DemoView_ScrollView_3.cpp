#include "DemoView.h"
#include <UIElements/Image.h>

using namespace ara;
using namespace glm;
using namespace std;

DemoView_ScrollView_3::DemoView_ScrollView_3() : DemoView("Scroll View demo / Horizontal arrange",vec4(.15f,.15f,.15f,1.f)) {
    setName(getTypeName<DemoView_ScrollView_3>());
}

void DemoView_ScrollView_3::init() {
    ui_SV = &push<ScrollView>(UINodePars{
        .pos = ivec2{ 0, 80 },
        .fgColor = vec4{ .1f, .1f, .1f, 1.f },
        .padding = vec4{ 10.f, 10.f, 10.f, 10.f }
    });
    ui_SV->setHeight(240);

    for (int i=0; i<8; i++) {
        auto& unit = ui_SV->push<Unit>(UINodePars{
            .size = ivec2{200, 200},
            .fgColor = vec4{.8f,.8f,.6f,1.f},
            .bgColor = vec4{ .2f,.2f,.5f,1.f},
            .valign = valign::center,
        });
        std::stringstream ss;
        ss << "Item "<< std::fixed << std::setprecision(2) << i+1;
        unit.m_Title = ss.str();
        unit.setX(i * (200+10));
    }
}

void DemoView_ScrollView_3::Unit::init() {
    setPadding(10.f);

    push<Label>(
        UINodePars{
            .pos = ivec2{0, 0},
            .size = ivec2{180, 24},
            .fgColor = getColor(),
            .bgColor = vec4{.1f, .1f, .2f, 1.f},
            .valign = valign::top,
        },
        LabelPars{

            .text = m_Title,
            .textAlignX = align::center,
            .textAlignY = valign::center,
            .fontType = "bold",
            .fontHeight = 22
        });

    push<Image>({
        .size = ivec2{110, 110},
        .align = align::center,
        .valign = valign::center
    }).setImg(std::rand() & 1 ? "trigrid.png" : "FullHD_Pattern.png",1);

    push<Label>(
        UINodePars{
            .pos = ivec2{0, 0},
            .size = ivec2{180, 24},
            .fgColor = vec4{.4f, .4f, .4f, 1.f},
            .bgColor = getBackgroundColor(),
            .align = align::center,
            .valign = valign::bottom,
        },
        LabelPars{
            .text = "More text here",
            .textAlignX = align::center,
            .textAlignY = valign::center,
            .fontType = "regular",
            .fontHeight = 22
        }
    );

}
