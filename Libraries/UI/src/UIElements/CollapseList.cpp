//
// Created by user on 08.08.2021.
//
/*
#include "CollapseList.h"

using namespace std;

namespace ara
{

CollapseList::CollapseList() : List()
{
    setName(getTypeName<CollapseList>());
}

CollapseList::CollapseList(const std::string& styleClass) :
List(styleClass)
{
    setName(getTypeName<CollapseList>());
}

void CollapseList::rebuildList()
{
    if (!m_table) return;

    m_table->clearCells();
    m_table->insertColumn(-1, 1, 1.f, true);
    m_table->insertRow(-1, (int)m_items.size(), m_rowHeight);

    int i=0;
    for (auto &li : m_items)
    {
        li.label = m_table->setCell<Label>(i, 0);
        li.label->setText(m_items[i].text);
        li.label->addMouseClickCb([li](hidData* data) { if (li.cb) li.cb(data);
}); if (!getStyleClass().empty())
            li.label->addStyleClass(getStyleClass()+".label");
        i++;
    }

    setDrawFlag();
}

}*/