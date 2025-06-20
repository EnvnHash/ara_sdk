#pragma once

#include "UIElements/Label.h"

namespace ara {

class Button : public Label {
public:
    Button();
    Button(glm::vec2 pos, glm::vec2 size, glm::vec4 text_color, glm::vec4 bg_color, const std::string& text,
           std::pair<align, valign> align, const std::string& font_type, int font_height);
    ~Button() override = default;

    bool draw(uint32_t& objId) override;
    void mouseUp(hidData& data) override;
    void mouseUpRight(hidData& data) override;
    void mouseMove(hidData& data) override;
    void mouseOut(hidData& data) override;
    void click(hidData& data);
    void toggle(bool val);

    virtual void setProp(Property<bool> &prop);

    [[maybe_unused]] void setAltText(const char *alt_text) { m_alt_text = std::string(alt_text); }
    [[maybe_unused]] void setAltTextFontSize(uint32_t fontSize) { m_altTextFontSize = fontSize; }
    [[maybe_unused]] void setAltTextFontType(const std::string& fontType) { m_altTextFontType = fontType; }
    [[maybe_unused]] void setToggleCb(const std::function<void(bool)>& cbFunc) { m_toggleCbFunc = cbFunc; }
    void setClickedCb(const std::function<void()>& cbFunc) { m_clickedFunc = cbFunc; }
    void setIsToggle(bool val) { m_isToggle = val; }

    bool m_mouseIsIn     = false;
    bool m_isToggle      = false;
    bool m_show_alt_text = false;

    std::chrono::time_point<std::chrono::system_clock> m_mouseInTime;

    uint32_t    m_altTextFontSize = 22;
    glm::vec2   m_textWidth{};
    glm::vec4   m_alt_text_offs{};
    glm::mat4   m_alt_text_mat{};
    glm::vec4   m_typoColor{0.f, 0.f, 0.f, 1.f};

    std::function<void(bool)> m_toggleCbFunc;
    std::function<void()>     m_clickedFunc;
    std::string               m_alt_text;
    std::string               m_altTextFontType;

    Shaders        *m_stdCol       = nullptr;
    Shaders        *m_stdColBorder = nullptr;
    Property<bool> *m_prop         = nullptr;
};
}  // namespace ara
