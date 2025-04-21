#include "DemoView.h"

using namespace ara;
using namespace glm;
using namespace std;
using namespace ara;

DemoView_ComboBox::DemoView_ComboBox() : DemoView("ComboBoxes",glm::vec4(.1f,.1f,.1f,1.f)) {
    setName(getTypeName<DemoView_ComboBox>());
}

void DemoView_ComboBox::init() {
    m_combo =  addChild<ComboBox>();
    m_combo->setMenuName("ComboBox");
    m_combo->setPos(0,50);
    m_combo->setSize(200,40);
    m_combo->setFontType("regular");
    m_combo->setPadding(5.f);
    m_combo->setBackgroundColor(.1f, .1f, .1f, 1.f);
    m_combo->setBorderRadius(5);
    m_combo->setBorderWidth(2);
    m_combo->setBorderColor(m_sharedRes->colors->at(uiColors::blue));
    m_combo->setColor(1.f, 1.f, 1.f, 1.f);

    m_combo->addEntry("Entry 1", []{ LOG << " entry one "; });
    m_combo->addEntry("Entry 2", []{ LOG << " entry two "; });
    m_combo->addEntry("Entry 3", []{ LOG << " entry three "; });
    m_combo->addEntry("Entry 4", []{ LOG << " entry four "; });
}
