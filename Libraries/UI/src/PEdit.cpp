//
// Created by user on 08.04.2021.
//

#include <DataModel/PropertyItemUi.h>
#include "PEdit.h"

using namespace std;
using namespace glm;

namespace ara::proj {

void PEdit::setProp(Property<std::string>* prop) {
    ItemRef::onPreChange<std::string>(prop, [this](std::any val) { setText(std::any_cast<std::string>(val)); });
    addEnterCb(
        [this, prop](const std::string& txt) {
            if (m_item) {
                m_item->saveState();
            }
            *prop = txt;
        },
        prop);
    setOnLostFocusCb([this, prop] { *prop = m_Text; });
    setText((*prop)());
}

void PEdit::setProp(Property<std::filesystem::path>* prop) {
    ItemRef::onPreChange<std::filesystem::path>(
        prop, [this](std::any val) { setText(std::any_cast<std::filesystem::path>(val).string()); });
    addEnterCb(
        [this, prop](const std::filesystem::path& txt) {
            if (m_item) {
                m_item->saveState();
            }
            *prop = std::filesystem::path(txt);
        },
        prop);
    setOnLostFocusCb([this, prop] { *prop = std::filesystem::path(m_Text); });
    setText((*prop)().string());
}

void PEdit::setProp(Property<int>* prop) {
    ItemRef::onPreChange<int>(prop, [this](std::any val) { setText(std::to_string(std::any_cast<int>(val))); });
    addEnterCb(
        [this, prop](const std::string& txt) {
            if (m_item) m_item->saveState();
            (*prop) = atoi(txt.c_str());
        },
        prop);
    setOnLostFocusCb([this, prop] { (*prop) = getIntValue(); });
    setMinMax(prop->getMin(), prop->getMax());
    setStep(prop->getStep());
    setText(std::to_string((*prop)()));
    setUseWheel(true);
}

void PEdit::setProp(Property<float>* prop) {
    ItemRef::onPreChange<float>(prop, [this](std::any val) { setValue(std::any_cast<float>(val)); });
    addEnterCb(
        [this, prop](const std::string& txt) {
            if (m_item) m_item->saveState();
            *prop = (float)atof(txt.c_str());
        },
        prop);
    setOnLostFocusCb([this, prop] { *prop = (float)atof(m_Text.c_str()); });
    setMinMax(prop->getMin(), prop->getMax());
    setStep(prop->getStep());
    setValue((*prop)());
    setUseWheel(true);
}

void PEdit::setProp(Property<glm::ivec2>* prop, int idx) {
    ItemRef::onPreChange<glm::ivec2>(prop,
                                     [this, idx](std::any val) { setValue(std::any_cast<glm::ivec2>(val)[idx]); });
    addEnterCb(
        [this, prop, idx](const std::string& txt) {
            if (m_item) m_item->saveState();
            glm::ivec2 newVal = (*prop)();
            newVal[idx]       = atoi(txt.c_str());
            (*prop)           = newVal;
        },
        prop);
    setOnLostFocusCb([this, prop, idx] {
        glm::ivec2 newVal = (*prop)();
        newVal[idx]       = atoi(m_Text.c_str());
        (*prop)           = newVal;
    });

    setOpt(UIEdit::num_int);
    setMinMax(prop->getMin()[idx], prop->getMax()[idx]);
    setStep(prop->getStep()[idx]);
    setValue((*prop)()[idx]);
    setUseWheel(true);
}

void PEdit::setProp(Property<glm::vec2>* prop, int idx) {
    ItemRef::onPreChange<glm::vec2>(prop, [this, idx](std::any val) { setValue(std::any_cast<glm::vec2>(val)[idx]); });
    addEnterCb(
        [this, prop, idx](const std::string& txt) {
            if (m_item) m_item->saveState();
            glm::vec2 newVal = (*prop)();
            newVal[idx]      = (float)atof(txt.c_str());
            (*prop)          = newVal;
        },
        prop);
    setOnLostFocusCb([this, prop, idx] {
        glm::vec2 newVal = (*prop)();
        newVal[idx]      = (float)atof(m_Text.c_str());
        (*prop)          = newVal;
    });

    setMinMax(prop->getMin()[idx], prop->getMax()[idx]);
    setStep(prop->getStep()[idx]);
    setValue((*prop)()[idx]);
    setUseWheel(true);
}

void PEdit::setProp(Property<glm::vec3>* prop, int idx) {
    ItemRef::onPreChange<glm::vec3>(prop, [this, idx](std::any val) { setValue(std::any_cast<glm::vec3>(val)[idx]); });
    addEnterCb(
        [this, prop, idx](const std::string& txt) {
            if (m_item) m_item->saveState();
            glm::vec3 newVal = (*prop)();
            newVal[idx]      = (float)atof(txt.c_str());
            (*prop)          = newVal;
        },
        prop);
    setOnLostFocusCb([this, prop, idx] {
        glm::vec3 newVal = (*prop)();
        newVal[idx]      = (float)atof(m_Text.c_str());
        (*prop)          = newVal;
    });

    setMinMax(prop->getMin()[idx], prop->getMax()[idx]);
    setStep(prop->getStep()[idx]);
    setValue((*prop)()[idx]);
    setUseWheel(true);
}

void PEdit::setProp(Property<glm::ivec3>* prop, int idx) {
    ItemRef::onPreChange<glm::ivec3>(prop,
                                     [this, idx](std::any val) { setValue(std::any_cast<glm::ivec3>(val)[idx]); });
    addEnterCb(
        [this, prop, idx](const std::string& txt) {
            if (m_item) m_item->saveState();
            glm::ivec3 newVal = (*prop)();
            newVal[idx]       = (int)atoi(txt.c_str());
            (*prop)           = newVal;
        },
        prop);
    setOnLostFocusCb([this, prop, idx] {
        glm::ivec3 newVal = (*prop)();
        newVal[idx]       = (int)atoi(m_Text.c_str());
        (*prop)           = newVal;
    });

    setMinMax(prop->getMin()[idx], prop->getMax()[idx]);
    setStep(prop->getStep()[idx]);
    setValue((*prop)()[idx]);
    setUseWheel(true);
}

}  // namespace ara::proj