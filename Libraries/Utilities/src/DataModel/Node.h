
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

#include <DataModel/NodeMacros.h>

namespace ara {

class Node;

class NodeFactory {
public:
    NodeFactory() {
        registerClass("Node", [&] { return std::make_shared<Node>(); } );
    };

    NodeFactory(const NodeFactory&) = delete;

    [[nodiscard]] std::shared_ptr<Node> create(const std::string& className) const {
        if (const auto it = m_creators.find(className); it != m_creators.end()) {
            return it->second();
        }
        return nullptr;
    }

    void registerClass(const std::string& className, const std::function<std::shared_ptr<Node>()>& func) {
        m_creators[className] = func;
    }

    std::function<Node *()> operator = (const NodeFactory&) = delete;

private:
    std::unordered_map<std::string, std::function<std::shared_ptr<Node>()>> m_creators;
};

class NodeWatchFile {
public:
    Node* node = nullptr;
    std::filesystem::path path;
    std::filesystem::file_time_type time{};
    std::uintmax_t fileSize{};
};

class Node : public std::enable_shared_from_this<Node> {
public:
    ARA_NODE_ADD_VIRTUAL_SERIALIZE_FUNCTIONS(m_name, m_typeName, m_uuid)

    enum class cbType : int { preChange=0, postChange, preAddChild, postAddChild, preRemoveChild, postRemoveChild, Size };
    enum class pushToType : int { undefined=0, array, object };

    struct memberVar {
        std::function<std::any()>               get;
        std::function<void(std::any&, size_t)>  set;
        const tpi                               typeIndex;
    };

    Node();
    virtual ~Node();

    void prePush() {
        if (!m_undoBuf.empty()) {
            saveState();
        }
        signalChange(cbType::preAddChild, std::nullopt);
    }

    template <class T>
    T& push() {
        return push(std::make_shared<T>());
    }

    template <class T>
    T& push(const std::shared_ptr<T>& ptr) {
        prePush();
        {
            const bool unlock = m_mtx.try_lock();
            m_children.emplace_back(ptr);
            setDefault(m_children.back());
            if (unlock) {
                m_mtx.unlock();
            }
        }
        signalChange(cbType::postAddChild, std::make_optional<Node*>(m_children.back().get()));
        return static_cast<T&>(*m_children.back().get());
    }

    template <class T>
    T& push(std::shared_ptr<T>&& ptr) {
        prePush();
        const bool unlock = m_mtx.try_lock();
        m_children.emplace_back(ptr);
        setDefault(m_children.back());
        if (unlock) {
            m_mtx.unlock();
        }
        signalChange(cbType::postAddChild, std::make_optional<Node*>(m_children.back().get()));
        return static_cast<T&>(*m_children.back().get());
    }

    template <class T>
    T& insertChild(int32_t position) {
        return insertChild(position, std::make_shared<T>());
    }

    template <typename T>
    T& insertChild(const std::string& name) {
        return dynamic_cast<T&>(insertChild(name, std::make_shared<T>()));
    }

    template <class T>
    T& insertChild(const int position, std::shared_ptr<T>&& child) {
        if (!child) {
            LOGE << "Node::insertChild failed, child empty!";
        } else {
            auto it = m_children.insert(std::next(m_children.begin(), position), std::move(child));
            signalChange(cbType::postAddChild, std::make_optional(it->get()));
            return static_cast<T&>(*it->get());
        }
        return push<T>(child);
    }

    template <typename T>
    T& insertAfter(const std::string& name) {
        return dynamic_cast<T&>(insertAfter(name, std::make_shared<T>()));
    }

    template <class T>
    T& insertAfter(const std::string& name, std::shared_ptr<T>&& child) {
        if (!child) {
            LOGE << "Node::insertAfter failed, child empty!";
        } else {
            auto r = std::ranges::find_if(m_children, [&name](auto& it){
                return name == it->name();
            });

            if (r != m_children.end()
                && static_cast<int>(std::distance(m_children.begin(), r) +1) <= static_cast<int>(m_children.size())) {
                const auto it = m_children.insert(std::next(r, 1), std::move(child));
                signalChange(cbType::postAddChild, std::make_optional(it->get()));
                return dynamic_cast<T&>(*it->get());
            }
        }
        return push<T>(child);
    }

