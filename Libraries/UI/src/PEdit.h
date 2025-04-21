//
// Created by user on 08.04.2021.
//

#pragma once

#include <DataModel/ItemRef.h>
#include <Property.h>
#include <UIEdit.h>

namespace ara::proj {

class PEdit : public UIEdit, ItemRef {
public:
    PEdit(ItemUi *item = nullptr) : UIEdit(), ItemRef(item) {}
    virtual ~PEdit() {}

    void setProp(Property<std::string> *prop);
    void setProp(Property<std::filesystem::path> *prop);
    void setProp(Property<int> *prop);
    void setProp(Property<float> *prop);
    void setProp(Property<glm::ivec2> *prop, int idx);
    void setProp(Property<glm::vec2> *prop, int idx);
    void setProp(Property<glm::vec3> *prop, int idx);
    void setProp(Property<glm::ivec3> *prop, int idx);
};

}  // namespace ara::proj
