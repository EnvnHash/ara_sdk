#include "DemoView.h"
#include <Image.h>

using namespace ara;
using namespace glm;
using namespace std;
using namespace ara;

DemoView_ScrollView_2::DemoView_ScrollView_2() : DemoView("Scroll View demo",glm::vec4(.15f,.15f,.15f,1.f)) {
    setName(getTypeName<DemoView_ScrollView_2>());
}

UITable* DemoView_ScrollView_2::addTable(UINode* rootNode) {
    auto taux = rootNode->addChild<UITable>();
    taux->setAlignY(valign::bottom);
    taux->setColor(.1f, .1f, .2f, 1.f);
    taux->setBackgroundColor(.0f, .0f, .0f, 1.0f);
    taux->t_setSpacing(10, 10);

    taux->insertRow(-1,1,45,false,true);
    taux->insertRow(-1,1,0,false,false);

    taux->insertColumn(-1,1,100,false,false,50,150);			// column with size limits [50..150]
    taux->insertColumn(-1,1,0);
    taux->insertColumn(-1,1,50);

    return taux;
}

UITable* DemoView_ScrollView_2::addNestedTable(UINode* node) {
    auto nt = dynamic_cast<UITable *>(node->addChild(make_unique<UITable>(0.f, 0.f, 600.f, 200.f, 0, 0)));
    nt->t_setMargins(2, 2);
    nt->t_setSpacing(10, 10);
    nt->setColor(.2f, .2f, .2f, 1.f);
    nt->setBackgroundColor(.1f, .1f, .2f, 1.0f);

    nt->insertColumn(-1,1,80,false,false,50,150);			// column with size limits [50..150]
    nt->insertColumn(-1,1,300,false,false);

    return nt;
}

void DemoView_ScrollView_2::addLabels(UITable* nt) {
    int i;
    glm::vec4 color_text(1.f);

    for (i = 0; i < 20; i++) {
        nt->insertRow(-1, 1, 100, false, false);

        std::stringstream ss;
        ss << std::setw(2) << std::setfill('0') << i;
        auto l = nt->setCell(i, 0, make_unique<Label>() );
        l->setText(ss.str());
        l->setFont("regular", 22,  align::center, valign::center, color_text);
    }
}

void DemoView_ScrollView_2::init() {
    setPadding(10);

    auto taux = addTable(this);

    auto ui_SV =  taux->setCell<ScrollView>(1, 1);
    ui_SV->setPos(0, 0);
    ui_SV->setBackgroundColor(.1f, .1f, .1f, 1.f);

    auto nt = addNestedTable(ui_SV);
    nt->setDynamicWidth(true);
    nt->setDynamicHeight(true);

    addLabels(nt);

    auto l = taux->setCell(0, 1, make_unique<Label>());
    l->setFont("regular", 22,  align::center, valign::center, glm::vec4(1.f));
    l->setBackgroundColor(.1f, .2f, .3f, 1.f);
    l->setText("SCROLL VIEW SAMPLE (Variable size)");
}
