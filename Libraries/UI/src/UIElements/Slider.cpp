#include "Slider.h"

#include "UIWindow.h"

using namespace glm;
using namespace std;

namespace ara {

Slider::Slider() {
    m_canReceiveDrag = true;
    setTypeName<Slider>();
    setName(getTypeName<Slider>());
    setFocusAllowed(false);

    m_horLine = &push<Div>();
    m_horLine->setFocusAllowed(false);

    m_knob = &push<SliderKnob>();
    m_knob->setFocusAllowed(false);
}

void Slider::init() {
    setKnowWidth();
    m_horLine->setSize(-m_knobWidth, 2);
    m_horLine->setAlign(align::center, valign::center);
    m_horLine->setBackgroundColor(m_sharedRes->colors->at(uiColors::blue));
    m_horLine->setBorderWidth(0);
    m_horLine->setBorderColor(m_sharedRes->colors->at(uiColors::blue));

    m_knob->setBackgroundColor(m_sharedRes->colors->at(uiColors::blue));
    m_knob->setAlignY(valign::center);
}

void Slider::adjustKnob() {
    setKnowWidth();
    m_knob->setSize(m_knobWidth, m_knobWidth);
    m_knob->setBorderRadius(m_knobWidth / 2);

    m_maxDragWay    = m_size.x - static_cast<float>(m_knobWidth);
    m_maxDragWayRel = m_maxDragWay / m_size.x;
}

void Slider::updateMatrix() {
    const bool changed = m_geoChanged;
    UINode::updateMatrix();
    if (changed) {
        adjustKnob();
        m_knob->setX(m_normValue * m_maxDragWayRel);
    }
}

UINode* Slider::addChild(std::shared_ptr<UINode> child) {
    m_children.emplace_back(std::move(child));
    const auto node = dynamic_cast<UINode*>(children().back().get());

    // if there is a numeric view as a child, use it to display the value
    if (m_children.back()->name() == "NumericView") {
        m_numView = node;
    }

    initChild(*node, this);

    // if a value was set earlier, update now
    if (m_numView) {
        m_numView->setValue(getValue());
    }

    return node;
}

void SliderKnob::mouseDrag(hidData& data) {
    const auto slid = dynamic_cast<Slider *>(getParent());

    // remember actual value
    if (data.dragStart) {
        m_dragStartValue = slid->getUnScaled(slid->getScaledNormValue());
    } else {
        // values from slider are scaled,
        const float relMove      = data.movedPix.x / slid->getMaxDragWay();
        const float newNormValue = std::min<float>(std::max<float>(m_dragStartValue + relMove, 0.f), 1.f);
        slid->setValue(newNormValue);

        setDrawFlag();
    }

    data.consumed = true;
}

float Slider::getScaledVal(const float in) {
    if (m_scaling == sliderScale::slideLinear) {
        return in;
    }
    if (m_scaling == sliderScale::slidSquared) {
        return std::sqrt(in);
    }
    if (m_scaling == sliderScale::slidSqrt) {
        return in * in;
    }

    return 0.f;
}

float Slider::getUnScaled(const float in) {
    if (m_scaling == sliderScale::slideLinear) {
        return in;
    }
    if (m_scaling == sliderScale::slidSquared) {
        return std::sqrt(in);
    }
    if (m_scaling == sliderScale::slidSqrt) {
        return in * in;
    }

    return 0.f;
}

// absolute value has to be normalized internally
void Slider::setAbsValue(const float val) {
    const float c_val = std::fmin(std::fmax(val, m_min), m_max);
    m_normValue       = (c_val - m_min) / (m_max - m_min);
    m_scaledNormValue = getScaledVal(m_normValue);

    if (m_maxDragWayRel != 0.f) {
        m_knob->setX(m_normValue * m_maxDragWayRel);
    }
    if (m_numView) {
        m_numView->setValue(getValue());
    }
    if (m_valueChangeCb) {
        m_valueChangeCb(getValue());
    }
}

// normalized value input 0-1
void Slider::setValue(const float val) {
    m_normValue       = val;
    m_scaledNormValue = getScaledVal(m_normValue);
    m_mappedValue     = val * (m_max - m_min) + m_min;

    m_knob->setX(m_normValue * m_maxDragWayRel);

    if (m_valueChangeCb) {
        m_valueChangeCb(getValue());
    }

    m_valueAsString = std::to_string(m_mappedValue);
}

// normalized value input 0-1
void Slider::setValuePtr(float* val) {
    m_normValue   = *val;
    m_mappedValue = m_normValue * (m_max - m_min) + m_min;
    m_valueAsString = std::to_string(m_mappedValue);

    m_knob->setX(m_normValue * m_maxDragWayRel);
}

}  // namespace ara
