#pragma once

#include <UIElements/Div.h>

namespace ara {
class Button;

class DropDownMenu : public Div {
public:
    DropDownMenu();
    ~DropDownMenu() override;

    void init() override;

    virtual void open();
    virtual void close();

    void mouseDown(hidData& data) override;
    virtual void globalMouseDown(hidData& data);

    void clearEntries() { m_entries.clear(); }
    void addEntry(const std::string& name, const std::function<void()>& f) {
        m_entries.emplace_back(name, f);
    }
    Button* getEntry(const std::string& name);
    void setEntry(int32_t nr);
    virtual void setMenuName(const std::string& str);
    void setUpdateTitleOnClick(const bool val) { m_updateTitleOnClick = val; }

protected:
    /** Called when the DropDown Menu is opened. It's unnecessary to call this from outside */
    virtual void rebuildEntryList();

    Button*                                                  m_menuEntryButt = nullptr;
    Div*                                                     m_entryList     = nullptr;
    std::string                                              m_menuEntryName;
    std::list<std::pair<std::string, std::function<void()>>> m_entries;
    std::list<Button*>                                       m_entryButts;
    bool                                                     m_open            = false;
    bool                                                     m_updateTitleOnClick = false;
    bool                                                     m_closing         = false;
    int                                                      m_listEntryHeight = 30;
    int32_t                                                  m_currentEntry = -1;
};
}  // namespace ara
