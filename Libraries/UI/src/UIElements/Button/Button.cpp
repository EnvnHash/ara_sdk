#include "Button.h"

#include "UIWindow.h"

using namespace glm;
using namespace std;

namespace ara {

Button::Button() {
    setTypeName<Button>();
    setName(getTypeName<Button>());
    setFocusAllowed(false);
    setScissorChildren(true);
}

Button::Button(const LabelPars &initData) : Label(initData) {
    setTypeName<Button>();
    setName(getTypeName<Button>());
    setFocusAllowed(false);
    setScissorChildren(true);
}

Button::Button(const vec2 pos, const vec2 size, const vec4 text_color, const vec4 bg_color, const std::string& text,
               const pair<align, valign> align, const std::string& fontType, int fontHeight)
    : Label(LabelPars{
          .pos = static_cast<ivec2>(pos),
          .size = static_cast<ivec2>(size),
          .color = text_color,
          .bgColor = bg_color,
          .text = text,
          .textAlignX = align.first,
          .textAlignY = align.second,
          .fontType = fontType,
          .fontHeight=0
      }), m_typoColor({0.f, 0.f, 0.f, 1.f}) {
    setName(getTypeName<Button>());
    setFocusAllowed(false);
    setScissorChildren(true);
}

bool Button::draw(uint32_t& objId) {
    return Label::draw(objId);
    // raw text is rendered here implicitly
}

void Button::mouseMove(hidData& data) {
    data.actIcon  = ui_MouseIcon;
    data.consumed = true;

    if (!m_mouseIsIn && !m_showAltText) {
        m_mouseInTime = std::chrono::system_clock::now();
    }

    for (const auto &key: m_mouseHidCb[hidEvent::MouseMove] | views::keys) {
        key(data);
    }

    m_mouseIsIn = true;
}

void Button::mouseOut(hidData& data) {
    m_mouseIsIn     = false;
    m_showAltText = false;
    UINode::mouseOut(data);
}

void Button::mouseUp(hidData& data) {
    if (data.hit) {
        click(data);
        data.consumed = true;
    }
}

void Button::mouseUpRight(hidData& data) {
    if (data.hit) {
        click(data);
        data.consumed = true;
    }
}

void Button::click(hidData& data) {
    toggle(m_state != state::selected);

    if (m_state != state::disabled && m_clickedFunc) {
        m_clickedFunc();
    }

    setDrawFlag();
}

void Button::toggle(const bool val) {
    if (!m_isToggle) {
        return;
    }

    UINode::setSelected(val, true);

    if (m_toggleCbFunc) {
        m_toggleCbFunc(val);
    }

    if (m_prop && (*m_prop)() != val) {
        *m_prop = val;
    }

    setDrawFlag();
}

void Button::setProp(Property<bool>& prop) {
    m_prop = &prop;
    onPreChange<bool>(prop, [this](const std::any &val) {
        if (const bool v = std::any_cast<bool>(val); v != (m_state == state::selected)) {
            toggle(v);
            setDrawFlag();
        }
    });

    toggle(prop());
    setDrawFlag();
}

}  // namespace ara
