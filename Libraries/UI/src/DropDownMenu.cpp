#include "DropDownMenu.h"

#include <Windows/WindowsMousePos.h>

#include "Button/Button.h"
#include "UIWindow.h"

using namespace glm;
using namespace std;

#ifdef ARA_USE_GLFW
using namespace ara::mouse;
#endif

namespace ara {

DropDownMenu::DropDownMenu() : Div() {
    setName(getTypeName<DropDownMenu>());
    setFocusAllowed(false);
    loadStyleDefaults();
}

DropDownMenu::DropDownMenu(std::string&& styleClass) : Div(std::move(styleClass)) {
    setName(getTypeName<DropDownMenu>());
    setFocusAllowed(false);
    loadStyleDefaults();
}

DropDownMenu::~DropDownMenu() {
#ifdef ARA_USE_GLFW
    getWinMan()->removeGlobalMouseButtonCb(this);
    auto uiWin = static_cast<UIWindow*>(getWindow());
    if (uiWin) {
        uiWin->removeGlobalMouseDownLeftCb(this);
        uiWin->removeGlobalMouseDownRightCb(this);
    }
#endif
}

void DropDownMenu::init() {
    // create the button in the MenuBar which by clicked will expand the EntryList
    m_menuEntryButt = addChild<Button>();
    m_menuEntryButt->setText(m_menuEntryName);
    m_menuEntryButt->setTextAlignX(align::left);
    m_menuEntryButt->setBorderWidth(1);
    m_menuEntryButt->setPadding(m_sharedRes->padding, 0.f, 0.f, 0.f);
    m_menuEntryButt->setFont("regular", 20, align::left, valign::center, m_sharedRes->colors->at(uiColors::font));
    m_menuEntryButt->setBackgroundColor(0.f, 0.f, 0.f, 0.f);

    // selected
    m_menuEntryButt->setColor(m_sharedRes->colors->at(uiColors::black), state::selected);
    m_menuEntryButt->setBackgroundColor(m_sharedRes->colors->at(uiColors::highlight), state::selected);

    m_listEntryHeight = m_sharedRes->gridSize.y;

    // global MouseDown callbacks for closing the DropDownMenu
    auto uiWin = static_cast<UIWindow*>(getWindow());

    // be sure the callback exits only once
    uiWin->removeGlobalMouseDownLeftCb(this);
    uiWin->addGlobalMouseDownLeftCb(this, [this](hidData* data) { globalMouseDown(data); });
    uiWin->addGlobalMouseDownRightCb(this, [this](hidData* data) { globalMouseDown(data); });

    // add a callback to listen for clicks also outside the window bounds
    // be sure the callback exits only once
#ifdef ARA_USE_GLFW
    getWinMan()->removeGlobalMouseButtonCb(this);
    getWinMan()->addGlobalMouseButtonCb(this, [this](GLFWwindow* win, int button, int action, int mods) {
        if (!m_open || m_closing) {
            return;
        }

        auto uiWin = static_cast<UIWindow*>(getWindow());
        if (!uiWin) {
            return;
        }

        int x, y;
        getAbsMousePos(x, y);

        if (x < uiWin->getPosition().x || x > (uiWin->getPosition().x + uiWin->getSize().x) ||
            y < uiWin->getPosition().y || y > (uiWin->getPosition().y + uiWin->getSize().y)) {
            uiWin->addGlCb(this, "closeDDM", [this]() {
                close();
                return true;
            });
            uiWin->update();  // force the gl thread to iterate
            m_closing = true;
        }
    });
#endif

    if (!getStyleClass().empty()) {
        if (m_menuEntryButt) {
            m_menuEntryButt->addStyleClass(getStyleClass() + ".actEntry");
        }

        if (m_entryList) {
            for (auto& butt : m_entryButts) {
                butt->addStyleClass(getStyleClass() + ".entries");
            }
        }
    }
}

void DropDownMenu::open() {
    if (m_open) {
        return;
    }

    m_open  = true;
    m_state = state::selected;
    m_menuEntryButt->setSelected(true);

    // create on top of all nodes
    m_entryList = getRoot()->addChild<Div>(getStyleClass() + ".list");
    m_entryList->setName("DDM_EntryList");
    m_entryList->setPos((int)m_menuEntryButt->getWinPos().x,
                        (int)(m_menuEntryButt->getWinPos().y + m_sharedRes->gridSize.y));
    m_entryList->setSize(200, (int)m_entries.size() * m_listEntryHeight);
    m_entryList->setBackgroundColor(m_sharedRes->colors->at(uiColors::background));
    m_entryList->setBorderWidth(1);
    m_entryList->setBorderColor(m_sharedRes->colors->at(uiColors::sepLine));
    m_entryList->setScissorChildren(true);

    rebuildEntryList();
}

void DropDownMenu::close() {
    m_state = state::none;
    m_menuEntryButt->setSelected(false);
    m_closing = false;

    if (m_open && m_entryList) {
        m_open = false;
        m_entryButts.clear();
        getRoot()->remove_child(m_entryList);

        m_entryList = nullptr;
    }
}

void DropDownMenu::mouseDown(hidData* data) {
    open();
    data->consumed = true;
}

void DropDownMenu::globalMouseDown(hidData* data) {
    // close the menu if it is open and the user clicked somewhere outside the menu
    bool isChild          = (uint32_t)data->objId >= getId() && (uint32_t)data->objId <= getMaxChildId();
    bool isEntryListChild = m_entryList && (uint32_t)data->objId >= m_entryList->getId() &&
                            (uint32_t)data->objId <= m_entryList->getMaxChildId();

    if (isInited() && m_open && !isChild && !isEntryListChild) {
        close();
        if (getWindow()) {
            getWindow()->update();
        }
    }
}

void DropDownMenu::rebuildEntryList() {
    if (!m_entryList) {
        return;
    }

    m_entryButts.clear();

    int i = 0;
    for (auto& entry : m_entries) {
        auto butt = m_entryList->addChild<Button>();
        m_entryButts.push_back(butt);  // maintain a separate list of entry button since m_entries
                                       // may contain other elements

        // default
        butt->setName("Button_" + entry.first);
        butt->setText(entry.first);
        butt->setTextAlignX(align::left);
        butt->setColor(m_sharedRes->colors->at(uiColors::font));
        butt->setSize(1.f, m_listEntryHeight);
        butt->setPos(0, m_listEntryHeight * i);
        butt->setBorderWidth(0);
        butt->setBackgroundColor(0.f, 0.f, 0.f, 0.f);
        butt->setFont("regular", 18, align::left, valign::center, butt->getSharedRes()->colors->at(uiColors::font));
        butt->setPadding(m_sharedRes->padding);
        butt->setAlign(align::left, valign::top);

        butt->setClickedCb([this, entry] {
            entry.second();
            // push to window gl callbacks which are processed after next
            // iteration in order to have the mouseUp still find this node
            if (getWindow()) {
                getWindow()->addGlCb(this, "close", [this] {
                    close();
                    return true;
                });
                m_sharedRes->requestRedraw = true;
            }
        });

        // highlight
        butt->setBorderWidth(1, state::highlighted);
        butt->setBorderColor(m_sharedRes->colors->at(uiColors::sepLine), state::highlighted);
        butt->setColor(m_sharedRes->colors->at(uiColors::black), state::highlighted);
        butt->setBackgroundColor(m_sharedRes->colors->at(uiColors::highlight), state::highlighted);

        i++;
    }
}

void DropDownMenu::setMenuName(std::string str) {
    m_menuEntryName = str;

    if (m_menuEntryButt) {
        m_menuEntryButt->setText(str);
    }

    if (getWindow()) {
        getWindow()->update();
    }
}

}  // namespace ara
