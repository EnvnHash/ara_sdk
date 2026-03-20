//
// Created by user on 09.03.2021.
//

#include "PropSlider.h"

using namespace std;
using namespace glm;

namespace ara {

PropSlider::PropSlider() {
    setTypeName<PropSlider>();
    setName(getTypeName<PropSlider>());
    setFocusAllowed(false);
    setScissorChildren(true);

    m_label = &push<Label>({ .style = getStyleClass()+ ".label" });
    m_label->setWidth(0.15f);
    m_label->setTextAlign(align::center, valign::center);
    m_label->setFontSize(20);
    m_label->setBorderWidth(1);
    m_label->setBorderColor(1.f, 1.f, 1.f, 0.5f);

    m_slider = &push<Slider>();
    m_slider->setAlign(align::left, valign::center);
    m_slider->setValue(0.f);
    m_slider->setX(0.16f);
    m_slider->setWidth(0.68f);

    m_edit = &push<UIEdit>(UIEdit::num_fp);
    m_edit->setAlign(align::right, valign::center);
    m_edit->setBorderWidth(1);
    m_edit->setBorderColor(1.f, 1.f, 1.f, 0.5f);
    m_edit->setWidth(0.15f);
    m_edit->setUseWheel(true);
    m_edit->setOnLostFocusCb([this] {
        if (m_onLostFocusCb) {
            m_onLostFocusCb();
        }
    });

    m_slider->setNumEdit(m_edit);
}

void PropSlider::setProp(Property<vec2>& prop, int idx) {
    // update elements when property changes
    onChanged<glm::vec2>(prop, [this, &prop, idx](const std::any &val) {
        auto v = std::any_cast<glm::vec2>(val);
        m_slider->setValue((v[idx] - prop.getMin()[idx]) / (prop.getMax()[idx] - prop.getMin()[idx]));
        m_edit->setValue(v[idx]);
        if (m_valChangeCb) {
            m_valChangeCb();
        }
    });

    m_edit->changeValType(UIEdit::num_fp);

    // update property on element changes
    m_edit->addEnterCb(
        [&prop, idx](const std::string& txt) {
            auto lastVal = prop();
            lastVal[idx] = static_cast<float>(atof(txt.c_str()));
            prop         = lastVal;  // to be done this way in order to cause a onPreChange()
        }, &prop);

    m_slider->addMouseDragCb([this, &prop, idx](hidData& data) {
        float newVal = m_slider->getValue() * (prop.getMax()[idx] - prop.getMin()[idx]) + prop.getMin()[idx];
        if (!m_onMouseUpUpdtMode) {
            auto lastVal = prop();
            lastVal[idx] = newVal;
            prop         = lastVal;  // to be done this way in order to cause a onPreChange()
        } else {
            m_edit->setValue(newVal);
        }
    });
    m_slider->setValue((prop()[idx] - prop.getMin()[idx]) / (prop.getMax()[idx] - prop.getMin()[idx]));
    m_slider->getKnob()->addMouseUpCb([this, &prop, idx](hidData& data) {
        if (m_onMouseUpUpdtMode) {
            auto lastVal = prop();
            lastVal[idx] = m_slider->getValue() * (prop.getMax()[idx] - prop.getMin()[idx]) + prop.getMin()[idx];
            prop      = lastVal;  // to be done this way in order to cause a onPreChange()
        }
    });

    m_edit->setMinMax(prop.getMin()[idx], prop.getMax()[idx]);
    m_edit->setStep(prop.getStep()[idx]);
    m_edit->setValue(prop()[idx]);
}

void PropSlider::addStyleClass(const std::string& styleClass) {
    UINode::addStyleClass(styleClass);

    if (m_edit && m_slider && m_label) {
        m_edit->addStyleClass(getStyleClass() + ".edit");
        m_slider->addStyleClass(getStyleClass() + ".slider");
        m_label->addStyleClass(getStyleClass() + ".label");
    }
}

}  // namespace ara