#pragma once

#include "Item.h"

namespace ara {

class ItemFactory {
public:
    virtual std::unique_ptr<Item> Create(const std::string &className) {
        // create propertyItems, detect PropertyType from suffix
        // (PropertyName_0, PropertyName_1, etc)
        for (int i = 0; i < (int)tpi::count; i++) {
            if (className == "PropertyItemUi_" + std::to_string(i)) {
                if ((tpi)i == tpi::tp_string)
                    return std::make_unique<PropertyItemUi<std::string>>();
                else if ((tpi)i == tpi::tp_char)
                    return std::make_unique<PropertyItemUi<const char *>>();
                else if ((tpi)i == tpi::tp_int32)
                    return std::make_unique<PropertyItemUi<int32_t>>();
                else if ((tpi)i == tpi::tp_uint32)
                    return std::make_unique<PropertyItemUi<uint32_t>>();
                else if ((tpi)i == tpi::tp_float)
                    return std::make_unique<PropertyItemUi<float>>();
                else if ((tpi)i == tpi::tp_double)
                    return std::make_unique<PropertyItemUi<double>>();
                else if ((tpi)i == tpi::tp_int64)
                    return std::make_unique<PropertyItemUi<long long>>();
                else if ((tpi)i == tpi::tp_uint64)
                    return std::make_unique<PropertyItemUi<unsigned long long>>();
                else if ((tpi)i == tpi::tp_bool)
                    return std::make_unique<PropertyItemUi<bool>>();
            }
        }

        return std::make_unique<Item>();
    }
};

}  // namespace ara
