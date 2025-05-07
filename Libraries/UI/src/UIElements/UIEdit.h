#pragma once

#include "DataModel/Item.h"
#include "UIElements/Label.h"
#include <regex>

namespace ara {

class UIEdit : public Label {
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

    explicit UIEdit(unsigned opt = 0, int max_count = std::numeric_limits<int>::max());
    ~UIEdit() override;

    void changeValType(unsigned long t);

    void blockEdit(bool val) {
        m_blockEdit      = val;
        m_glyphsPrepared = false;
    }

    Font* UpdateDGV(bool* checkFontTexture) override;

    void loadStyleDefaults() override;
    void init() override;
    void initSelBgShader();
    bool draw(uint32_t& objId) override;
    bool drawIndirect(uint32_t& objId) override;
    void drawSelectionBg();
    void drawGlyphs(uint32_t& objId);
    void drawCaret(bool forceCaretVaoUpdt = true);
    void prepareSelBgVao();

    void updateStyleIt(ResNode* node, state st, const std::string& styleClass) override;
    void updateFontGeo() override;
    void clearDs() override;

    void setText(const std::string& str);
    void setValue(float val) override;
    void setValue(double val);
    void setValue(int val);

    [[nodiscard]] int getIntValue() const { return m_iValue; }

    bool setSelRangeAll();
    bool setSelRange(int lo_index, int hi_index);
    bool getSelRange(glm::ivec2& range);  // range should receive 2 values
    void clearSelRange();
    bool eraseContent(int lo_index, int hi_index);
    void setTextCb(std::function<void(std::string)> func) { m_setTextCb = std::move(func); }
    void addEnterCb(std::function<void(std::string)> func, void* ptr) { m_onEnterCb[ptr] = std::move(func); }
    void removeEnterCb(void* ptr) {
        if (auto it = m_onEnterCb.find(ptr); it != m_onEnterCb.end()) {
            m_onEnterCb.erase(it);
        }
    }
    void clearEnterCb() { m_onEnterCb.clear(); }
    void setMin(int min) { m_minInt = min; }
    void setMax(int max) { m_maxInt = max; }
    void setMinMax(int min, int max) {
        m_minInt = min;
        m_maxInt = max;
    }
    void                setMin(float min) { m_minF = min; }
    [[nodiscard]] float getMin() const { return m_minF; }
    void                setMax(float max) { m_maxF = max; }
    [[nodiscard]] float getMax() const { return m_maxF; }
    void                setMinMax(float min, float max) {
        m_minF = min;
        m_maxF = max;
    }
    void setMinMax(double min, double max) {
        m_minD = min;
        m_maxD = max;
    }
    void        setStep(double step) { m_stepD = step; }
    void        setStep(float step) { m_stepF = step; }
    void        setStep(int step) { m_stepI = step; }
    void        setPrecision(int prec) { m_precision = prec; }
    void        incValue(float amt, cfState cf);
    void        clampValue();

    static bool isValidIntInput(const std::string& str) {
        return std::regex_match(str, std::regex("[-+]|(\\+|-)?[[:digit:]]+"));
    }
    static bool isValidFloatInput(std::string& str) {
        return std::regex_match(str, std::regex("[-+]|[-+]?([0-9]*\\.[0-9]+|[0-9]+)")) ||
               std::regex_match(str, std::regex("[-+]|[-+]?([0-9]*\\.)"));
    }

    void setBkSelColor(glm::vec4 c) {
        m_BkSelColor     = c;
        m_glyphsPrepared = false;
    }
    void setUseWheel(bool val) { m_useWheel = val; }
    void setCaretColor(glm::vec4 c, state st = state::m_state) {
        m_caretColor     = c;
        m_glyphsPrepared = false;
        setStyleInitVal("caret-color",
                        "rgba(" + std::to_string(c.r * 255.f) + "," + std::to_string(c.g * 255.f) + "," +
                            std::to_string(c.b * 255.f) + "," + std::to_string(c.a * 255.f) + ")",
                        st);
    }
    void setCaretColor(float r, float g, float b, float a, state st = state::m_state) {
        m_caretColor.r   = r;
        m_caretColor.g   = g;
        m_caretColor.b   = b;
        m_caretColor.a   = a;
        m_glyphsPrepared = false;
        setStyleInitVal("caret-color",
                        "rgba(" + std::to_string(r * 255.f) + "," + std::to_string(g * 255.f) + "," +
                            std::to_string(b * 255.f) + "," + std::to_string(a * 255.f) + ")",
                        st);
    }
    void setCaretWidth(int w) { m_caretWidth = w; }

