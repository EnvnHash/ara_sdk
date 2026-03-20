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
    T* addTab(const std::string& title) {
        const UINodePars nodePars{};
        return addTab<T>(title, nodePars);
    }

    template <class T>
    T* addTab(const std::string& title, const UINodePars& nodePars) {
        // be sure there is one row in the table
        if (!m_tab(0).getCount()) {
            m_tab(0).add(1);
        }

        m_tab(1).add(1);

        auto& pushedNode = m_contentArea->push<T>(nodePars);
        pushedNode.setVisibility(m_tab.empty());
        auto& tab = addTabLabelButton(title);
        auto& underline = addTabUnderline(tab);

        // create a Tab Object, with the title, the content node and the Tab-Button
        m_tab.emplace_back(e_tab{title, &pushedNode, &tab, &underline, m_tab.empty()});

        arrangeTabs();
        m_geoChanged = true;
        return &pushedNode;
    }

    //UINode* addTab(const std::string& title, std::shared_ptr<UINode> uinode, UINodePars* nodePars=nullptr);

    Button& addTabLabelButton(const std::string& title);
    Div& addTabUnderline(Button& tab);
    void clearTabs();
    bool setActivateTab(int idx);
    void setTabSelected(bool val, e_tab& tab) const;

    virtual void setTabButBgColSelected(const glm::vec4 col) { m_tabButtBgColSel = col; }
    virtual void setTabButBgColDeSelected(const glm::vec4 col) { m_tabButtBgColDeSel = col; }
    void         setTabSwitchCb(const std::function<void(size_t)>& f) { m_switchTabCb = f; }

protected:
    CellTable<e_tab> m_tab;

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
