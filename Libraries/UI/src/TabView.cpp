#include "TabView.h"

#include "UIApplication.h"

using namespace glm;
using namespace std;

namespace ara {

TabView::TabView() : Div() {
    setName(getTypeName<TabView>());
    m_canReceiveDrag = true;
    setFocusAllowed(false);

    // add a container for the tabs
    m_tabArea = addChild<Div>();
    m_tabArea->setName("TabArea");
    m_tabArea->setHeight(m_TabHeight);

    m_contentArea = addChild<Div>();
    m_contentArea->setHeight(-m_TabHeight);
    m_contentArea->setAlignY(valign::bottom);

    // m_tabButtBgColSel = glm::vec4(.5f, .5f, .5f, 1.0f);
    m_tabButtBgColDeSel = glm::vec4(.2f, .2f, .2f, 1.0f);
    m_tabButtBgColSel   = glm::vec4(.2f, .2f, .2f, 1.0f);
}

UINode* TabView::addTab(const std::string& title, std::unique_ptr<UINode> uinode) {
    // be sure there is one row in the table
    if (!m_Tab(0).getCount()) {
        m_Tab(0).add(1);
    }

    m_Tab(1).add(1);

    if (uinode) {
        // add content
        auto pushedNode = m_contentArea->addChild(std::move(uinode));  // implicitly sets shared res from parent (this)
        pushedNode->setVisibility(m_Tab.empty());

        // add a Tab-Button
        auto tab = m_tabArea->addChild<Button>();
        tab->setFont("regular", 17, align::left, valign::center, textcolor);
        tab->setPadding(5.f, 0.f, 5.f, 0.f);
        tab->setText(title);
        tab->setTextAlignX(align::center);
        tab->setTextAlignY(valign::center);
        tab->addMouseClickCb([this, tab, title](hidData* data) {
            for (int i = 0; i < (int)m_tabArea->getChildren().size(); i++) {
                if (m_tabArea->getChildren()[i]->getId() == data->objId) {
                    setActivateTab(i);
                    break;
                }
            }
            setDrawFlag();
        });

        // tab underline
        auto underline = tab->addChild<Div>();
        underline->setName("underline");
        underline->setHeight(1);
        underline->setAlign(align::center, valign::bottom);
        underline->setBackgroundColor(m_sharedRes->colors->at(uiColors::blue));
        underline->excludeFromPadding(true);
        underline->excludeFromObjMap(true);
        underline->setVisibility(false);

        // create a Tab Object, with the title, the content node and the Tab-Button
        m_Tab.emplace_back(e_tab{title, pushedNode, tab, underline, m_Tab.empty()});

        arrangeTabs();
        m_geoChanged = true;
        return pushedNode;
    }

    return nullptr;
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
    m_Tab.reset();
    m_contentArea->clearChildren();
    m_tabArea->clearChildren();
}

void TabView::arrangeTabs() {
    m_Tab.updateGeo(m_tabArea->getSize().x, m_tabArea->getSize().y, 0, 0, 0, 0, 5, 0);

    int            i = 0;
    eTable_CellGeo cg;

    for (e_tab& t : m_Tab) {
        if (m_Tab.getCellGeo(cg, i++)) {
            if (cg.pixSize[0] > 0) {
                t.tab->setPos(static_cast<int>(cg.pixPos[0]), static_cast<int>(cg.pixPos[1]));
                t.tab->setSize(static_cast<int>(cg.pixSize[0]), static_cast<int>(cg.pixSize[1]));
                t.tab->setBackgroundColor(t.selected ? m_tabButtBgColSel : m_tabButtBgColDeSel);
            }
        }
    }

    m_geoChanged         = true;
    m_preventArrangeFdbk = true;
}

bool TabView::setActivateTab(int idx) {
    if (idx < 0 || idx >= m_Tab.getCellCount()) {
        return false;
    }

    m_selectedTab = idx;

    // deselect all
    for (auto& it : m_Tab) {
        setTabSelected(false, &it);
    }

    // select the request tab
    setTabSelected(true, &m_Tab[idx]);

    if (m_switchTabCb) {
        m_switchTabCb(idx);
    }

    return true;
}

void TabView::setTabSelected(bool val, e_tab* tab) {
    tab->selected = val;
    tab->tab->setColor(m_sharedRes->colors->at(val ? uiColors::blue : uiColors::white));
    tab->ui_Node->setVisibility(val);
    tab->underline->setVisibility(val);
}

}