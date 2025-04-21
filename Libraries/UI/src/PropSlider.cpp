//
// Created by user on 09.03.2021.
//

#include "PropSlider.h"

using namespace std;
using namespace glm;

namespace ara {

PropSlider::PropSlider() : Div() {
    setName(getTypeName<PropSlider>());
    setFocusAllowed(false);
    setScissorChildren(true);
    addStyleClass("propertySlider");  // NOTE: there must be a valid entry of
                                      // this in the res.txt !!!!

    m_label = addChild<Label>(getStyleClass() + ".label");
    m_label->setWidth(0.15f);
    m_label->setTextAlignX(align::left);
    m_label->setFontSize(21);

    m_Slider = addChild<Slider>(getStyleClass() + ".slider");
    m_Slider->setAlign(align::left, valign::center);
    m_Slider->setKnobProportion(.8f);
    m_Slider->setValue(0.f);

    m_Edit = (UIEdit*)addChild(make_unique<UIEdit>(UIEdit::num_fp));
    m_Edit->addStyleClass(getStyleClass() + ".edit");
    m_Edit->setUseWheel(true);
    m_Edit->setOnLostFocusCb([this] {
        if (m_onLostFocusCb) m_onLostFocusCb();
    });

    m_Slider->setNumEdit(m_Edit);
}

void PropSlider::setProp(Property<glm::vec2>* prop, int idx) {
    if (!prop) return;

    // update elements when property changes
    onChanged<glm::vec2>(prop, [this, prop, idx](std::any val) {
        glm::vec2 v = std::any_cast<glm::vec2>(val);
        m_Slider->setValue((v[idx] - prop->getMin()[idx]) / (prop->getMax()[idx] - prop->getMin()[idx]));
        m_Edit->setValue(v[idx]);
        if (m_valChangeCb) {
            m_valChangeCb();
        }
    });

    m_Edit->changeValType(UIEdit::num_fp);

    // update property on element changes
    m_Edit->addEnterCb(
        [prop, idx](const std::string& txt) {
            glm::vec2 lastVal = (*prop)();
            lastVal[idx]      = (float)atof(txt.c_str());
            (*prop)           = lastVal;  // to be done this way in order to cause a onPreChange()
        },
        prop);

    m_Slider->addMouseDragCb([this, prop, idx](hidData* data) {
        float newVal = m_Slider->getValue() * (prop->getMax()[idx] - prop->getMin()[idx]) + prop->getMin()[idx];
        if (!m_onMouseUpUpdtMode) {
            glm::vec2 lastVal = (*prop)();
            lastVal[idx]      = newVal;
            (*prop)           = lastVal;  // to be done this way in order to cause a onPreChange()
        } else {
            m_Edit->setValue(newVal);
        }
    });
    m_Slider->setValue(((*prop)()[idx] - prop->getMin()[idx]) / (prop->getMax()[idx] - prop->getMin()[idx]));
    m_Slider->getKnob()->addMouseUpCb([this, prop, idx](hidData* data) {
        if (m_onMouseUpUpdtMode) {
            glm::vec2 lastVal = (*prop)();
            lastVal[idx] = m_Slider->getValue() * (prop->getMax()[idx] - prop->getMin()[idx]) + prop->getMin()[idx];
            (*prop)      = lastVal;  // to be done this way in order to cause a onPreChange()
        }
    });

    m_Edit->setMinMax(prop->getMin()[idx], prop->getMax()[idx]);
    m_Edit->setStep(prop->getStep()[idx]);
    m_Edit->setValue((*prop)()[idx]);
}

void PropSlider::addStyleClass(std::string&& styleClass) {
    UINode::addStyleClass(std::move(styleClass));

    if (m_Edit && m_Slider && m_label) {
        m_Edit->addStyleClass(getStyleClass() + ".edit");
        m_Slider->addStyleClass(getStyleClass() + ".slider");
        m_label->addStyleClass(getStyleClass() + ".label");
    }
}

}  // namespace ara