    template <class T>
    T& insertChild(const std::string& name, std::shared_ptr<T>&& child) {
        if (!child) {
            LOGE << "UINode::insertAfter failed, child empty!";
        } else {
            auto r = std::ranges::find_if(m_children, [&name](auto& it){
                return name == it->name();
            });

            if (r != m_children.end()) {
                const auto it = m_children.insert(r, std::move(child));
                signalChange(cbType::postAddChild, std::make_optional(it->get()));
                return dynamic_cast<T&>(*it->get());
            }
        }
        return push<T>(child);
    }

    void setDefault(const std::shared_ptr<Node> &child) {
        child->setUuid(generateUUID());
        child->setParent(this);
        if (m_undoBufRoot) {
            child->setUndoBufferRoot(m_undoBufRoot);
        }
    }

    template <class T>
    void setTypeName() {
        signalChange(cbType::preChange, std::nullopt);
        const bool unlock = m_mtx.try_lock();
        setTypeName(ara::getTypeName<T>());
        if (unlock) {
            m_mtx.unlock();
        }
        signalChange(cbType::postChange, std::nullopt);
    }

    Node* findParentByType(const std::string& typeStr) {
        Node* par = parent();
        const bool unlock = m_mtx.try_lock();
        while (par && par->typeName() != typeStr) {
            par = par->parent();
        }
        if (unlock) {
            m_mtx.unlock();
        }
        return par;
    }

    Node* findChildByUuid(const std::string& uuid) {
        Node* out = nullptr;
        iterateChildren(*root(), [&out, &uuid] (Node& node) {
            if (node.uuid() == uuid) {
                out = &node;
                return false;
            }
            return true;
        });
        return out;
    }

    Node* findChildByKey(const std::string& key) {
        Node* out = nullptr;
        iterateChildren(*root(), [&out, &key] (Node& node) {
            if (node.key() == key) {
                out = &node;
                return false;
            }
            return true;
        });
        return out;
    }

    template <class T>
    std::deque<T*> findChildrenByType() {
        std::deque<T*> outList;
        auto typeStr = getTypeName<T>();
        const bool unlock = m_mtx.try_lock();
        iterateChildren(*this, [&outList, &typeStr] (Node& node) {
            if (node.typeName() == typeStr) {
                outList.emplace_back(dynamic_cast<T*>(&node));
            }
            return true;
        });
        if (unlock) {
            m_mtx.unlock();
        }
        return std::move(outList);
    }

    template <class T>
    std::optional<T*> getFirstChildByType() {
        std::optional<T*> out{};
        auto typeStr = getTypeName<T>();
        const bool unlock = m_mtx.try_lock();
        iterateChildren(*this, [&typeStr, &out] (Node& node) {
            if (node.typeName() == typeStr) {
                out = dynamic_cast<T*>(&node);
                return false;
            }
            return true;
        });
        if (unlock) {
            m_mtx.unlock();
        }
        return out;
    }

    std::deque<Node*> findChildrenByTypes(const std::list<std::string>& typeStrList) {
        std::deque<Node*> outList;
        const bool unlock = m_mtx.try_lock();
        iterateChildren(*this, [&outList, &typeStrList] (Node& node) {
            const auto res = std::ranges::find_if(typeStrList, [&node](auto& it){
                return it == node.typeName();
            });
            if (res != typeStrList.end()) {
                outList.emplace_back(&node);
            }
        });
        if (unlock) {
            m_mtx.unlock();
        }
        return std::move(outList);
    }

    template <class T>
    T value() { return std::get<T>(m_nodeValue); }

