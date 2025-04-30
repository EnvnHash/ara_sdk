//
// Created by hahne on 30.04.2025.
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

#include "DataModel/Item.h"

namespace ara {

class ItemUi : public Item {
public:
    ItemUi() : Item() { setTypeName<ItemUi>(); }

    void serializeProperties(nlohmann::json &node) override;
    void parseProperties(nlohmann::json& node) override;

    ItemUi *getChildById(int id, Item *item = nullptr);

    Property<std::string> displayName = {"item_ui"}; ///> alternative name for displaying this Property in a GUI
    Property<int>         id = {};
    Property<bool>        visible = {true};

    template <class T>
    Property<T> *addProp(std::string inName, const std::string &dpName = "", bool isVisible = true) {
        auto it = add<PropertyItemUi<T>>(inName);
        if (!dpName.empty()) {
            it->displayName = dpName;
        }
        it->visible  = isVisible;
        it->itemType = it->itemType() + "_" + std::to_string(static_cast<int>(it->m_typeId));  // attach PropertyItem m_typeId
        signalItemChanged();
        return it->getPtr();
    }
};

template <PropertyType T>
class PropertyItemUi : public ItemUi {
public:
    PropertyItemUi() {
        isPropertyItem = true;
        setTypeName<PropertyItemUi>();
        connectPostChangeCb();
        m_typeId = tpiTypeMap[typeid(T)];
    }

    explicit PropertyItemUi(const std::string &inName) {
        name           = inName;
        isPropertyItem = true;
        connectPostChangeCb();
        m_typeId = tpiTypeMap[typeid(T)];
    }

    Property<T> *getPtr() {
        return &prop;
    }

    void connectPostChangeCb() {
        prop.onPostChange([this]() { signalItemChanged(); }, &prop);
    }

    void callPostChangeCb() override {
        prop.callOnPostChange();
    }

    void serializeProperties(nlohmann::json &node) override {
        Item::serializeProperties(node);
        node[name()] = prop();
    }

    void parseProperties(nlohmann::json &node) override {
        Item::parseProperties(node);
        prop = static_cast<T>(node[name()]);
    }

    Property<T> prop   = {};
};

}