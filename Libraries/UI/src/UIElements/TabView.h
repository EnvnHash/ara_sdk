#pragma once

#include "Table.h"
#include "Button/Button.h"

namespace ara {

class TabView : public Div {
public:
    struct e_tab {
        std::string title{};
        UINode*     ui_Node   = nullptr;
        Button*     tab       = nullptr;
        Div*        underline = nullptr;
        bool        selected  = false;
    };

    TabView();
    ~TabView() override = default;

    void onResize() override;

    template <class T>
    T* addTab(std::string title) {
        return static_cast<T *>(addTab(title, std::make_unique<T>()));
    }

    UINode* addTab(const std::string& title, std::unique_ptr<UINode> uinode);
    void    clearTabs();
    bool    setActivateTab(int idx);
    void    setTabSelected(bool val, e_tab& tab) const;

    virtual void setTabButBgColSelected(glm::vec4 col) { m_tabButtBgColSel = col; }
    virtual void setTabButBgColDeSelected(glm::vec4 col) { m_tabButtBgColDeSel = col; }
    void         setTabSwitchCb(const std::function<void(size_t)>& f) { m_switchTabCb = f; }

protected:
    CellTable<e_tab> m_Tab;

    int m_TabHeight = 40;  // marco.g: again, this should be a value that we get from a global resource
    unsigned int m_selectedTab = 0;

    void arrangeTabs();
    bool m_preventArrangeFdbk = false;

    Div* m_tabArea     = nullptr;
    Div* m_contentArea = nullptr;

    glm::vec2 m_lastSize{0.f};
    glm::vec4 textcolor{1.f};
    glm::vec4 m_tabButtBgColSel{1.f};
    glm::vec4 m_tabButtBgColDeSel{1.f};

    std::function<void(size_t)> m_switchTabCb;
};

}  // namespace ara
