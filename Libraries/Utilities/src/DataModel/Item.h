
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
/*!
 *\brief Base class for item. Provides and standard name property
 */

#pragma once

#include <Property.h>
#include <util_common.h>
#include <json/json.hpp>

namespace ara {

template <PropertyType T>
class PropertyItemUi;

class ItemFactory;

class Item {
public:
    Item() { setTypeName<Item>(); }

    virtual ~Item() = default;

    template <class T>
    T *add(std::unique_ptr<T> ptr) {
        std::unique_lock l(m_mtx);
        for (auto &[fst, func] : m_aboutAddItem) {
            func();
        }

        m_children.emplace_back(std::move(ptr));

        for (auto &[fst, func] : m_itemAdded) {
            func();
        }

        m_children.back()->parent        = this;
        m_children.back()->m_itemFactory = m_itemFactory;
        m_children.back()->onItemChanged(this, [this] { signalItemChanged(); });
        signalItemChanged();
        return static_cast<T *>(m_children.back().get());
    }

    // add item return a raw pointer to Object
    template <class T>
    T *add() {
        std::unique_lock l(m_mtx);
        for (auto &it : m_aboutAddItem | std::views::values) {
            it();
        }

        m_children.emplace_back(std::make_unique<T>());

        for (auto &it : m_itemAdded) {
            it.second();
        }

        m_children.back()->parent        = this;
        m_children.back()->m_itemFactory = m_itemFactory;
        m_children.back()->onItemChanged(this, [this] { signalItemChanged(); });
        signalItemChanged();
        return static_cast<T *>(m_children.back().get());
    }

    template <class T>
    T *add(const std::string &inName) {
        auto it  = Item::add<T>();
        it->name = inName;
        return it;
    }

    template <class T>
    Property<T> *getProp() {
        if (isPropertyItem) {
            if (typeid(T) == typeid(std::string)) {
                m_typeId = tpi::tp_string;
                return true;
            } else if (typeid(T) == typeid(const char *)) {
                m_typeId = tpi::tp_char;
                return true;
            } else if (typeid(T) == typeid(int)) {
                m_typeId = tpi::tp_int32;
                return true;
            } else if (typeid(T) == typeid(int32_t)) {
                m_typeId = tpi::tp_int32;
                return true;
            } else if (typeid(T) == typeid(unsigned int)) {
                m_typeId = tpi::tp_uint32;
                return true;
            } else if (typeid(T) == typeid(uint32_t)) {
                m_typeId = tpi::tp_uint32;
                return true;
            } else if (typeid(T) == typeid(float)) {
                m_typeId = tpi::tp_float;
                return true;
            } else if (typeid(T) == typeid(double)) {
                m_typeId = tpi::tp_double;
                return true;
            } else if (typeid(T) == typeid(int64_t)) {
                m_typeId = tpi::tp_int64;
                return true;
            } else if (typeid(T) == typeid(uint64_t)) {
                m_typeId = tpi::tp_uint64;
                return true;
            } else if (typeid(T) == typeid(bool)) {
                m_typeId = tpi::tp_bool;
                return true;
            }
        } else
            return nullptr;
    }

    template <class T>
    void remove(T *delIt) {
        if (!delIt) {
            return;
        }

        // std::unique_lock<std::mutex> l(m_mtx);
        for (auto &[fst, func] : m_aboutRemoveItem) {
            func();
        }
        auto it = std::find_if(m_children.begin(), m_children.end(),
                               [delIt](const std::unique_ptr<Item> &item) { return item.get() == delIt; });
        if (it != m_children.end()) {
            m_children.erase(it);
        }

        for (auto &[fst, func] : m_itemRemoved) {
            func();
        }
        signalItemChanged();
    }

    template <class T>
    void setTypeName() {
        // std::unique_lock<std::mutex> l(m_mtx);
        itemType = getTypeName<T>();
        signalItemChanged();
    }

    /** get all children of a specific type **/
    template <class T>
    std::list<T *> children() {
        // std::unique_lock<std::mutex> l(m_mtx);
        auto typeStr = getTypeName<T>();
        std::list<T *> outList;
        for (auto &it : m_children) {
            if (it && it->itemType() == typeStr) {
                outList.emplace_back(static_cast<T *>(it.get()));
            }
        }

        return std::move(outList);
    }

    template <class T>
    std::list<T *> findChildren() {
        // std::unique_lock<std::mutex> l(m_mtx);
        auto typeStr = getTypeName<T>();
        std::list<T *> outList;
        for (auto &it : m_children) {
            if (it->itemType() == typeStr) {
                outList.emplace_back(static_cast<T *>(it.get()));
            }
            outList.splice(outList.end(), it->findChildren<T>());
        }
        return std::move(outList);
    }

