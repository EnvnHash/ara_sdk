#include "Button.h"

#include "UIWindow.h"

using namespace glm;
using namespace std;

namespace ara {

Button::Button() : Label() {
    setName(getTypeName<Button>());
    setFocusAllowed(false);
    Label::setScissorChildren(true);
}

Button::Button(const std::string& styleClass) : Label(styleClass) {
    setName(getTypeName<Button>());
    setFocusAllowed(false);
    Label::setScissorChildren(true);
}

Button::Button(vec2 pos, vec2 size, vec4 text_color, vec4 bg_color, const std::string& text,
               pair<align, valign> align, const std::string& font_type, int font_height)
    : Label(LabelInitData{
          .pos = static_cast<glm::ivec2>(pos),
          .size = static_cast<glm::ivec2>(size),
          .text_color = text_color,
          .bg_color = bg_color,
          .text = text,
          .ax = align.first,
          .ay = align.second,
          .font_type = font_type,
          .font_height=0
      }), m_typoColor({0.f, 0.f, 0.f, 1.f}) {
    setName(getTypeName<Button>());
    setFocusAllowed(false);
    Label::setScissorChildren(true);
}

bool Button::draw(uint32_t* objId) {
    return Label::draw(objId);
    // raw text is rendered here implicitly
}

void Button::mouseMove(hidData* data) {
    data->actIcon  = ui_MouseIcon;
    data->consumed = true;

    if (!m_mouseIsIn && !m_show_alt_text) {
        m_mouseInTime = std::chrono::system_clock::now();
    }

    for (const auto &key: m_mouseHidCb[hidEvent::MouseMove] | views::keys) {
        key(data);
    }

    m_mouseIsIn = true;
}

void Button::mouseOut(hidData* data) {
    m_mouseIsIn     = false;
    m_show_alt_text = false;
    UINode::mouseOut(data);
}

void Button::mouseUp(hidData* data) {
    if (data->hit) {
        click(data);
        data->consumed = true;
    }
}

void Button::mouseUpRight(hidData* data) {
    if (data->hit) {
        click(data);
        data->consumed = true;
    }
}

void Button::click(hidData* data) {
    toggle(m_state != state::selected);

    if (m_state != state::disabled && m_clickedFunc) {
        m_clickedFunc();
    }

    // force a mouseOut
    if (getWindow() && getWindow()->getLastHoverFound() && data) {
        getWindow()->getLastHoverFound()->mouseOut(data);
        getWindow()->setLastHoverFound(nullptr);
    }

    setDrawFlag();
}

void Button::toggle(bool val) {
    if (!m_isToggle) {
        return;
    }

    UINode::setSelected(val, true);

    if (m_toggleCbFunc) {
        m_toggleCbFunc(val);
    }

    if (m_prop && (*m_prop)() != val) {
        (*m_prop) = val;
    }

    setDrawFlag();
}

void Button::setProp(Property<bool>* prop) {
    if (!prop) {
        return;
    }

    m_prop = prop;
    onChanged<bool>(prop, [this](const std::any &val) {
        bool v = std::any_cast<bool>(val);
        if (v != (m_state == state::selected)) {
            toggle(v);
            setDrawFlag();
        }
    });

    toggle((*prop)());
    setDrawFlag();
}

}  // namespace ara
