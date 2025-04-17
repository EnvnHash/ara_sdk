#include "DemoView.h"

using namespace ara;
using namespace glm;
using namespace std;
using namespace ara;

DemoView::DemoView(std::string title, glm::vec4 bk_color) : Div()
{
    setName(getTypeName<DemoView>());
    setBackgroundColor(bk_color);
    setPadding(20);

    m_label = addChild<Label>(0,0,400,60);
    m_label->setFont("regular", 28, align::left, valign::top, m_textcolor);
    m_label->setText(title);

    m_canReceiveDrag=true;
}


