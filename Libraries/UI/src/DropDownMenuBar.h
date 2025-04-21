//
// Created by user on 05.10.2020.
//

#pragma once

#include <WindowManagement/GLFWWindow.h>

#include "DropDownMenu.h"

namespace ara {

class DropDownMenuBar : public Div {
public:
    DropDownMenuBar();
    virtual ~DropDownMenuBar() = default;

    void          globalMouseDown(hidData* data);
    DropDownMenu* addDropDownMenu(std::string name);

private:
    std::vector<DropDownMenu*> m_menuEntries;
    int                        m_menuButtSize = 0;
};
}  // namespace ara
