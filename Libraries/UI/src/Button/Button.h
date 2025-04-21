#pragma once

#include "Utils/Texture.h"

#include "Label.h"

namespace ara {

class Button : public Label {
public:
    Button();
    Button(std::string &&styleClass);
    Button(int x, int y, int w, int h, glm::vec4 text_color, glm::vec4 bg_color, const std::string& _text, align ax, valign ay,
           const std::string& font_type, int font_height);
    virtual ~Button() = default;

    bool draw(uint32_t *objId) override;
    void mouseUp(hidData *data) override;
    void mouseUpRight(hidData *data) override;
    void mouseMove(hidData *data) override;
    void mouseOut(hidData *data) override;
    void click(hidData *data);
    void toggle(bool val);

    virtual void setProp(Property<bool> *prop);

    void setAltText(const char *alt_text) { m_alt_text = std::string(alt_text); }
    void setAltTextFontSize(uint32_t fontSize) { m_altTextFontSize = fontSize; }
    void setAltTextFontType(const std::string& fontType) { m_altTextFontType = fontType; }
    void setToggleCb(const std::function<void(bool)>& cbFunc) { m_toggleCbFunc = cbFunc; }
    void setClickedCb(const std::function<void()>& cbFunc) { m_clickedFunc = cbFunc; }
    void setIsToggle(bool val) { m_isToggle = val; }

    bool m_getTextWidth  = true;
    bool m_mouseIsIn     = false;
    bool m_isToggle      = false;
    bool m_show_alt_text = false;

    uint32_t m_altTextFontSize = 22;

    std::chrono::time_point<std::chrono::system_clock> m_mouseInTime;
    glm::vec2                                          m_textWidth;
    glm::vec4                                          m_alt_text_offs;
    glm::mat4                                          m_alt_text_mat;
    glm::vec4                                          m_typoColor;

    std::function<void(bool)> m_toggleCbFunc;
    std::function<void()>     m_clickedFunc;
    std::string               m_alt_text;
    std::string               m_altTextFontType;

    Shaders        *m_stdCol       = nullptr;
    Shaders        *m_stdColBorder = nullptr;
    Property<bool> *m_prop         = nullptr;
};
}  // namespace ara
