//
// Created by user on 01.04.2021.
//

#pragma once

#include <ComboBox.h>

#include <magic_enum.hpp>

#include "CameraCalibration.h"
#include "DataModel/ItemRef.h"
#include "ItemConnectable.h"

namespace ara::proj {

class PropertyComboBox : public ComboBox, public ItemRef {
public:
    PropertyComboBox() : ComboBox(), ItemRef() {}

    template <class T>
    void setPropEnum(Property<T>* prop) {
        // enum case, create an entry for each entry in the enum
        if (std::is_enum<T>::value) {
            clearEntries();
            for (int i = 0; i < magic_enum::enum_count<T>(); i++) {
                addEntry(std::string(magic_enum::enum_name((T)i)), [this, prop, i] { (*prop) = (T)i; });
            }
        }
        setMenuName(std::string(magic_enum::enum_name((*prop)())));
        menuNameUpdtEnum<T>(prop);
    }

    template <typename T>
    void setProp(Property<T>* prop, std::vector<T> initList, int precision) {
        // enum case, create an entry for each entry in the enum
        clearEntries();
        for (int i = 0; i < initList.size(); i++) {
            T val = initList[i];
            addEntry(to_string_prec(initList[i], precision), [this, prop, val] { (*prop) = val; });
        }

        setMenuName(to_string_prec((*prop)(), precision));
        menuNameUpdt<T>(prop, precision);
    }

    template <typename T>
    void menuNameUpdt(Property<T>* prop, int precision) {
        ItemRef::onChanged<T>(prop, [this, precision](std::any val) {
            T rawVal = std::any_cast<T>(val);
            if (typeid(T) == typeid(float)) {
                setMenuName(to_string_prec(rawVal, precision));
            }
        });
    }

    template <class T>
    void menuNameUpdtEnum(Property<T>* prop) {
        ItemRef::onChanged<T>(prop, [this](std::any val) {
            T rawVal = std::any_cast<T>(val);
            setMenuName(std::string(magic_enum::enum_name(rawVal)));
        });
    }

    template <typename T>
    std::string to_string_prec(const T a_value, const int n = 6) {
        std::ostringstream out;
        out.precision(n);
        out << std::fixed << a_value;
        return out.str();
    }

private:
};

}  // namespace ara::proj
