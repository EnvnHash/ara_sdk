#pragma once

#include "DataModel/Item.h"
#include "UIElements/Text/TextBlock.h"

namespace ara {

class UIEdit : public TextBlock {
public:
    explicit UIEdit(unsigned opt = 0, int max_count = std::numeric_limits<int>::max());
    ~UIEdit() override;

    void changeValType(unsigned long t);
    void blockEdit(bool val);

    Font* UpdateDGV(bool* checkFontTexture) override;

    void loadStyleDefaults() override;
    void init() override;
    bool draw(uint32_t& objId) override;
    bool drawIndirect(uint32_t& objId) override;
    void drawCaret(bool forceCaretVaoUpdt = true);

    void updateStyleIt(ResNode* node, state st, const std::string& styleClass) override;
    void updateFontGeo() override;

    void setTextDist(const std::string& str);
    void setTextDist(const std::filesystem::path& p);
    void setValue(float val) override;
    void setValue(double val);
    void setValue(int val);

    [[nodiscard]] int getIntValue() const { return m_iValue; }

    void                setText(const std::string &str) override;
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
    void                setUseWheel(bool val) { m_useWheel = val; }
    void                setCaretColor(glm::vec4 c, state st = state::m_state) ;
    void                setCaretColor(float r, float g, float b, float a, state st = state::m_state) ;
    void                setCaretWidth(int w) { m_caretWidth = w; }
    void                setPropItem(Item* item) override;

    template<typename T>
    requires std::is_same_v<T, std::string> || std::is_same_v<T, std::filesystem::path>
    void setProp(Property<T> &prop) {
        m_stringProp = &prop;
        onChanged<T>(prop, [this](const std::any &val) { setText(std::any_cast<T>(val)); });
        addEnterCb([&prop](const std::string &txt) { prop = txt; }, &prop);
        setOnLostFocusCb([this, &prop] { prop = T(m_text); });
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
            prop = std::is_floating_point_v<T> ? static_cast<float>(atof(m_text.c_str())) : getIntValue();
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
            newVal[idx] = isFloat ? static_cast<float>(atof(m_text.c_str())) : atoi(m_text.c_str());
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

    void mouseDrag(hidData& data) override;
    void mouseDown(hidData& data) override;
    void mouseUp(hidData& data) override;
    void mouseWheel(hidData& data) override;
    void globalMouseDown(hidData& data) override;
    bool validateInputToString(int ch);
    void checkLimits();

    int insertChar(int ch, int position, bool call_cb = true);  // returns new caret position

    std::function<void(std::string)>                            m_setTextCb;
    std::unordered_map<void*, std::function<void(std::string)>> m_onEnterCb;

    int m_minInt = 0;
    int m_maxInt = 1000;

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

    bool m_blockEdit    = false;
};

}  // namespace ara
