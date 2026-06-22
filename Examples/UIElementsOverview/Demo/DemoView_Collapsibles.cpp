#include "DemoView.h"
#include "UIElements/Menu/TreeCollapsible.h"

using namespace ara;
using namespace glm;
using namespace std;
using namespace nlohmann;

DemoView_Collapsibles::DemoView_Collapsibles() : DemoView("Collapsibles", vec4(.1f,.1f,.1f,1.f)) {
    setName(getTypeName<DemoView_Collapsibles>());
}

void DemoView_Collapsibles::init() {
    push<Label>(LabelPars {
        .pos = ivec2{0,50},
        .size = ivec2 {200, 40},
        .color = vec4{ 1.f, 1.f, 1.f, 1.f },
        .text = "ComboBoxes",
        .textAlignX = align::left,
        .textAlignY = valign::top,
        .fontType = "regular",
        .fontHeight = 20,
    });

    auto pars = UINodePars{
        .pos = ivec2{0,80},
        .size = ivec2 {200, 40},
        .fgColor = vec4{ 1.f, 1.f, 1.f, 1.f },
        .bgColor = vec4{ .1f, .1f, .1f, 1.f },
        .borderWidth = 2,
        .borderRadius = 5,
        .borderColor = m_sharedRes->colors->at(uiColors::blue),
        .padding = vec4{ 5, 5, 5,5 },
    };
    m_combo = &push<ComboBox>(pars);
    m_combo->setMenuName("ComboBox");
    m_combo->setFontType("regular");

    m_combo->addEntry("Entry 1", []{ LOG << " entry one "; });
    m_combo->addEntry("Entry 2", []{ LOG << " entry two "; });
    m_combo->addEntry("Entry 3", []{ LOG << " entry three "; });
    m_combo->addEntry("Entry 4", []{ LOG << " entry four "; });


    push<Label>(LabelPars {
        .pos = ivec2{250,50},
        .size = ivec2 {200, 40},
        .color = vec4{ 1.f, 1.f, 1.f, 1.f },
        .text = "Collapsibles Trees",
        .textAlignX = align::left,
        .textAlignY = valign::top,
        .fontType = "regular",
        .fontHeight = 20,
    });

    // Tree View must use the ara sdk Node class or a derivative
    const std::string str = R"({"children":[{"children":[{"name":"sub1_1_1","uuid":"1"}],"name":"sub1_1","uuid":"0"},{"children":[{"name":"sub1_2_1","uuid":"3"},{"name":"sub1_2_2","uuid":"4"}],"name":"sub1_2","uuid":"2"}],"name":"root","uuid":"10"})";
    m_node.loadFromString(str);

    pars.pos = ivec2{250,80};
    pars.size = ivec2 {200, 160};
    auto& tree = push<TreeCollapsible>(pars);

    tree.setNode(&m_node);

}
