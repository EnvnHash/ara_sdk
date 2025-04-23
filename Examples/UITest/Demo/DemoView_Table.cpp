#include "DemoView.h"

using namespace glm;
using namespace std;
using namespace ara;

DemoView_Table::DemoView_Table() : DemoView("Table demo",glm::vec4(.1f,.1f,.1f,1.f)) {
    setName(getTypeName<DemoView_Table>());
}

void DemoView_Table::init() {
    ui_Table = dynamic_cast<UITable*>(addChild(make_unique<UITable>(0.f, 100.f, getContentSize().x, getContentSize().y - 100.f, 4, 3)));
    ui_Table->t_setSpacing(8, 8);
    ui_Table->t_setMargins(5, 5);

    for (int i = 0; i < ui_Table->getRowCount(); i++) {
        for (int j = 0; j < ui_Table->getColumnCount(); j++) {
            auto label = ui_Table->setCell<Label>(i, j);
            label->setFont("regular", 22, align::left, valign::top, m_textColor);
            label->setBackgroundColor(.2f, .2f, .3f, 1.0f);
            label->setText("(" + std::to_string(i) + "," + std::to_string(j) + ")");
            label->setPadding(12);
        }
    }
}
