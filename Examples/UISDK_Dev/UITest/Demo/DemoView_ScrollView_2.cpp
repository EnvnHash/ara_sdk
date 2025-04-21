#include "DemoView.h"
#include <Image.h>

using namespace ara;
using namespace glm;
using namespace std;
using namespace ara;

DemoView_ScrollView_2::DemoView_ScrollView_2() : DemoView("Scroll View demo",glm::vec4(.15f,.15f,.15f,1.f)) {
    setName(getTypeName<DemoView_ScrollView_2>());
}

void DemoView_ScrollView_2::init() {
    setPadding(10);

    auto taux = addChild<UITable>();
    taux->setSize(1.f, -100);
    taux->setAlignY(valign::bottom);
    taux->t_setSpacing(8, 8);
    taux->t_setMargins(0, 0);
    taux->setColor(.2f, 0.2f, 0.2f, 1.f);
    taux->setBackgroundColor(.0f, .0f, .0f, 1.0f);

    taux->insertRow(-1,1,40,false,true);						// fixed row
    taux->insertRow(-1,1,0,false,false);
    taux->insertRow(-1,1,40,false,false);

    taux->insertColumn(-1,1,100,false,false,50,150);			// column with size limits [50..150]
    taux->insertColumn(-1,1,0);
    taux->insertColumn(-1,1,50);

    ui_SV =  taux->setCell<ScrollView>(1, 1);
    ui_SV->setPos(0, 0);
    ui_SV->setBackgroundColor(.1f, .1f, .1f, 1.f);

    auto nt = (UITable*) ui_SV->addChild(make_unique<UITable>(0.f, 0.f, 600.f, 200.f, 0, 0));
    nt->t_setSpacing(8, 8);
    nt->t_setMargins(2, 2);
    nt->setColor(.2f, .2f, .2f, 1.f);
    nt->setBackgroundColor(.1f, .1f, .2f, 1.0f);

    nt->insertColumn(-1,1,80,false,false,50,150);			// column with size limits [50..150]
    nt->insertColumn(-1,1,300,false,false);
    nt->insertColumn(-1,1,25);

    int i;
    glm::vec4 color_bg(.1f,.2f,.3f,1.f);
    glm::vec4 color_text(1.f);

    for (i = 0; i < 20; i++) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << i;

        nt->insertRow(-1, 1, 100, false, false);						// fixed row

        auto l = nt->setCell(i, 0, make_unique<Label>() );
        l->setFont("regular", 22,  align::center, valign::center, color_text);
        l->setBackgroundColor(color_bg);
        l->setText(ss.str());
    }

    nt->setDynamicWidth(true);
    nt->setDynamicHeight(true);

    auto l = taux->setCell(0, 1, make_unique<Label>());
    l->setFont("regular", 22,  align::center, valign::center, glm::vec4(1.f));
    l->setBackgroundColor(.1f, .2f, .3f, 1.f);
    l->setText("SCROLL VIEW SAMPLE (Variable size)");
}
