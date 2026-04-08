#include "DemoView.h"
#include <UIElements/Image.h>

using namespace ara;
using namespace glm;
using namespace std;
using listT = std::list<std::string>;
using listT2 = std::vector<int32_t>;

DemoView_ScrollViewList::DemoView_ScrollViewList() : DemoView("Scroll View with a table inside", glm::vec4(.15f,.15f,.15f,1.f)) {
    setName("DemoView_ScrollViewList");
}

void DemoView_ScrollViewList::init() {
    listT data;
    for (int i=0; i<20; i++) {
        data.emplace_back("Row Nr "+std::to_string(i));
    }

    auto& l1 = push<List<listT>>(UINodePars{
        .size = vec2{ 0.3f, 1.f },
        .style = "list_demo"
    });
    l1.setRowHeight(45.f);
    l1.setSpacing(0, 5.f);
    l1.set(data);

    listT2 data2;
    for (int i=0; i<20; i++) {
        data2.emplace_back(i);
    }

    auto& l2 = push<List<listT2>>(UINodePars{
        .pos = vec2{ 0.33f, 0.f },
        .size = vec2{ 0.3f, 1.f },
        .style = "list_demo"
    });
    l2.setRowHeight(45.f);
    l2.setSpacing(0, 5.f);
    l2.set(data2);


    for (int i=0; i<20; i++) {
        m_data3.push_back("Prop Nr "+std::to_string(i));
    }

    auto& l3 = push<PList<std::string>>(UINodePars{
        .pos = vec2{ 0.66f, 0.f },
        .size = vec2{ 0.3f, 1.f },
        .style = "list_demo"
    });
    l3.setRowHeight(45.f);
    l3.setSpacing(0, 5.f);
    l3.set(&m_data3);
}
