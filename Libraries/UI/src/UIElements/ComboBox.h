//
// Created by user on 27.01.2021.
//

#pragma once

#include "DropDownMenu.h"

namespace ara {

class Image;

class ComboBox : public DropDownMenu {
public:
    ComboBox() { setName(getTypeName<ComboBox>()); }
    explicit ComboBox(const std::string& styleClass) : DropDownMenu(styleClass) { setName(getTypeName<ComboBox>()); }

    ~ComboBox() override = default;

    void init() override;
    void open() override;
    void rebuildEntryList() override;

    void clearEntries() { m_entries.clear(); }
    void setSelectedEntryName(const std::string& str) { setMenuName(str); }
    void setMaxListEntries(int count) { m_maxListEntries = count; }

private:
    Image* m_downIcon       = nullptr;
    int    m_maxListEntries = 100;
};

}  // namespace ara
