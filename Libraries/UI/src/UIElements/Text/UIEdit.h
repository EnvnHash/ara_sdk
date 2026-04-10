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

    Font* updateDGV(bool* checkFontTexture) override;
    void calculateOffset();

    void loadStyleDefaults() override;
    void init() override;
    bool draw(uint32_t& objId) override;
    bool drawIndirect(uint32_t& objId) override;
    void drawCaret(bool forceCaretVaoUpdt = true);
    void drawCaretIndirect(bool updtTree, bool forceCaretVaoUpdt) const;
    void drawCaretSelectedState(bool& updtTree);
    void setCaretRespectAlignment(glm::vec2& tCaretPos);

    void updateValFromText(const std::string& txt, bool updateText=true);
    void updateStyleIt(ResNode* node, state st, const std::string& styleClass) override;
    void updateFontGeo() override;

    void setTextDist(const std::string& str);
    void setTextDist(const std::filesystem::path& p);

    void addEnterCb(std::function<void(std::string)> func, void* ptr) { m_onEnterCb[ptr] = std::move(func); }
    void removeEnterCb(void* ptr);
    void clearEnterCb() { m_onEnterCb.clear(); }

    void setText(const std::string &str) override;
    void setTextCb(std::function<void(const std::string&)> func) { m_setTextCb = std::move(func); }
    void setPrecision(const int precision) { m_precision = precision; }
    void setUseWheel(const bool val) { m_useWheel = val; }
    void setCaretColor(glm::vec4 c, state st = state::m_state) ;
    void setCaretColor(float r, float g, float b, float a, state st = state::m_state) ;
    void setCaretWidth(const int w) { m_caretWidth = w; }
    void setPropItem(Item* item) override;

    void incValue(float amt, cfState cf);
    void clampValue();

    float getValue() override {
        return getValue<float>();
    }

    template <typename CoordinateType>
    CoordinateType getValue() {
        return std::get<CoordinateType>(m_val);
    }

    template <typename CoordinateType>
    void setValue(CoordinateType val, const bool updtText=true) {
        std::get<CoordinateType>(m_val) = std::max(std::min(val, std::get<CoordinateType>(m_maxVal)), std::get<CoordinateType>(m_minVal));
        setOpt(std::is_floating_point_v<CoordinateType> ? num_fp : num_int);

        if (updtText) {
            std::stringstream stream;
            stream << std::fixed << std::setprecision(m_precision) << std::get<CoordinateType>(m_val);
            const bool updt = m_text.size() != stream.str().size();
            m_text    = stream.str();
            reqUpdtGlyphs(updt);
        }
    }

    template <typename T>
    void incValue(float amt) {
        setValue<T>(std::get<T>(m_val) + std::get<T>(m_step) * static_cast<T>(amt));
    }

    template <typename CoordinateType>
    void setMinMax(CoordinateType min, CoordinateType max) {
        setMin<CoordinateType>(min);
        setMax<CoordinateType>(max);
    }

    template <typename CoordinateType>
    void setMin(CoordinateType min) {
        std::get<CoordinateType>(m_minVal) = min;
    }

    template <typename CoordinateType>
    void setMax(CoordinateType min) {
        std::get<CoordinateType>(m_maxVal) = min;
    }

    template <typename CoordinateType>
    void setStep(CoordinateType step) {
        std::get<CoordinateType>(m_step) = step;
    }

    template<typename T>
    requires std::is_same_v<T, std::string> || std::is_same_v<T, std::filesystem::path>
    void setProp(Property<T> &prop) {
        m_stringProp = &prop;
        onChanged<T>(prop, [this](const std::any &val) { setText(std::any_cast<T>(val)); });
        addEnterCb([&prop](const std::string &txt) { prop = txt; }, &prop);
        setOnLostFocusCb([this, &prop] { prop = T(m_text); });
        setTextDist(prop());
    }

    template<typename CoordinateType>
    void setProp(Property<CoordinateType> &prop) {
        setOpt(single_line | (std::is_floating_point_v<CoordinateType> ? num_fp : num_int));

        onChanged<CoordinateType>(prop, [this](const std::any &val) { setText(std::to_string(std::any_cast<CoordinateType>(val))); });
        addEnterCb([&prop](const std::string &txt) {
            prop = std::is_floating_point_v<CoordinateType> ? static_cast<CoordinateType>(atof(txt.c_str()))
                                                            : atoi(txt.c_str());
        }, &prop);
        setOnLostFocusCb([this, &prop] {
            prop = std::is_floating_point_v<CoordinateType> ? static_cast<CoordinateType>(atof(m_text.c_str()))
                                                            : std::get<CoordinateType>(m_val);
        });
        setMinMax(prop.getMin(), prop.getMax());
        setStep(prop.getStep());

        if (std::is_floating_point_v<CoordinateType>){
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

        setOpt(isFloat ? num_fp : num_int);
        setMinMax(prop.getMin()[idx], prop.getMax()[idx]);
        setStep(prop.getStep()[idx]);
        setValue(prop()[idx]);
        setUseWheel(true);
    }

    void onChar(hidData& data) override;
    void onLostFocus() override;
    void mouseWheel(hidData& data) override;

protected:
    void keyDown(hidData& data) override;
    void mouseDrag(hidData& data) override;
    void mouseDown(hidData& data) override;
    void mouseUp(hidData& data) override;
    void globalMouseDown(hidData& data) override;

    void procEnterAndReturn();
    void procTab();
    void procShiftPlusArrowSelect(const hidData& data);
    void moveCaret(const hidData& data);
    void moveCaretBackspace();
    void moveCaretDel();
    void moveCaretLeft();
    void moveCaretRight();
    void moveCaretHome();
    void moveCaretEnd();
    void keyModifySelection(const hidData& data, bool& updateValue);
    bool validateNumInputToString(int ch);
    void calcLeftLineOffset();
    void calcRightLineOffset();
    void checkLimits();

    int insertChar(int ch, int position, bool call_cb = true);  // returns new caret position

    std::function<void(const std::string&)>                             m_setTextCb;
    std::unordered_map<void*, std::function<void(const std::string&)>>  m_onEnterCb;

    Number m_minVal{ std::numeric_limits<int32_t>::min(), std::numeric_limits<float>::min(), std::numeric_limits<double>::min()};
    Number m_maxVal{ std::numeric_limits<int32_t>::max(), std::numeric_limits<float>::max(), std::numeric_limits<double>::max()};
    Number m_step{ 1, 0.1f, 0.1 };
    Number m_val{ 0, 0.f, 0 };

    int m_precision = 3;
    bool m_blockEdit = false;
    bool m_needsOverflowHandling = false;
};

}  // namespace ara
