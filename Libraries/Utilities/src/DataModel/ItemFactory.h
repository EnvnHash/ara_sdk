
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
