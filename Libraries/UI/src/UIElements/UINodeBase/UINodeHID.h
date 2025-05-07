//
// Created by user on 5/5/25.
//

#pragma once

#include <UIElements/UINodeBase/UINodeStyle.h>

namespace ara {

class UIWindow;
class UINode;

class UINodeHID : public UINodeStyle {
public:
    // Mouse interaction
    [[nodiscard]] bool      canReceiveDrag() const { return m_canReceiveDrag; }
    virtual void            setCanReceiveDrag(bool val) { m_canReceiveDrag = val; }
    void                    setMouseIcon(MouseIcon icon) { ui_MouseIcon = icon; }
    [[nodiscard]] bool      isHIDBlocked() const { return m_blockHID; }
    virtual void            setHIDBlocked(bool val);

    virtual void            onLostFocus();
    virtual void            onGotFocus();
    virtual bool            removeFocus();
    [[nodiscard]] bool      hasInputFocus() const { return m_hasInputFocus; }
    void                    setInputFocus(bool val) { m_hasInputFocus = val; }
    [[nodiscard]] bool      getFocusAllowed() const { return m_focusAllowed; }
    void                    setFocusAllowed(bool val) { m_focusAllowed = val; }
    void                    setOnFocusedCb(std::function<void()> f) { m_onFocusedCb = std::move(f); }
    std::function<void()>*  getOnFocusedCb() { return m_onFocusedCb ? &m_onFocusedCb : nullptr; }
    void                    setOnLostFocusCb(std::function<void()> f) { m_onLostFocusCb = std::move(f); }
    std::function<void()>*  getOnLostFocusCb() { return m_onLostFocusCb ? &m_onLostFocusCb : nullptr; }
    void                    setReturnFocusCb(std::function<void()> func) { m_returnFocusCb = std::move(func); }
    std::function<void()>   getReturnFocusCb() { return m_returnFocusCb; }

    static void hidIt(hidData& data, hidEvent evt, std::list<UINode*>::iterator it, std::list<UINode*>& tree);

    virtual void keyDownIt(hidData& data);
    virtual void onCharIt(hidData& data);
    // virtual void clearFocusIt();

    // mouse HID methods which are called during local tree iteration (root -> this node)
    virtual void mouseMove(hidData& data) {}
    virtual void mouseDrag(hidData& data) {}
    virtual void mouseDown(hidData& data) {}
    virtual void mouseDownRight(hidData& data) {}
    virtual void mouseUp(hidData& data) {}
    virtual void mouseUpRight(hidData& data) {}
    virtual void mouseWheel(hidData& data) {}

    // mouseIn and mouseOut are called directly without tree iteration
    virtual void mouseIn(hidData& data);
    virtual void mouseOut(hidData& data);

    // keyboard HID methods which are called during local tree iteration (root -> this node)
    virtual void keyDown(hidData& data) {}
    virtual void keyUp(hidData& data) {}
    virtual void onChar(hidData& data) {}
    virtual void onResize() {}

    void addMouseHidCb(hidEvent evt, const std::function<void(hidData&)>& func, bool onHit = true);
    void addMouseClickCb(const std::function<void(hidData&)>& func, bool onHit = true);
    void addMouseClickRightCb(const std::function<void(hidData&)>& func, bool onHit = true);
    void addMouseUpCb(const std::function<void(hidData&)>& func, bool onHit = true);
    void addMouseUpRightCb(const std::function<void(hidData&)>& func, bool onHit = true);
    void addMouseDragCb(const std::function<void(hidData&)>& func, bool onHit = true);
    void addMouseMoveCb(const std::function<void(hidData&)>& func, bool onHit = true);
    void addMouseWheelCb(const std::function<void(hidData&)>& func, bool onHit = true);
    void clearMouseCb(hidEvent evt);
    void addMouseInCb(std::function<void(hidData&)> func, state st = state::m_state);
    void addMouseOutCb(std::function<void(hidData&)> func, state st = state::none);

    std::list<mouseCb>&             getMouseHidCb(hidEvent evt) { return m_mouseHidCb[evt]; }
    std::list<mouseCb>&             getMouseDownCb() { return m_mouseHidCb[hidEvent::MouseDownLeft]; }
    std::list<mouseCb>&             getMouseUpCb() { return m_mouseHidCb[hidEvent::MouseUpLeft]; }
    std::list<mouseCb>&             getMouseDownRightCb() { return m_mouseHidCb[hidEvent::MouseDownRight]; }
    std::list<mouseCb>&             getMouseUpRightCb() { return m_mouseHidCb[hidEvent::MouseUpRight]; }
    std::list<mouseCb>&             getMouseDragCb() { return m_mouseHidCb[hidEvent::MouseDrag]; }
    std::list<mouseCb>&             getMouseWheelCb() { return m_mouseHidCb[hidEvent::MouseWheel]; }
    std::function<void(hidData&)>*  getKeyDownCb() { return m_keyDownCb ? &m_keyDownCb : nullptr; }
    std::function<void(hidData&)>*  getKeyUpCb() { return m_keyUpCb ? &m_keyUpCb : nullptr; }

    void setKeyDownCb(std::function<void(hidData&)> func) { m_keyDownCb = std::move(func); }
    void setKeyUpCb(std::function<void(hidData&)> func) { m_keyUpCb = std::move(func); }

    UIWindow*  getWindow() const;

protected:
    std::unordered_map<hidEvent, std::list<mouseCb>> m_mouseHidCb;

    std::unordered_map<state, std::function<void(hidData&)>>    m_mouseInCb;
    std::unordered_map<state, std::function<void(hidData&)>>    m_mouseOutCb;
    std::function<void(hidData&)>                               m_keyDownCb;
    std::function<void(hidData&)>                               m_keyUpCb;

    std::function<void()> m_onFocusedCb;
    std::function<void()> m_onLostFocusCb;
    std::function<void()> m_returnFocusCb;

    MouseIcon ui_MouseIcon = MouseIcon::arrow;

    bool m_focusAllowed                  = true;
    bool m_hasInputFocus                 = false;
    bool m_canReceiveDrag                = false;
    bool m_blockHID                      = false;
};

}
