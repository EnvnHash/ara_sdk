//
// Created by user on 5/5/25.
//

#include <Asset/ResNode.h>
#include <UIElements/UINodeBase/UINodeHID.h>
#include <UIElements/UINodeBase/UINode.h>
#include <UIWindow.h>

using namespace std;
using namespace glm;

namespace ara {

void UINodeHID::hidIt(hidData& data, hidEvent evt, std::list<UINode*>::iterator it, std::list<UINode*>& tree) {
    const auto node = *it;
    data.hit = node->containsObjectId(data.objId)
                && node->getState() != state::disabled
                && node->getState() != state::disabledSelected
                && node->getState() != state::disabledHighlighted
                && !node->isHIDBlocked();

    if (data.hit && evt == hidEvent::MouseDownLeft) {
        data.actIcon = node->ui_MouseIcon;

        // proc input focus
        if (node->getWindow()) {
            node->getWindow()->procInputFocus();
        }
    }

    // calculate the mouse position relative to this node
    data.mousePosNodeRel = data.mousePos - node->getWinPos();  // virtual pixels

    static std::unordered_map<hidEvent, std::function<void(hidData&, UINode*)> > evtMap = {
        { hidEvent::MouseDownLeft, [](hidData& d, UINode* n) { n->mouseDown(d); } },
        { hidEvent::MouseUpLeft, [](hidData& d, UINode* n) { n->mouseUp(d); } },
        { hidEvent::MouseDownRight, [](hidData& d, UINode* n) { n->mouseDownRight(d); } },
        { hidEvent::MouseUpRight, [](hidData& d, UINode* n) { n->mouseUpRight(d); } },
        { hidEvent::MouseMove, [](hidData& d, UINode* n) { n->mouseMove(d); } },
        { hidEvent::MouseDrag, [](hidData& d, UINode* n) { n->mouseDrag(d); } },
        { hidEvent::MouseWheel, [](hidData& d, UINode* n) { n->mouseWheel(d); } }
    };
    evtMap[evt](data, node);

    // process callbacks
    for (const auto& cbIt : node->getMouseHidCb(evt)) {
        if (!cbIt.second || (cbIt.second && (data.hit || evt == hidEvent::MouseDrag))) {
            cbIt.first(data);
            if (data.breakCbIt) {
                return;
            }
        }
    }

    // if hit, store that node and procInput focus if the left Mouse button was clicked
    if (data.consumed) {
        data.hitNode[evt] = node;
        return;
    }

    // go up one hierarchy, if we are at the end of the list, stop
    ++it;
    if (it == tree.end()) {
        return;
    }

    hidIt(data, evt, it, tree);
}

void UINodeHID::keyDownIt(hidData& data) {
    if (!m_blockHID && m_state != state::disabled && m_state != state::disabledSelected &&
        m_state != state::disabledHighlighted) {
        keyDown(data);
    }
}

void UINodeHID::onCharIt(hidData& data) {
    if (!m_blockHID && m_state != state::disabled && m_state != state::disabledSelected &&
        m_state != state::disabledHighlighted) {
        onChar(data);
    }
}

void UINodeHID::mouseIn(hidData& data) {
    if (m_state == state::disabled || m_state == state::disabledSelected || m_state == state::disabledHighlighted) {
        return;
    }

    if (!m_excludeFromStyles) {
        // set state only in case there are styledefinitions for it. If this is
        // not the case, we assume that this Node should ignore this state
        // setState(state::highlighted, true);
        if (!m_setStyleFunc[state::highlighted].empty()) {
            setState(state::highlighted);
        }

        // call all style definitions for the highlighted state if there are any
        for (const auto& it : m_setStyleFunc[state::highlighted]) {
            it.second();
        }

        if (!m_setStyleFunc[state::highlighted].empty()) {
            m_sharedRes->requestRedraw = true;
        }
    }

    for (const auto& it : m_mouseInCb | views::values) {
        it(data);
    }
}

void UINodeHID::mouseOut(hidData& data) {
    if (m_state == state::disabled || m_state == state::disabledSelected || m_state == state::disabledHighlighted) {
        return;
    }

    state lastState  = m_lastState;
    bool  changeBack = !m_setStyleFunc[state::highlighted].empty() && m_state != state::selected;

    // if the last state has no highlighted style definitions, the state didn't
    // change, so no need to change it back
    if (changeBack) {
        setState(m_lastState);
    }

    // change styles back to the last state if necessary
    if (!m_excludeFromStyles && changeBack) {
        for (const auto& it : m_setStyleFunc[lastState] | views::values) {
            it();
        }

        m_sharedRes->requestRedraw = true;
    }

    for (const auto& it : m_mouseOutCb) {
        it.second(data);
    }
}

void UINodeHID::addMouseHidCb(hidEvent evt, const std::function<void(hidData&)>& func, bool onHit) {
    m_mouseHidCb[evt].emplace_back(std::make_pair(func, onHit));
}

void UINodeHID::addMouseClickCb(const std::function<void(hidData&)>& func, bool onHit) {
    m_mouseHidCb[hidEvent::MouseDownLeft].emplace_back(std::make_pair(func, onHit));
}

void UINodeHID::addMouseClickRightCb(const std::function<void(hidData&)>& func, bool onHit) {
    m_mouseHidCb[hidEvent::MouseDownRight].emplace_back(std::make_pair(func, onHit));
}

void UINodeHID::addMouseUpCb(const std::function<void(hidData&)>& func, bool onHit) {
    m_mouseHidCb[hidEvent::MouseUpLeft].emplace_back(std::make_pair(func, onHit));
}

void UINodeHID::addMouseUpRightCb(const std::function<void(hidData&)>& func, bool onHit) {
    m_mouseHidCb[hidEvent::MouseUpRight].emplace_back(std::make_pair(func, onHit));
}

void UINodeHID::addMouseDragCb(const std::function<void(hidData&)>& func, bool onHit) {
    m_mouseHidCb[hidEvent::MouseDrag].emplace_back(std::make_pair(func, onHit));
}

void UINodeHID::addMouseMoveCb(const std::function<void(hidData&)>& func, bool onHit) {
    m_mouseHidCb[hidEvent::MouseMove].emplace_back(std::make_pair(func, onHit));
}

void UINodeHID::addMouseWheelCb(const std::function<void(hidData&)>& func, bool onHit) {
    m_mouseHidCb[hidEvent::MouseWheel].emplace_back(std::make_pair(func, onHit));
}

void UINodeHID::clearMouseCb(hidEvent evt) {
    m_mouseHidCb[evt].clear();
}

void UINodeHID::addMouseInCb(std::function<void(hidData&)> func, state st) {
    m_mouseInCb[st] = std::move(func);
}

void UINodeHID::addMouseOutCb(std::function<void(hidData&)> func, state st) {
    m_mouseOutCb[st] = std::move(func);
}

void UINodeHID::onLostFocus() {
    if (m_onLostFocusCb) {
        m_onLostFocusCb();
    }
}

void UINodeHID::onGotFocus() {
    if (m_onFocusedCb) {
        m_onFocusedCb();
    }
}

bool UINodeHID::removeFocus() {
    if (getWindow() && getWindow()->getInputFocusNode())
        if (dynamic_cast<UINodeHID*>(getWindow()->getInputFocusNode()) == this) {
            this->onLostFocus();
            return true;
        }
    return false;
}

void UINodeHID::setHIDBlocked(bool val) {
    m_blockHID = val;
}

UIWindow* UINodeHID::getWindow() const {
    return m_sharedRes ? static_cast<UIWindow*>(m_sharedRes->win) : nullptr;
}

}