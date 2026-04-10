#include "DemoView.h"
#include <UIElements/Image.h>

using namespace ara;
using namespace glm;
using namespace std;
using listT = std::list<std::string>;
using listT2 = std::vector<int32_t>;

DemoView_ScrollViewList::DemoView_ScrollViewList() : DemoView("Scroll View with a table inside", vec4(.15f,.15f,.15f,1.f)) {
    setName("DemoView_ScrollViewList");
}

void DemoView_ScrollViewList::init() {
    addStringList();
    addVectorInt();
    addPropertyList();
    addCustomUIList();
}

void DemoView_ScrollViewList::addStringList() {
    listT data;
    for (int i=0; i<20; i++) {
        data.emplace_back("Row Nr "+std::to_string(i));
    }

    auto& l1 = push<List<listT>>(UINodePars{
        .size = vec2{ 0.24f, 1.f },
        .style = "list_demo",
        .valign = valign::bottom
    });
    l1.setHeight(-45);
    l1.setRowHeight(45.f);
    l1.setSpacing(0, 5.f);
    l1.set(data);
}

void DemoView_ScrollViewList::addVectorInt() {
    listT2 data2;
    for (int i=0; i<20; i++) {
        data2.emplace_back(i);
    }

    auto& l2 = push<List<listT2>>(UINodePars{
        .pos = vec2{ 0.25f, 0.f },
        .size = vec2{ 0.24f, 1.f },
        .style = "list_demo",
        .valign = valign::bottom
    });
    l2.setHeight(-45);
    l2.setRowHeight(45.f);
    l2.setSpacing(0, 5.f);
    l2.set(data2);
}

void DemoView_ScrollViewList::addPropertyList() {
    for (int i=0; i<20; i++) {
        m_data3.emplace_back("Prop Nr "+std::to_string(i));
    }

    auto& l3 = push<PList<std::string>>(UINodePars{
        .pos = vec2{ 0.5f, 0.f },
        .size = vec2{ 0.24f, 1.f },
        .style = "list_demo",
        .valign = valign::bottom
    });
    l3.setHeight(-45);
    l3.setRowHeight(45.f);
    l3.setSpacing(0, 5.f);
    l3.set(&m_data3);
}

struct customType {
    float floatVal{};
    int32_t intVal{};
    string stringVal{};
};

class CustomListItem : public ListItemBase {
public:
    CustomListItem() {
        setTypeName<CustomListItem>();
        setName(getTypeName<CustomListItem>());
    }

    void init() override {
        Label::init();
        setData(val, m_idx);
    }

    virtual void setData(const customType& data, const int idx) {
        val   = data;
        m_idx = idx;
        setText(data.stringVal+", "+std::to_string(data.intVal)+", "+std::to_string(data.floatVal));
    }

    customType  val{};
    int         m_idx = 0;
};


void DemoView_ScrollViewList::addCustomUIList() {

    vector<customType> data {
        { 0.5f, 1, "Value 1"},
        { 1.5f, 2, "Value 2"},
        { 2.5f, 3, "Value 3"},
    };

    // pass the custom list item type as a second template parameter
    auto& list = push<List< vector<customType>, CustomListItem >>(UINodePars{
        .pos = vec2{ 0.75f, 0.f },
        .size = vec2{ 0.24f, 1.f },
        .style = "list_demo",
        .valign = valign::bottom
    });
    list.setHeight(-45);
    list.setRowHeight(45.f);
    list.setSpacing(0, 5.f);
    list.set(data);
}