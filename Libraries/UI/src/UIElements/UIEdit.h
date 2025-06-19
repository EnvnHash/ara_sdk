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
    void blockEdit(bool val);

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

    void setTextDist(const std::string& str);
    void setTextDist(const std::filesystem::path& p);
    void setText(const std::string &str);
    void setValue(float val) override;
    void setValue(double val);
    void setValue(int val);

    [[nodiscard]] int getIntValue() const { return m_iValue; }

    bool                setSelRangeAll();
    bool                setSelRange(int lo_index, int hi_index);
    bool                getSelRange(glm::ivec2& range);  // range should receive 2 values
    void                clearSelRange();
    bool                eraseContent(int lo_index, int hi_index);
    void                setTextCb(std::function<void(std::string)> func) { m_setTextCb = std::move(func); }
    void                addEnterCb(std::function<void(std::string)> func, void* ptr) { m_onEnterCb[ptr] = std::move(func); }
    void                removeEnterCb(void* ptr);
    void                clearEnterCb() { m_onEnterCb.clear(); }
    void                setMin(int min) { m_minInt = min; }
    void                setMax(int max) { m_maxInt = max; }
    void                setMinMax(int min, int max);
    void                setMin(float min) { m_minF = min; }
    [[nodiscard]] float getMin() const { return m_minF; }
    void                setMax(float max) { m_maxF = max; }
    [[nodiscard]] float getMax() const { return m_maxF; }
    void                setMinMax(float min, float max);
    void                setMinMax(double min, double max) ;
    void                setStep(double step) { m_stepD = step; }
    void                setStep(float step) { m_stepF = step; }
    void                setStep(int step) { m_stepI = step; }
    void                setPrecision(int prec) { m_precision = prec; }
    void                incValue(float amt, cfState cf);
    void                clampValue();
    void                setBkSelColor(glm::vec4 c);
    void                setUseWheel(bool val) { m_useWheel = val; }
    void                setCaretColor(glm::vec4 c, state st = state::m_state) ;
    void                setCaretColor(float r, float g, float b, float a, state st = state::m_state) ;
    void                setCaretWidth(int w) { m_caretWidth = w; }
    virtual void        setPropItem(Item* item);
    virtual void        clearProp();

    template<typename T>
    requires std::is_same_v<T, std::string> || std::is_same_v<T, std::filesystem::path>
    void setProp(Property<T> &prop) {
        m_stringProp = &prop;
        onChanged<T>(prop, [this](const std::any &val) { setText(std::any_cast<T>(val)); });
        addEnterCb([&prop](const std::string &txt) { prop = txt; }, &prop);
        setOnLostFocusCb([this, &prop] { prop = T(m_Text); });
        setTextDist(prop());
    }

    template<typename T>
    requires std::is_integral_v<T> || std::is_floating_point_v<T>
    void setProp(Property<T> &prop) {
        setOpt(single_line | (std::is_floating_point_v<T> ? num_fp : num_int));

        onChanged<T>(prop, [this](const std::any &val) { setText(std::to_string(std::any_cast<T>(val))); });
        addEnterCb([&prop](const std::string &txt) {
            prop = std::is_floating_point_v<T> ? static_cast<float>(atof(txt.c_str())) : atoi(txt.c_str());
        }, &prop);
        setOnLostFocusCb([this, &prop] {
            prop = std::is_floating_point_v<T> ? static_cast<float>(atof(m_Text.c_str())) : getIntValue();
        });
        setMinMax(prop.getMin(), prop.getMax());
        setStep(prop.getStep());
        if (std::is_floating_point_v<T>){
            setValue(prop());
        } else {
            setText(std::to_string(prop()));
        }
        setUseWheel(true);
    }

    template<typename T>
    requires std::is_same_v<T, glm::ivec2> || std::is_same_v<T, glm::ivec3> || std::is_same_v<T, glm::vec2> || std::is_same_v<T, glm::vec3>
    void setProp(Property<T> &prop, int idx) {
        bool isFloat = typeid(prop()[0]) == typeid(float);
        onChanged<T>(prop, [this, idx](std::any val) { setValue(std::any_cast<T>(val)[idx]); });
        addEnterCb([&prop, idx, isFloat](const std::string &txt) {
            auto newVal = prop();
            newVal[idx] = isFloat ? static_cast<float>(atof(txt.c_str())) : atoi(txt.c_str());
            prop        = newVal;
        }, &prop);

        setOnLostFocusCb([this, &prop, idx, isFloat] {
            auto newVal = prop();
            newVal[idx] = isFloat ? static_cast<float>(atof(m_Text.c_str())) : atoi(m_Text.c_str());
            prop        = newVal;
        });

        setOpt(isFloat ? UIEdit::num_fp : UIEdit::num_int);
        setMinMax(prop.getMin()[idx], prop.getMax()[idx]);
        setStep(prop.getStep()[idx]);
        setValue(prop()[idx]);
        setUseWheel(true);
    }

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
    glm::vec2 m_tCaretPos{};
    int       m_tCi[2]{};  /// first and last character index
    glm::vec2 cp{}, aux{};
    glm::vec2 posLimit{};
    glm::vec2 m_posLT{};
    glm::vec2 m_posRB{};
    glm::vec2 m_lSize{};
    float     m_x1   = 0.f;
    float     m_x2   = 0.f;
    float     m_y1   = 0.f;
    float     m_y2   = 0.f;
    int       l_cpos = 0;

    valign l_auxAlign{};
};

}  // namespace ara