    void                setValue(const nodeValue& val);
    void                pop();
    void                remove(Node&);
    void                remove(Node*);
    void                removeChangeCb(cbType, void *ptr);
    std::deque<Node*>   findChild(const std::string& name);
    void                clearChildren();
    nlohmann::json      asJson(bool skipClassEntries=false);
    void                serialize(nlohmann::json& json, bool skipClassEntries=false);
    void                serializePerClass(nlohmann::json& j, bool skipClassEntries);
    void                serializeNonClass(nlohmann::json& j, const pushToType pushTo = pushToType::undefined);
    nlohmann::json      serializeClassValues();
    nlohmann::json&     serializeNonClassValue(nlohmann::json& json, pushToType pushToArray = pushToType::undefined);
    void                deserialize(const std::string&, bool skipClassEntries=false);
    void                deserialize(const nlohmann::json& j, bool skipClassEntries=false,
                                    std::optional<std::list<std::function<void()>*>*> cbs = std::nullopt);
    void                parseClassChildren(const nlohmann::json& j, bool skipClassEntries,
                                           std::unordered_map<std::string, Node*>& existingChildren, std::list<std::function<void()>*>*);
    virtual void        parseNonClassEntries(const nlohmann::json& j, bool skipClassEntries,
                                             std::list<std::function<void()>*>* postLoadCbsArg);
    virtual void        load(const std::filesystem::path& filePath, bool skipNonClass=false);
    void                loadFromAssets(const std::filesystem::path& filePath, bool skipNonClass=false);
    virtual void        load();
    virtual void        load(bool fromAssets, bool skipNonClass=false);
    void                loadFromJson(const nlohmann::json& json, bool skipClassEntries=false);
    void                loadFromString(const std::string& str, bool skipClassEntries=false);
    void                saveAs(const std::filesystem::path& filePath);
    void                save(bool skipNonClass=false);
    void                saveState();
    void                undo();
    void                redo();
    void                signalChange(cbType cbType, std::optional<Node*> node);
    static bool         iterateChildren(Node& node, const std::function<void(Node&)>& f);
    bool                iterateChildren(const std::function<void(Node&)>& f) const;
    Node*               root();
    void                changeVal(const std::function<void()>& f);
    void                setUndoBuffer(bool enabled, size_t size);
    void                checkAndAddWatchPath(const std::string& fn);
    static void         checkWatchThreadRunning();
    virtual void        setWatch(bool val);
    static void         startWatchThread();
    static void         watchThreadIterate();
    static void         stopWatchThread();

    std::deque<std::function<void(std::optional<Node*>)>> collectCallbacks(cbType cbType, bool withChildrenOnly);

    auto&               mutex() { return m_mtx; }
    auto&               children() const { return const_cast<std::list<std::shared_ptr<Node>>&>(m_children); }
    Node*               parent() const { return m_parent; }
    const std::string&  typeName() { return m_typeName; }
    const auto&         name() { return m_name; }
    const std::string&  uuid() { return m_uuid; }
    const std::string&  key() { return m_key; }
    auto&               undoBufQueue() { return m_undoBuf; }
    static const auto&  getClassKeys() { return m_classKeys; }
    static void         clearClassKeys() { m_classKeys.clear(); }
    const auto&         getNodeValueType() const { return m_nodeValueType; }
    const auto&         getMemberVariables() const { return m_memberVars; }

    std::unordered_map<cbType, std::unordered_map<void*, std::function<void(std::optional<Node*>)>>>&   changeCb() { return m_changeCb; }

    void setName(const std::string& name)           { changeVal([&]{ m_name = name; }); }
    void setUuid(const std::string& uuid)           { m_uuid = uuid; }
    void setTypeName(const std::string& name)       { m_typeName = name; }
    void setParent(Node* ptr)                       { m_parent = ptr; }
    void setUndoBufferRoot(Node* node)              { m_undoBufRoot = node; }
    void setKey(const std::string& key)             { m_key = key; }
    void setNodeValueType(const nodeValueType& t)   { m_nodeValueType = t; }

    void setOnChangeCb(const cbType cbType, void *ptr, std::function<void(std::optional<Node*>)> func) {
        m_changeCb[cbType][ptr] = std::move(func);
    }

