#include "DemoView.h"
#include "../../../../Libraries/GLSceneGraph/src/UI/Image.h"

using namespace ara;
using namespace glm;
using namespace std;
using namespace ara;

DemoView_ScrollViewList::DemoView_ScrollViewList() : DemoView("Scroll View with a table inside", glm::vec4(.15f,.15f,.15f,1.f))
{
    setName("DemoView_ScrollViewTable");
}

void DemoView_ScrollViewList::init()
{
	ui_SV = addChild<ScrollView>();
	ui_SV->setPadding(10.f);
    ui_SV->setBackgroundColor(.1f, .1f, .3f, 1.f);

    int nrRows = 20;
    for (int i=0; i<nrRows; i++)
        m_data.emplace_back("Row Nr "+std::to_string(i));

    m_list = ui_SV->addChild<List<std::string>>();
    m_list->setRowHeight(45.f);
    m_list->setSpacing(0, 5.f);

    getWindow()->addGlCb(this, "rbd", [this, nrRows]{
        // test list rebuilding
        for (int rep=0; rep<2; rep++)
            m_list->set(&m_data);

        return true;
    });
}
