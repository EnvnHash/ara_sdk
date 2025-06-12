#include "DemoView.h"
#include <UIElements/Image.h>

using namespace ara;
using namespace glm;
using namespace std;

DemoView_ScrollViewList::DemoView_ScrollViewList() : DemoView("Scroll View with a table inside", glm::vec4(.15f,.15f,.15f,1.f)) {
    setName("DemoView_ScrollViewTable");
}

void DemoView_ScrollViewList::init() {
	ui_SV = addChild<ScrollView>(UINodePars{
        .bgColor = vec4{.1f, .1f, .3f, 1.f},
        .padding = vec4{10.f, 10.f, 10.f, 10.f}
    });

    for (int i=0; i<20; i++) {
        m_data.emplace_back("Row Nr "+std::to_string(i));
    }

    m_list = ui_SV->addChild<List<std::string>>();
    m_list->setRowHeight(45.f);
    m_list->setSpacing(0, 5.f);

    getWindow()->addGlCb(this, "rbd", [this]{
        // test list rebuilding
        for (int rep=0; rep<2; rep++) {
            m_list->set(&m_data);
        }

        return true;
    });
}