    void         setProp(Property<std::string>* prop) override;
    void         setProp(Property<std::filesystem::path>* prop) override;
    virtual void setProp(Property<int>* prop);
    virtual void setProp(Property<float>* prop);
    virtual void setProp(Property<glm::ivec2>* prop, int idx);
    virtual void setProp(Property<glm::vec2>* prop, int idx);
    virtual void setProp(Property<glm::vec3>* prop, int idx);
    virtual void setProp(Property<glm::ivec3>* prop, int idx);
    virtual void setPropItem(Item* item);

    virtual void clearProp();

protected:
    void keyDown(hidData& data) override;
    void onChar(hidData& data) override;
    void onLostFocus() override;

    void         mouseDrag(hidData& data) override;
    void         mouseDown(hidData& data) override;
    void         mouseUp(hidData& data) override;
    void         mouseWheel(hidData& data) override;
    virtual void globalMouseDown(hidData& data);

    bool validateInputToString(int ch) const;
    void checkLimits();
    int getCaretByPixPos(float px, float py);
    int validateCaretPos(int cpos) const;

    int insertChar(int ch, int position,
                   bool call_cb = true);  // returns new caret position

    Shaders*     m_selBgShader = nullptr;
    UniformBlock m_uniBlockBg;
    VAO          m_backVao;
    Div*         m_caret = nullptr;
    IndDrawBlock m_selBgDB;

    float m_TabSize = 50.f;  // Tab size in pixels

    glm::vec4 m_BkSelColor{0.f, 0.f, 1.f, 0.3f};  // Background selection color
    glm::vec4 m_caretColor{1.f};                  // Background selection color
    int       m_caretWidth = 2;

    std::string m_RenderText;

    int        m_CaretIndex = 0;
    glm::ivec2 m_CaretRange{0};
    int        m_MaxCount = 0;

    glm::vec2 m_mousePosCr{0.f};
    glm::vec2 m_bs{0};
    int       m_mouseEvent = 0;

    std::function<void(std::string)>                            m_setTextCb;
    std::unordered_map<void*, std::function<void(std::string)>> m_onEnterCb;

    int m_minInt = 0;
    int m_maxInt = 1000;

    glm::ivec2 m_charSelection{0};
    glm::ivec2 m_lastSelRange{0};

    float m_minF = 0.f;
    float m_maxF = 1.f;

    double m_minD = 0.f;
    double m_maxD = 1.f;

    double m_stepD = 0.1;
    float  m_stepF = 0.1f;
    int    m_stepI = 1;

    double m_dValue = 0.f;
    float  m_fValue = 0.f;
    int    m_iValue = 0;

    int m_precision = 3;

    bool m_useWheel     = false;
    bool m_blockEdit    = false;
    bool m_updtUniBlock = false;

    std::vector<glm::vec4> m_backPos;
    std::vector<GLuint>    m_backIndices;

    Property<std::string>* m_stringProp = nullptr;

    // temporary local variables
    glm::vec2 m_tCaretPos{0.f};
    int       m_tCi[2]{};  /// first and last character index
    glm::vec2 cp{0.f}, aux{0.f};
    glm::vec2 posLimit{0.f};
    glm::vec2 m_posLT{0.f};
    glm::vec2 m_posRB{0.f};
    glm::vec2 m_lSize{0.f};
    float     m_x1   = 0.f;
    float     m_x2   = 0.f;
    float     m_y1   = 0.f;
    float     m_y2   = 0.f;
    int       l_cpos = 0;

    valign l_auxAlign{};
};

}  // namespace ara
