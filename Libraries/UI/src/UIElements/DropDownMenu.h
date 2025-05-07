#pragma once

#include <UIElements/Div.h>

namespace ara {
class Button;

class DropDownMenu : public Div {
public:
    DropDownMenu();
    DropDownMenu(const std::string& styleClass);
    ~DropDownMenu() override;

    void init() override;

    virtual void open();
    virtual void close();

    virtual void mouseDown(hidData& data) override;
    virtual void globalMouseDown(hidData& data);

    void         addEntry(const std::string& name, const std::function<void()>& f) {
        m_entries.emplace_back(name, f);
    }
    virtual void setMenuName(const std::string& str);

protected:
    /** called when the DropDown Menu is opened. It's not needed and not necessary to call this from outside */
    virtual void rebuildEntryList();

    Button*                                                  m_menuEntryButt = nullptr;
    Div*                                                     m_entryList     = nullptr;
    std::string                                              m_menuEntryName;
    std::list<std::pair<std::string, std::function<void()>>> m_entries;
    std::list<Button*>                                       m_entryButts;
    bool                                                     m_open            = false;
    bool                                                     m_closing         = false;
    int                                                      m_listEntryHeight = 30;
};
}  // namespace ara
