//
// Created by user on 09.03.2021.
//

#pragma once

#include <Property.h>
#include <UIElements/Label.h>
#include <UIElements/Slider.h>

namespace ara {

class PropSlider : public Div {
public:
    PropSlider();
    ~PropSlider() override = default;

    template <class T>
    void setProp(Property<T>* prop) {
        if (!prop) return;

        onChanged<T>(prop, [this, prop](std::any val) {
            m_Slider->setValue(static_cast<float>((std::any_cast<T>(val) - prop->getMin()) /
                                                  static_cast<float>(prop->getMax() - prop->getMin())));
            m_Edit->setValue(std::any_cast<T>(val));
        });

        if (typeid(T) == typeid(int)) {
            m_Edit->changeValType(UIEdit::num_int);
            m_Edit->addEnterCb([prop](const std::string& txt) { (*prop) = static_cast<T>(atoi(txt.c_str())); }, prop);
        } else if (typeid(T) == typeid(float) || typeid(T) == typeid(double)) {
            m_Edit->changeValType(UIEdit::num_fp);
            m_Edit->addEnterCb([prop](const std::string& txt) { (*prop) = static_cast<T>(atof(txt.c_str())); }, prop);
        }

        m_Edit->setMinMax(prop->getMin(), prop->getMax());
        m_Edit->setValue((*prop)());
        m_Edit->setStep(prop->getStep());

        m_Slider->addMouseDragCb([this, prop](hidData* data) {
            auto newValue = static_cast<T>(m_Slider->getValue() * (static_cast<float>(prop->getMax()) - static_cast<float>(prop->getMin())) +
                               static_cast<float>(prop->getMin()));
            newValue = static_cast<T>(static_cast<int>(static_cast<float>(newValue) / static_cast<float>(prop->getStep()))) * prop->getStep();
            if (!m_onMouseUpUpdtMode) {
                (*prop) = newValue;
            } else {
                m_Edit->setValue(newValue);
            }
            data->consumed = true;
        });

        m_Slider->getKnob()->addMouseUpCb([this, prop](hidData* data) {
            if (m_onMouseUpUpdtMode) {
                auto newValue = static_cast<T>(m_Slider->getValue() * (static_cast<float>(prop->getMax()) - static_cast<float>(prop->getMin())) +
                                   static_cast<float>(prop->getMin()));
                newValue = static_cast<T>(static_cast<int>(static_cast<float>(newValue) / static_cast<float>(prop->getStep()))) * prop->getStep();
                (*prop)  = newValue;
            }
        });
        m_Slider->setValue(static_cast<float>(((*prop)() - prop->getMin()) / static_cast<float>(prop->getMax() - prop->getMin())));
    }

    void setProp(Property<int>* prop) { setProp<int>(prop); }
    void setProp(Property<float>* prop) { setProp<float>(prop); }
    void setProp(Property<double>* prop) { setProp<double>(prop); }
    void setProp(Property<glm::vec2>* prop, int idx);
    void addStyleClass(const std::string& styleClass) override;

    void setLabel(const std::string& txt) const {
        if (m_label) {
            m_label->setText(txt);
        }
    }
    void setUseWheel(bool val) const {
        if (m_Edit) {
            m_Edit->setUseWheel(val);
        }
    }
    void setPrecision(int prec) const {
        if (m_Edit) {
            m_Edit->setPrecision(prec);
        }
    }
    void setSliderOnMouseUpUpdtMode(bool val) { m_onMouseUpUpdtMode = val; }
    void setValueChgCb(const std::function<void()>& f) { m_valChangeCb = f; }
    [[nodiscard]] UIEdit* getEdit() const { return m_Edit; }

private:
    Label*                m_label             = nullptr;
    Slider*               m_Slider            = nullptr;
    UIEdit*               m_Edit              = nullptr;
    bool                  m_onMouseUpUpdtMode = false;
    std::function<void()> m_valChangeCb;
};

}  // namespace ara
