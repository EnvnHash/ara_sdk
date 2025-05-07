//
// Created by user on 05.10.2020.
//

#pragma once

#include "DropDownMenu.h"

namespace ara {

class DropDownMenuBar : public Div {
public:
    DropDownMenuBar();
    ~DropDownMenuBar() override = default;

    void          globalMouseDown(hidData& data) const;
    DropDownMenu* addDropDownMenu(const std::string& name);

private:
    std::vector<DropDownMenu*> m_menuEntries;
    int                        m_menuButtSize = 0;
};
}  // namespace ara
