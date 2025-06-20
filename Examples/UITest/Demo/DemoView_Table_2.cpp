#include "DemoView.h"

using namespace ara;
using namespace glm;
using namespace std;

DemoView_Table_2::DemoView_Table_2() : DemoView("Table demo 2 / Table in table / fixed row / row size limits",glm::vec4(.1f,.1f,.1f,1.f)) {
    setName(getTypeName<DemoView_Table_2>());
}

void DemoView_Table_2::init() {
    ui_Table = addChild<UITable>(UITableParameters{
        .pos = ivec2{ 0.f, 100 },
        .size = ivec2{ getContentSize().x, getContentSize().y - 100 },
        .spacing = ivec2{ 8, 8 }
    });

    ui_Table->setColor(.2f, 0.2f, 0.2f, 1.f);
    ui_Table->setBackgroundColor(.0f, .0f, .0f, 1.0f);

    ui_Table->insertRow(-1,1,40,false,true);						// fixed row
    ui_Table->insertRow(-1,1,0,false,false);
    ui_Table->insertRow(-1,1,40,false,false);

    ui_Table->insertColumn(-1,1,100,false,false,50,150);			// column with size limits [50..150]
    ui_Table->insertColumn(-1,1,0);
    ui_Table->insertColumn(-1,1,50);

    auto taux = ui_Table->setCell<UITable>(1, 1,
                                           make_unique<UITable>(UITableParameters{ .size = vec2{300.f, 200.f},
                                                                                   .topology = ivec2{3, 3}}));
    taux->setSize(1.f, 1.f);
    taux->setSpacing(8, 8);
    taux->setMargins(0, 0);
    taux->setColor(.4f, .4f, 0.6f, 1.f);
    taux->setBackgroundColor(.2f, .2f, .2f, 1.0f);

    Label* l =  ui_Table->setCell<Label>(0, 1);
    l->setColor(1.f, 1.f, 0.8f, 1.f);
    l->setBackgroundColor(.3f, .3f, .4f, 1.0f);
    l->setPadding(10.f, 0.f, 0.f, 0.f);
    l->setText("TABLE TITLE SAMPLE (FIXED ROW)");
    l->setTextAlignY(valign::center);

}
