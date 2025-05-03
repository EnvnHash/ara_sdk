
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include "PropertyItemUi.h"

namespace ara {

class ItemFactory {
public:
    virtual std::unique_ptr<Item> Create(const std::string &className) {

        // create propertyItems, detect PropertyType from suffix
        // (PropertyName_0, PropertyName_1, etc)
        for (int i = 0; i < static_cast<int>(tpi::count); i++) {
            if (className == "PropertyItemUi_" + std::to_string(i)) {
                if (static_cast<tpi>(i) == tpi::tp_string)
                    return std::make_unique<PropertyItemUi<std::string>>();
                if (static_cast<tpi>(i) == tpi::tp_int8)
                    return std::make_unique<PropertyItemUi<int8_t>>();
                if (static_cast<tpi>(i) == tpi::tp_uint8)
                    return std::make_unique<PropertyItemUi<uint8_t>>();
                if (static_cast<tpi>(i) == tpi::tp_int16)
                    return std::make_unique<PropertyItemUi<int16_t>>();
                if (static_cast<tpi>(i) == tpi::tp_uint16)
                    return std::make_unique<PropertyItemUi<uint16_t>>();
                if (static_cast<tpi>(i) == tpi::tp_int32)
                    return std::make_unique<PropertyItemUi<int32_t>>();
                if (static_cast<tpi>(i) == tpi::tp_uint32)
                    return std::make_unique<PropertyItemUi<uint32_t>>();
                if (static_cast<tpi>(i) == tpi::tp_int64)
                    return std::make_unique<PropertyItemUi<int64_t>>();
                if (static_cast<tpi>(i) == tpi::tp_uint64)
                    return std::make_unique<PropertyItemUi<uint64_t>>();
                if (static_cast<tpi>(i) == tpi::tp_float)
                    return std::make_unique<PropertyItemUi<float>>();
                if (static_cast<tpi>(i) == tpi::tp_double)
                    return std::make_unique<PropertyItemUi<double>>();
                if (static_cast<tpi>(i) == tpi::tp_bool)
                    return std::make_unique<PropertyItemUi<bool>>();
            }
        }

        return std::make_unique<Item>();
    }
};

}  // namespace ara
