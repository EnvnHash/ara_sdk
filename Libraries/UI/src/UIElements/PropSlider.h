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
            m_slider->setValue(static_cast<float>((std::any_cast<T>(val) - prop->getMin()) /
                                                  static_cast<float>(prop->getMax() - prop->getMin())));
            m_edit->setValue(std::any_cast<T>(val));
        });

        if (typeid(T) == typeid(int)) {
            m_edit->changeValType(UIEdit::num_int);
            m_edit->addEnterCb([prop](const std::string& txt) { (*prop) = static_cast<T>(atoi(txt.c_str())); }, prop);
        } else if (typeid(T) == typeid(float) || typeid(T) == typeid(double)) {
            m_edit->changeValType(UIEdit::num_fp);
            m_edit->addEnterCb([prop](const std::string& txt) { (*prop) = static_cast<T>(atof(txt.c_str())); }, prop);
        }

        m_edit->setMinMax(prop->getMin(), prop->getMax());
        m_edit->setValue((*prop)());
        m_edit->setStep(prop->getStep());

        m_slider->addMouseDragCb([this, prop](hidData& data) {
            auto newValue = static_cast<T>(
                m_slider->getValue() * (static_cast<float>(prop->getMax()) - static_cast<float>(prop->getMin())) +
                               static_cast<float>(prop->getMin()));
            newValue = static_cast<T>(static_cast<int>(static_cast<float>(newValue) / static_cast<float>(prop->getStep()))) * prop->getStep();
            if (!m_onMouseUpUpdtMode) {
                (*prop) = newValue;
            } else {
                m_edit->setValue(newValue);
            }
            data.consumed = true;
        });

        m_slider->getKnob()->addMouseUpCb([this, prop](hidData& data) {
            if (m_onMouseUpUpdtMode) {
                auto newValue = static_cast<T>(
                    m_slider->getValue() * (static_cast<float>(prop->getMax()) - static_cast<float>(prop->getMin())) +
                                   static_cast<float>(prop->getMin()));
                newValue = static_cast<T>(static_cast<int>(static_cast<float>(newValue) / static_cast<float>(prop->getStep()))) * prop->getStep();
                (*prop)  = newValue;
            }
        });
        m_slider->setValue(static_cast<float>(((*prop)() - prop->getMin()) / static_cast<float>(prop->getMax() - prop->getMin())));
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
        if (m_edit) {
            m_edit->setUseWheel(val);
        }
    }
    void setPrecision(int prec) const {
        if (m_edit) {
            m_edit->setPrecision(prec);
        }
    }
    void setSliderOnMouseUpUpdtMode(bool val) { m_onMouseUpUpdtMode = val; }
    void setValueChgCb(const std::function<void()>& f) { m_valChangeCb = f; }
    [[nodiscard]] UIEdit* getEdit() const { return m_edit; }

private:
    Label*                m_label             = nullptr;
    Slider*               m_slider            = nullptr;
    UIEdit*               m_edit              = nullptr;
    bool                  m_onMouseUpUpdtMode = false;
    std::function<void()> m_valChangeCb;
};

}  // namespace ara
