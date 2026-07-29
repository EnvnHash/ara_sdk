//
// Created by user on 27.01.2021.
//

#pragma once

#include "DropDownMenu.h"

namespace ara {

class Image;

class ComboBox : public DropDownMenu {
public:
    ComboBox() { setTypeName<ComboBox>(); setName(getTypeName<ComboBox>()); }
    ~ComboBox() override = default;

    void init() override;
    void open() override;

    void setSelectedEntryName(const std::string& str) { setMenuName(str); }
    void setMaxListEntries(const int count) { m_maxListEntries = count; }

protected:
    void rebuildEntryList() override;

private:
    int    m_maxListEntries = 100;
};

}  // namespace ara
