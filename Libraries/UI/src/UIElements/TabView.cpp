#include "TabView.h"
#include "UIApplication.h"



using namespace glm;
using namespace std;

namespace ara {

TabView::TabView() {
    setTypeName<TabView>();
    setName(getTypeName<TabView>());
    m_canReceiveDrag = true;
    setFocusAllowed(false);

    // add a container for the tabs
    m_tabArea = &push<Div>();
    m_tabArea->setName("TabArea");
    m_tabArea->setHeight(m_TabHeight);

    m_contentArea = &push<Div>();
    m_contentArea->setHeight(-m_TabHeight);
    m_contentArea->setAlignY(valign::bottom);

    m_tabButtBgColDeSel = vec4{.2f, .2f, .2f, 1.0f};
    m_tabButtBgColSel   = vec4{.2f, .2f, .2f, 1.0f};
}

Button& TabView::addTabLabelButton(const std::string& title) {
    auto& tab = m_tabArea->push<Button>({ .style = getStyleClass()+".button"});
    tab.setBackgroundColor(m_tabButtBgColDeSel);
    tab.setFont("regular", 17, align::left, valign::center, textcolor);
    tab.setPadding(5.f, 0.f, 5.f, 0.f);
    tab.setText(title);
    tab.setTextAlignX(align::center);
    tab.setTextAlignY(valign::center);
    tab.addMouseClickCb([this](const hidData& data) {
        const auto ret = ranges::find_if(m_tabArea->children(), [&data](auto& item){
            return std::dynamic_pointer_cast<UINode>(item)->getId() == data.objId;
        });
        if (ret != m_tabArea->children().end()) {
            setActivateTab(static_cast<int32_t>(std::distance( m_tabArea->children().begin(), ret)));
            setDrawFlag();
            getSharedRes()->reqRedraw();
        }
    });
    return tab;
}

Div& TabView::addTabUnderline(Button& tab)  {
    auto& underline = tab.push<Div>({ .style = getStyleClass()+".button"});
    underline.setName("underline");
    underline.setHeight(1);
    underline.setAlign(align::center, valign::bottom);
    underline.setBackgroundColor(m_sharedRes->colors->at(uiColors::blue));
    underline.excludeFromPadding(true);
    underline.excludeFromObjMap(true);
    return underline;
}

void TabView::onResize() {
    // arrangeTabs sets m_geoChanged which causes an updateMatrix calls, which
    // causes an onResize() calls. prevent this feedback with a flag
    if (!m_preventArrangeFdbk) {
        arrangeTabs();
    } else {
        m_preventArrangeFdbk = false;
    }
}

void TabView::clearTabs() {
    m_tab.reset();
    m_contentArea->clearChildren();
    m_tabArea->clearChildren();
}

void TabView::arrangeTabs() {
    m_tab.updateGeo(m_tabArea->getSize().x, m_tabArea->getSize().y, 0, 0, 0, 0, 5, 0);

    int            i = 0;
    Table_CellGeo cg;

    for (auto&[title, ui_Node, tab, underline, selected] : m_tab) {
        if (m_tab.getCellGeo(cg, i++)) {
            if (cg.pixSize[0] > 0) {
                tab->setPos(static_cast<int>(cg.pixPos[0]), static_cast<int>(cg.pixPos[1]));
                tab->setSize(static_cast<int>(cg.pixSize[0]), static_cast<int>(cg.pixSize[1]));
                tab->setBackgroundColor(selected ? m_tabButtBgColSel : m_tabButtBgColDeSel);
            }
        }
    }

    m_geoChanged         = true;
    m_preventArrangeFdbk = true;
}

bool TabView::setActivateTab(const int idx) {
    if (idx < 0 || idx >= m_tab.getCellCount()) {
        return false;
    }

    m_selectedTab = idx;

    // deselect all
    for (auto& it : m_tab) {
        setTabSelected(false, it);
    }

    // select the request tab
    setTabSelected(true, m_tab[idx]);

    if (m_switchTabCb) {
        m_switchTabCb(idx);
    }

    return true;
}

void TabView::setTabSelected(const bool val, e_tab& tab) const {
    tab.selected = val;
    tab.tab->setColor(m_sharedRes->colors->at(val ? uiColors::blue : uiColors::white));
    tab.ui_Node->setVisibility(val);
    tab.underline->setVisibility(val);
}

UINode* TabView::getTabByTitle(const std::string& str) {
    if (const auto r = std::ranges::find_if(m_tab, [&str](auto& tab) { return tab.title == str; }); r != m_tab.end()) {
        return r->ui_Node;
    }
    return nullptr;
}

int32_t TabView::getTabIndexByTitle(const std::string& str) {
    if (const auto r = std::ranges::find_if(m_tab, [&str](auto& tab) { return tab.title == str; }); r != m_tab.end()) {
        return static_cast<int32_t>(std::distance(m_tab.begin(), r));
    }
    return -1;
}

}