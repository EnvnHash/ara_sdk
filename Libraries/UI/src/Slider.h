#pragma once

#include "UIEdit.h"

namespace ara {

class SliderKnob : public Div {
public:
    SliderKnob() : Div() {
        setName("SliderKnob");
        m_canReceiveDrag = true;
    }
    ~SliderKnob() override = default;

    void mouseDrag(hidData* data) override;

private:
    float m_dragStartValue;
};

class Slider : public Div {
public:
    Slider();
    ~Slider() override = default;

    UINode* addChild(std::unique_ptr<UINode> child);

    void init() override;

    virtual void addMouseDragCb(const std::function<void(hidData*)>& func) { m_knob->addMouseDragCb(func); }

    virtual void adjustKnob();
    void         updateMatrix() override;

    float getValue() override { return m_normValue * (m_max - m_min) + m_min; }

    virtual float getScaledVal(float in);
    virtual float getUnScaled(float in);

    int   getKnobWidth() const { return m_knobWidth; }
    float getMaxDragWay() { return (m_maxDragWay = getSize().x - static_cast<float>(m_knobWidth)); }
    float getScaledNormValue() const  { return m_scaledNormValue; }
    void  setNumEdit(UIEdit* edit) { m_numEdit = edit; }
    void  setLineColor(float r, float g, float b, float a) {
        m_lineColor.r = r;
        m_lineColor.g = g;
        m_lineColor.b = b;
        m_lineColor.a = a;
    }
    void setMin(float min) { m_min = min; }
    void setMax(float max) { m_max = max; }
    void setValueChangeCb(std::function<void(float)> f) { m_valueChangeCb = std::move(f); }

    void         setAbsValue(float val) override;
    void         setValue(float val) override;
    virtual void setValuePtr(float* val);

    void setValueString(std::string val) { m_valueAsString = std::move(val); }
    void setScale(sliderScale scaling) { m_scaling = scaling; }
    void setMaxDragWay(float val) {
        m_maxDragWay    = val;
        m_maxDragWayRel = m_maxDragWay / getSize().x;
    }
    void        reset() { m_valueAsString = ""; }
    float       setKnobProportion(float nprop) { return (m_knobProportion = nprop); }
    SliderKnob* getKnob() { return m_knob; }

private:
    UINode*     m_numView   = nullptr;
    UIEdit*     m_numEdit   = nullptr;
    Div*        m_horLine   = nullptr;
    SliderKnob* m_knob      = nullptr;
    glm::vec4   m_lineColor = glm::vec4{1.f};
    glm::mat4   m_knobMat;

    std::string m_valueAsString;

    int         m_knobWidth       = 10;
    float       m_knobProportion  = .8f;
    float       m_max             = 1.f;
    float       m_min             = 0.f;
    float       m_maxDragWay      = 0.f;
    float       m_maxDragWayRel   = 0.f;
    float       m_normValue       = 0.5f;
    float       m_scaledNormValue = 0.f;
    float       m_mappedValue     = 0.f;
    sliderScale m_scaling         = sliderScale::slideLinear;

    std::function<void(float)> m_valueChangeCb;
};
}  // namespace ara
