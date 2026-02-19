//
// Created by sven on 16-02-26.
//

#pragma once

#include "DataModel/Item.h"
#include "UIElements/Text/Label.h"

namespace ara {

class TextBlock : public Label {
public:
    enum eopt {
        none               = 0x0000,
        num_int            = 0x0001,
        num_fp             = 0x0002,
        pass               = 0x0004,
        selectall_on_focus = 0x0008,  // when entering focus all content will be selected
        single_line        = 0x0010,  // Single line, do not accept CR
        accept_tabs        = 0x0020,  // Accepting TABs
        manual_space       = 0x0040,  // the edit pixel space is defined by the user,
                                      // otherwise it will adapt to content size
    };

    explicit TextBlock(unsigned opt = 0, int max_count = std::numeric_limits<int>::max());
    ~TextBlock() override;

    Font* UpdateDGV(bool* checkFontTexture) override;

    void loadStyleDefaults() override;
    void init() override;
    void initSelBgShader();
    bool draw(uint32_t& objId) override;
    bool drawIndirect(uint32_t& objId) override;
    void drawSelectionBg();
    void drawGlyphs(uint32_t& objId);
    void prepareSelBgVao();

    void updateStyleIt(ResNode* node, state st, const std::string& styleClass) override;
    void updateFontGeo() override;
    void clearDs() override;

    virtual void    setText(const std::string &str);
    std::string     parseTextForColors(const std::string& str);
    bool            setSelRangeAll();
    bool            setSelRange(int loIndex, int highIndex);
    bool            getSelRange(glm::ivec2& range);  // range should receive 2 values
    void            clearSelRange();
    bool            eraseContent(int loIndex, int highIndex);
    void            setBkSelColor(glm::vec4 c);
    virtual void    setPropItem(Item* item);
    virtual void    clearProp();

    template<typename T>
    requires std::is_same_v<T, std::string> || std::is_same_v<T, std::filesystem::path>
    void setProp(Property<T> &prop) {
        m_stringProp = &prop;
        onChanged<T>(prop, [this](const std::any &val) { setText(std::any_cast<T>(val)); });
    }

protected:
    void            mouseDrag(hidData& data) override;
    void            mouseDown(hidData& data) override;
    void            mouseUp(hidData& data) override;
    void            keyDown(hidData& data) override;
    virtual void    globalMouseDown(hidData& data);

    std::string     validateInputToString(int ch);
    int             getCaretByPixPos(float px, float py);
    int             validateCaretPos(int cpos) const;

    Shaders*     m_selBgShader = nullptr;
    UniformBlock m_uniBlockBg;
    VAO          m_backVao;
    Div*         m_caret = nullptr;
    IndDrawBlock m_selBgDB;

    float m_TabSize = 50.f;  // Tab size in pixels

    std::vector<std::pair<glm::ivec2, glm::vec4>> m_textColors{};

    glm::vec4 m_bkSelColor{0.f, 0.f, 1.f, 0.3f};  // Background selection color
    glm::vec4 m_caretColor{1.f};                  // Background selection color
    int       m_caretWidth = 2;

    std::string m_renderText;

    int        m_caretIndex = 0;
    glm::ivec2 m_caretRange{0};
    int        m_maxCount = 0;

    glm::vec2 m_mousePosCr{0.f};
    glm::vec2 m_bs{0};
    int       m_mouseEvent = 0;

    glm::ivec2 m_charSelection{0};
    glm::ivec2 m_lastSelRange{0};

    bool m_useWheel     = false;
    bool m_updtUniBlock = false;

    std::vector<glm::vec4> m_backPos;
    std::vector<GLuint>    m_backIndices;

    Property<std::string>* m_stringProp = nullptr;

    glm::vec2 cp{}, aux{};
};
}