    // check for a parent of a specific type
    template <class T>
    T *findParent() {
        // std::unique_lock<std::mutex> l(m_mtx);
        auto currentParent = parent();
        while (currentParent != nullptr) {
            if (std::is_same_v<T*, decltype(currentParent)>) {
                return static_cast<T *>(currentParent);
            }
            currentParent = currentParent->parent();
        }
        return nullptr;
    }

    // iterate upwards until the next parent is nullptr -> all children have
    // parent != nullptr
    Item *getRoot() {
        // std::unique_lock<std::mutex> l(m_mtx);
        auto currentParent = parent();
        if (!currentParent) {
            return this;
        }
        while (currentParent->parent()) {
            currentParent = currentParent->parent();
        }
        return currentParent;
    }

    size_t size() const {
        // std::unique_lock<std::mutex> l(m_mtx);
        return m_children.size();
    }

    void saveState();
    void undo();
    void redo();

    virtual void load(const std::filesystem::path& filePath);
    virtual void saveAs(const std::filesystem::path& filePath);
    virtual void save();
    virtual void serializeProperties(nlohmann::json &node);
    virtual void parseProperties(nlohmann::json& node);
    virtual void serializeToJson(nlohmann::json &root);
    virtual void parseFromJson(nlohmann::json& node);

    void         onAboutAddItem(void *ptr, std::function<void()> func)      { m_aboutAddItem[ptr] = std::move(func); }
    void         onItemAdded(void *ptr, std::function<void()> func)         { m_itemAdded[ptr] = std::move(func); }
    void         onAboutItemRemove(void *ptr, std::function<void()> func)   { m_aboutRemoveItem[ptr] = std::move(func); }
    void         onItemRemoved(void *ptr, std::function<void()> func)       { m_itemRemoved[ptr] = std::move(func); }
    void         onItemChanged(void *ptr, std::function<void()> func)       { m_itemChanged[ptr] = std::move(func); }
    virtual void callPostChangeCb() {}

    /** utility function for recursive updates after an item is loaded */
    virtual void onLoaded() {
        for (const auto &it : m_children) {
            it->onLoaded();
        }
    }

    void removeAboutAddItemCb(void *ptr) {
        if (const auto c = m_aboutAddItem.find(ptr); c != m_aboutAddItem.end()) {
            m_aboutAddItem.erase(c);
        }
    }

    void removeItemAddedCb(void *ptr) {
        if (const auto c = m_itemAdded.find(ptr); c != m_itemAdded.end()) {
            m_itemAdded.erase(c);
        }
    }

    void removeAboutAddRemoveCb(void *ptr) {
        if (const auto c = m_aboutRemoveItem.find(ptr); c != m_aboutRemoveItem.end()) {
            m_aboutRemoveItem.erase(c);
        }
    }

    void removeItemRemovedCb(void *ptr) {
        if (const auto c = m_itemRemoved.find(ptr); c != m_itemRemoved.end()) {
            m_itemRemoved.erase(c);
        }
    }

    void removeItemChangedCb(void *ptr) {
        if (const auto c = m_itemChanged.find(ptr); c != m_itemChanged.end()) {
            m_itemChanged.erase(c);
        }
    }

    void signalItemChanged() const {
        for (const auto &[fst, func] : m_itemChanged) {
            func();
        }
    }

    void clearChildren() {
        m_children.clear();
        signalItemChanged();
    }

    virtual std::list<std::unique_ptr<Item>> &children() { return m_children; }
    virtual void setItemFactory(ItemFactory* fact) { m_itemFactory = fact; };

    Property<Item *>        parent = {};
    bool                    isPropertyItem = false;
    tpi                     m_typeId       = tpi::none;
    Property<std::string>   itemType;
    Property<std::string>   name = {"item_"};

protected:
    std::list<std::unique_ptr<Item>> m_children;  // use unique_ptr -> when iterating over a list of items,
                                                  // use get() to get a pointer to the derived class
    std::deque<nlohmann::json>              m_undoBuf;
    std::deque<nlohmann::json>::iterator    m_undoBufIt;
    std::filesystem::path                   m_fileName;
    ItemFactory*                            m_itemFactory = nullptr;

    std::unordered_map<void *, std::function<void()>> m_aboutAddItem;
    std::unordered_map<void *, std::function<void()>> m_itemAdded;
    std::unordered_map<void *, std::function<void()>> m_aboutRemoveItem;
    std::unordered_map<void *, std::function<void()>> m_itemRemoved;
    std::unordered_map<void *, std::function<void()>> m_itemChanged;

    size_t m_undoIdx        = 0;
    size_t m_maxUndoBufSize = 50;

    std::mutex m_mtx;
};


}  // namespace ara