    static nlohmann::json getValues(const nlohmann::json& j) {
        nlohmann::json valueJson;
        for (const auto& [key, value] : j.items()) {
            if (!value.is_object() && !value.is_array()) {
                valueJson.emplace(key, value);
            }
        }
        return valueJson;
    }

    void callChangeCbs(const cbType cbType) {
        for (auto &cb: m_changeCb[cbType] | std::views::values) {
            cb(std::nullopt);
        }
    }

    static inline NodeFactory               m_factory;
    static inline std::thread               m_watchThrd;
    static inline std::mutex                m_watchMtx;
    static inline std::atomic<bool>         m_watchThreadRunning{false};
    static inline std::atomic<bool>         m_useWatchThread{true};
    static inline std::list<NodeWatchFile>  m_watchFiles;

protected:
    static void checkClassKeyEntry(const std::string& key, const std::vector<std::string>& newClassKeys) {
        if (!m_classKeys.contains(key)) {
            m_classKeys[key] = { {}, false };
        }

        if (m_classKeys[key].second) { // prevent permanent rebuild and unnecessary checks
            return;
        }

        // special case: m_children is not part of the variable serialization idiom
        if (key == "Node" && std::ranges::find(m_classKeys[key].first, "children") == m_classKeys[key].first.end()) {
            m_classKeys[key].first.emplace_back("children");
        }

        const auto keysOverlap = std::ranges::any_of(m_classKeys[key].first, [&newClassKeys](const auto& elem) {
            return std::ranges::find(newClassKeys, elem) != newClassKeys.end();
        });

        if (keysOverlap) {
            m_classKeys[key].second = true;
            return;
        }

        m_classKeys[key].first.insert(m_classKeys[key].first.end(), newClassKeys.begin(), newClassKeys.end());
    }

    static void addMemberVars(std::vector<std::string>::iterator) {}

    // Base case that handles when there are no remaining arguments
    static void serializeSingleClassValue(nlohmann::json&, std::vector<std::string>::iterator) {}

    template <typename T, typename... Args>
    void serializeSingleClassValue(nlohmann::json& j, std::vector<std::string>::iterator name, T&& arg, Args&&... args) {
        j[*name] = arg;
        addMemberVar(name, arg);
        serializeSingleClassValue(j, ++name, std::forward<Args>(args)...);  // Recursively call for the rest of the arguments
    }

    template <typename... Args>
    std::vector<std::string> splitAndSerializeClassValues(nlohmann::json& j, const std::string& inArgNames, Args&&... args) {
        auto names = node::splitMacroStringArgs(inArgNames);
        serializeSingleClassValue(j, names.begin(), std::forward<Args>(args)...);
        return names;
    }

    static void createSingleProp(std::vector<std::string>::iterator) {}

    template <typename NodeValueType, typename... Args>
    void createSingleProp( std::vector<std::string>::iterator name, NodeValueType&& arg, Args&&... args) {
        addMemberVar(name, arg);
        createSingleProp(++name, std::forward<Args>(args)...);  // Recursively call for the rest of the arguments
    }

    template <typename... Args>
    std::vector<std::string> splitAndCreateClassProps(const std::string& inArgNames, Args&&... args) {
        auto names = node::splitMacroStringArgs(inArgNames);
        createSingleProp(names.begin(), std::forward<Args>(args)...);
        return names;
    }

    // Base case that handles when there are no remaining arguments
    static void deserializeSingleClassValue(const nlohmann::json&, std::vector<std::string>::iterator) {}

    template <typename T, typename... Args>
    void deserializeSingleClassValue(const nlohmann::json& j, std::vector<std::string>::iterator name, T&& arg, Args&&... args) {
        if (j.contains(*name) && !j[*name].is_null()) {
            j.at(*name).get_to(arg);
        }
        addMemberVar(name, arg);
        deserializeSingleClassValue(j, ++name, std::forward<Args>(args)...);  // Recursively call for the rest of the arguments
    }

    template <typename... Args>
    std::vector<std::string> splitAndDeserializeClassValues(const nlohmann::json& j, const std::string& inArgNames, Args&&... args) {
        auto names = node::splitMacroStringArgs(inArgNames);
        deserializeSingleClassValue(j, names.begin(), std::forward<Args>(args)...);
        return names;
    }

