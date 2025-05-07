//
// Created by user on 08.08.2021.
//

#pragma once
/*
#include <List.h>

namespace ara
{

class CollapseListItem : public ListItem
{
public:

};

class CollapseList : public List
{
public:
    CollapseList();
    CollapseList(const std::string& styleClass);
    virtual ~CollapseList()=default;

    void rebuildList();

    inline std::vector<ListItem>::iterator addItem(std::string name,
std::function<void(hidData&)> cb, bool rebuild=true) {
        m_items.emplace_back(CollapseListItem{name, cb});
        if (rebuild) rebuildList();
        return m_items.end()-1;
    }

protected:
};

}
*/