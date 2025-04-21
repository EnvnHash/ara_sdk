//
// Created by user on 27.01.2021.
//

#pragma once

#include "DropDownMenu.h"

namespace ara {

class Image;

class ComboBox : public DropDownMenu {
public:
    ComboBox() : DropDownMenu() { setName(getTypeName<ComboBox>()); }
    ComboBox(std::string&& styleClass) : DropDownMenu(std::move(styleClass)) { setName(getTypeName<ComboBox>()); }
    virtual ~ComboBox() = default;

    virtual void init();
    virtual void open();

    virtual void rebuildEntryList();

    void clearEntries() { m_entries.clear(); }
    void setSelectedEntryName(const std::string& str) { setMenuName(str); }
    void setMaxListEntries(int count) { m_maxListEntries = count; }

private:
    Image* m_downIcon       = nullptr;
    int    m_maxListEntries = 100;
};

}  // namespace ara