    template <typename NodeValueType>
    void addMemberVar(const std::vector<std::string>::iterator name, NodeValueType&& arg) {
        using Container = std::decay_t<NodeValueType>;
        constexpr auto tpIndex = getTpi<Container>();

        m_memberVars.emplace(*name, memberVar{
            .get = [&]{ return arg; },
            .set = [&] (std::any& val, size_t idx) {
                if constexpr (tpIndex == tpi::tp_vector_float ||
                    tpIndex == tpi::tp_vector_int32 ||
                    tpIndex == tpi::tp_vector_string ||
                    tpIndex == tpi::tp_ivec2 ||
                    tpIndex == tpi::tp_ivec3 ||
                    tpIndex == tpi::tp_ivec4 ||
                    tpIndex == tpi::tp_vec2 ||
                    tpIndex == tpi::tp_vec3 ||
                    tpIndex == tpi::tp_vec4) {
                    using Elem = Container::value_type;
                    arg[idx] =  std::any_cast<Elem>(val);
                } else {
                    arg = std::any_cast<Container>(val);
                }
            },
            .typeIndex = tpIndex,
        });
    }

    template <typename T>
    void serializeByType(const pushToType& nv, nlohmann::json& j) {
        if (nv == pushToType::array) {
            serializeToArray<T>(j);
        } else if (nv == pushToType::object) {
            serializeToObject<T>(j);
        } else {
            serializeSingleValue<T>(j);
        }
    }

    template <typename T>
    void serializeSingleValue(nlohmann::json &j) {
        j[m_key] = get<T>(m_nodeValue);
    };

    template <typename T>
    void serializeToArray(nlohmann::json &j) {
        j.emplace_back(get<T>(m_nodeValue));
    };

    template <typename T>
    void serializeToObject(nlohmann::json &j) {
        j[m_key] = get<T>(m_nodeValue);
    };

    virtual Node& createNewElement() { return push<Node>(); }

    void parseArrayOrObjectChild(const nlohmann::json& value, const std::string& key, const bool skipClassEntries, std::list<std::function<void()>*>* postLoadCbsArg);
    void parseSingleValueChild(const nlohmann::json& value, const std::string& key);

    std::string                                     m_name;
    std::string                                     m_typeName;
    std::string                                     m_uuid;
    std::string                                     m_path;
    std::string                                     m_key;
    nodeValue                                       m_nodeValue;
    nodeValueType                                   m_nodeValueType{};
    bool                                            m_watch = false;
    bool                                            m_useAssetLoader = false;
    NodeWatchFile*                                  m_watchFile = nullptr;
    Node*                                           m_parent = nullptr; // can't use weak pointer, since can't create weak_ptr from this without previous shared_ptr creation
    std::mutex                                      m_mtx;
    std::list<std::shared_ptr<Node>>                m_children;
    std::filesystem::path                           m_fileName;
    std::filesystem::path                           m_fileNameForWatcher;
    std::atomic<bool>                               m_undoing{false};
    Node*                                           m_undoBufRoot = nullptr;
    std::deque<std::vector<std::uint8_t>>           m_undoBuf;
    std::deque<std::vector<std::uint8_t>>::iterator m_undoBufIt;
    size_t                                          m_maxUndoBufSize=0;
    std::optional<std::function<void()>>            m_postLoadCb;
    std::list<std::function<void()>*>               m_postCbList;
    std::unordered_map<std::string, memberVar>      m_memberVars;

    static inline std::unordered_map<std::string, std::pair<std::vector<std::string>, bool>> m_classKeys;
    static inline std::filesystem::file_time_type   m_initFt{};

    std::unordered_map<cbType, std::unordered_map<void*, std::function<void(std::optional<Node*>)>>> m_changeCb {
        { cbType::preChange, {}},
        { cbType::postChange, {}},
        { cbType::preAddChild, {}},
        { cbType::postAddChild, {}},
        { cbType::preRemoveChild, {}},
        { cbType::postRemoveChild, {}} };

};

}
