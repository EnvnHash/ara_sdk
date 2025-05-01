//
// Created by user on 08.04.2021.
//

#pragma once

#include <DataModel/ItemRef.h>
#include <DataModel/PropertyItemUi.h>
#include <Property.h>
#include <UIEdit.h>

namespace ara::proj {

template<typename T>
concept PEditPropTypeSingle = std::is_same_v<T, std::string> || std::is_same_v<T, std::filesystem::path> || std::is_integral_v<T> || std::is_floating_point_v<T>;

template<typename T>
concept PEditPropTypeVec = std::is_same_v<T, glm::vec2> || std::is_same_v<T, glm::ivec2> || std::is_same_v<T, glm::vec3> || std::is_same_v<T, glm::ivec3> || std::is_same_v<T, glm::vec4> || std::is_same_v<T, glm::ivec4>;

class PEdit : public UIEdit, ItemRef {
public:
    PEdit(ItemUi *item = nullptr) : UIEdit(), ItemRef(item) {}
    virtual ~PEdit() {}

    template<PEditPropTypeSingle T>
    void setProp(Property<T>* prop) {
        ItemRef::onPreChange<T>(prop, [this](std::any val) { setText(std::any_cast<T>(val)); });
        addEnterCb([this, prop](const T& txt) {
            if (m_item) {
                m_item->saveState();
            }
            *prop = txt;
        }, prop);
        setOnLostFocusCb([this, prop] { *prop = m_Text; });
        setText((*prop)());
        if (typeid(T) != typeid(std::string) && typeid(T) != typeid(std::filesystem::path)) {
            setMinMax(prop->getMin(), prop->getMax());
            setStep(prop->getStep());
            setText(std::to_string((*prop)()));
            setUseWheel(true);
        }
    }

    template<PEditPropTypeVec T>
    void setProp(Property<T>* prop, int idx) {
        ItemRef::onPreChange<T>(prop, [this, idx](std::any val) { setValue(std::any_cast<T>(val)[idx]); });
        addEnterCb([this, prop, idx](const std::string& txt) {
            if (m_item) {
                m_item->saveState();
            }
            T newVal = (*prop)();
            newVal[idx]       = (int)atoi(txt.c_str());
            (*prop)           = newVal;
        }, prop);

        setOnLostFocusCb([this, prop, idx] {
            T newVal = (*prop)();
            newVal[idx]       = (int)atoi(m_Text.c_str());
            (*prop)           = newVal;
        });

        setMinMax(prop->getMin()[idx], prop->getMax()[idx]);
        setStep(prop->getStep()[idx]);
        setValue((*prop)()[idx]);
        setUseWheel(true);
    }

    void setProp(Property<glm::ivec2> *prop, int idx);
    void setProp(Property<glm::vec2> *prop, int idx);
    void setProp(Property<glm::vec3> *prop, int idx);
    void setProp(Property<glm::ivec3> *prop, int idx);
};

}  // namespace ara::proj
