#include "DemoView.h"

using namespace ara;
using namespace glm;
using namespace std;
using namespace ara;


DemoView_Table::DemoView_Table() : DemoView("Table demo",glm::vec4(.1f,.1f,.1f,1.f))
{
    setName(getTypeName<DemoView_Table>());
}

void DemoView_Table::init()
{
    ui_Table = (UITable*) addChild(make_unique<UITable>(0.f, 100.f, getContentSize().x, getContentSize().y - 100.f, 4, 3));
    ui_Table->t_setSpacing(8, 8);
    ui_Table->t_setMargins(5, 5);

    Label* l;
    for (int i = 0; i < ui_Table->getRowCount(); i++)

        for (int j = 0; j < ui_Table->getColumnCount(); j++) {

            l=ui_Table->setCell<Label>(i, j);
            l->setFont("regular", 22, align::left, valign::top, m_textColor);
            l->setBackgroundColor(.3f, .3f, .4f, 1.0f);
            l->setText("(" + std::to_string(i) + "," + std::to_string(j) + ")");
            l->setPadding(10);

        }

}
