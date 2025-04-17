/*!
 *\brief Base class for item. Provides and standard name property
 */

#pragma once

#include <Property.h>
#include <util_common.h>
#include <pugixml.hpp>
#include <json/json.hpp>

namespace ara {
using stdVar =  std::variant<float, double, int32_t, uint32_t, std::string, const char *, int64_t, uint64_t, bool>;

template <typename T>
class PropertyItemUi;

class ItemFactory;

class Item {
public:
    Item() { setTypeName<Item>(); }

    virtual ~Item() = default;

    template <class T>
    T *add(std::unique_ptr<T> ptr) {
        std::unique_lock<std::mutex> l(m_mtx);
        for (auto &it : m_aboutAddItem) {
            it.second();
        }

        m_children.emplace_back(std::move(ptr));

        for (auto &it : m_itemAdded) {
            it.second();
        }

        m_children.back()->parent        = this;
        m_children.back()->m_itemFactory = m_itemFactory;
        m_children.back()->onItemChanged(this, [this] { signalItemChanged(); });
        signalItemChanged();
        return (T *)m_children.back().get();
    }

    // add item return a raw pointer to Object
    template <class T>
    T *add() {
        std::unique_lock<std::mutex> l(m_mtx);
        for (auto &it : m_aboutAddItem) {
            it.second();
        }

        m_children.emplace_back(std::make_unique<T>());

        for (auto &it : m_itemAdded) {
            it.second();
        }

        m_children.back()->parent        = this;
        m_children.back()->m_itemFactory = m_itemFactory;
        m_children.back()->onItemChanged(this, [this] { signalItemChanged(); });
        signalItemChanged();
        return (T *)m_children.back().get();
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
        if (!delIt) return;

        // std::unique_lock<std::mutex> l(m_mtx);
        for (auto &it : m_aboutRemoveItem) it.second();
        auto it = std::find_if(m_children.begin(), m_children.end(),
                               [delIt](const std::unique_ptr<Item> &it) { return it.get() == delIt; });
        if (it != m_children.end()) {
            m_children.erase(it);
        }

        for (auto &itR : m_itemRemoved) {
            itR.second();
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
                outList.emplace_back((T *)it.get());
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
                outList.emplace_back((T *)it.get());
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
            if (std::is_same<T*, decltype(currentParent)>::value) {
                return (T *)currentParent;
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

    size_t size() {
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
        for (auto &it : m_children) it->onLoaded();
    }

    void removeAboutAddItemCb(void *ptr) {
        auto c = m_aboutAddItem.find(ptr);
        if (c != m_aboutAddItem.end()) {
            m_aboutAddItem.erase(c);
        }
    }

    void removeItemAddedCb(void *ptr) {
        auto c = m_itemAdded.find(ptr);
        if (c != m_itemAdded.end()) {
            m_itemAdded.erase(c);
        }
    }

    void removeAboutAddRemoveCb(void *ptr) {
        auto c = m_aboutRemoveItem.find(ptr);
        if (c != m_aboutRemoveItem.end()) {
            m_aboutRemoveItem.erase(c);
        }
    }

    void removeItemRemovedCb(void *ptr) {
        auto c = m_itemRemoved.find(ptr);
        if (c != m_itemRemoved.end()) {
            m_itemRemoved.erase(c);
        }
    }

    void removeItemChangedCb(void *ptr) {
        auto c = m_itemChanged.find(ptr);
        if (c != m_itemChanged.end()) {
            m_itemChanged.erase(c);
        }
    }

    void signalItemChanged() {
        for (auto &it : m_itemChanged) {
            it.second();
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
        it->itemType = it->itemType() + "_" + std::to_string((int)it->m_typeId);  // attach PropertyItem m_typeId
        signalItemChanged();
        return it->getPtr();
    }
};

template <typename T>
class PropertyItemUi : public ItemUi {
public:
    PropertyItemUi() : ItemUi() {
        isPropertyItem = true;
        setTypeName<PropertyItemUi>();
        m_isStdType = isStdType();
        connectPostChangeCb();
    }

    explicit PropertyItemUi(const std::string &inName) : ItemUi() {
        name           = inName;
        isPropertyItem = true;
        m_isStdType    = isStdType();
        connectPostChangeCb();
    }

    Property<T> *getPtr() {
        if (m_typeId == tpi::tp_string)
            return reinterpret_cast<Property<T> *>(&stdString);
        else if (m_typeId == tpi::tp_int32)
            return reinterpret_cast<Property<T> *>(&stdInt);
        else if (m_typeId == tpi::tp_uint32)
            return reinterpret_cast<Property<T> *>(&stdUInt);
        else if (m_typeId == tpi::tp_float)
            return reinterpret_cast<Property<T> *>(&stdFloat);
        else if (m_typeId == tpi::tp_double)
            return reinterpret_cast<Property<T> *>(&stdDouble);
        else if (m_typeId == tpi::tp_int64)
            return reinterpret_cast<Property<T> *>(&stdLong);
        else if (m_typeId == tpi::tp_uint64)
            return reinterpret_cast<Property<T> *>(&stdUlong);
        else if (m_typeId == tpi::tp_bool)
            return reinterpret_cast<Property<T> *>(&stdBool);
        else
            return nullptr;
    }

    bool isStdType() {
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
        } else if (typeid(T) == typeid(long long)) {
            m_typeId = tpi::tp_int64;
            return true;
        } else if (typeid(T) == typeid(int64_t)) {
            m_typeId = tpi::tp_int64;
            return true;
        } else if (typeid(T) == typeid(unsigned long long)) {
            m_typeId = tpi::tp_uint64;
            return true;
        } else if (typeid(T) == typeid(uint64_t)) {
            m_typeId = tpi::tp_uint64;
            return true;
        } else if (typeid(T) == typeid(bool)) {
            m_typeId = tpi::tp_bool;
            return true;
        } else
            return false;
    }

    void connectPostChangeCb() {
        if (m_typeId == tpi::tp_string)
            stdString.onPostChange([this]() { signalItemChanged(); }, &stdString);
        else if (m_typeId == tpi::tp_int32)
            stdInt.onPostChange([this]() { signalItemChanged(); }, &stdInt);
        else if (m_typeId == tpi::tp_uint32)
            stdUInt.onPostChange([this]() { signalItemChanged(); }, &stdUInt);
        else if (m_typeId == tpi::tp_float)
            stdFloat.onPostChange([this]() { signalItemChanged(); }, &stdFloat);
        else if (m_typeId == tpi::tp_double)
            stdDouble.onPostChange([this]() { signalItemChanged(); }, &stdDouble);
        else if (m_typeId == tpi::tp_int64)
            stdLong.onPostChange([this]() { signalItemChanged(); }, &stdLong);
        else if (m_typeId == tpi::tp_uint64)
            stdUlong.onPostChange([this]() { signalItemChanged(); }, &stdUlong);
        else if (m_typeId == tpi::tp_bool)
            stdBool.onPostChange([this]() { signalItemChanged(); }, &stdBool);
    }

    void callPostChangeCb() override {
        if (m_typeId == tpi::tp_string)
            stdString.callOnPostChange();
        else if (m_typeId == tpi::tp_int32)
            stdInt.callOnPostChange();
        else if (m_typeId == tpi::tp_uint32)
            stdUInt.callOnPostChange();
        else if (m_typeId == tpi::tp_float)
            stdFloat.callOnPostChange();
        else if (m_typeId == tpi::tp_double)
            stdDouble.callOnPostChange();
        else if (m_typeId == tpi::tp_int64)
            stdLong.callOnPostChange();
        else if (m_typeId == tpi::tp_uint64)
            stdUlong.callOnPostChange();
        else if (m_typeId == tpi::tp_bool)
            stdBool.callOnPostChange();
    }

    void serializeProperties(nlohmann::json &node) override {
        Item::serializeProperties(node);

        if (m_isStdType) {
            if (m_typeId == tpi::tp_string)
                node[name()] = stdString();
            else if (m_typeId == tpi::tp_int32)
                node[name] = stdInt();
            else if (m_typeId == tpi::tp_uint32)
                node[name] = stdUInt();
            else if (m_typeId == tpi::tp_float)
                node[name()] = stdFloat();
            else if (m_typeId == tpi::tp_double)
                node[name()] = stdDouble();
            else if (m_typeId == tpi::tp_int64)
                node[name()] = stdLong();
            else if (m_typeId == tpi::tp_uint64)
                node[name()] = stdUlong();
            else if (m_typeId == tpi::tp_bool)
                node[name()] = stdBool();
        }
    }

    void parseProperties(nlohmann::json &node) override {
        Item::parseProperties(node);

        if (m_isStdType) {
            if (m_typeId == tpi::tp_string) {
                stdString = static_cast<std::string>(node[name()]);
            } else if (m_typeId == tpi::tp_int32) {
                stdInt = static_cast<int32_t>(node[name()]);
            } else if (m_typeId == tpi::tp_uint32) {
                stdUInt = static_cast<uint32_t>(node[name()]);
            } else if (m_typeId == tpi::tp_float) {
                stdFloat = static_cast<float>(node[name()]);
            } else if (m_typeId == tpi::tp_double) {
                stdDouble = static_cast<double>(node[name()]);
            } else if (m_typeId == tpi::tp_int64) {
                stdLong = static_cast<int64_t>(node[name()]);
            } else if (m_typeId == tpi::tp_uint64) {
                stdUlong = static_cast<uint64_t>(node[name()]);
            } else if (m_typeId == tpi::tp_bool) {
                stdBool = static_cast<bool>(node[name()]);
            }
        }
    }

    bool m_isStdType = false;

    Property<int32_t>       stdInt   = {0, 0, 0, 0};
    Property<uint32_t>      stdUInt  = {0, 0, 0, 0};
    Property<float>         stdFloat = {0.f, 0.f, 0.f, 0.f};
    Property<std::string>   stdString;
    Property<double>        stdDouble = {0.0, 0.0, 0.0, 0.0};
    Property<int64_t>       stdLong   = {0, 0, 0, 0};
    Property<uint64_t>      stdUlong  = {0, 0, 0, 0};
    Property<bool>          stdBool   = {false};
};

}  // namespace ara
