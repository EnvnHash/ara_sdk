
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
        auto it = m_creators.find(className);
        if (it != m_creators.end()) {
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

    Node();
    ~Node();

    template <class T>
    T& push() {
        prePush();
        {
            std::unique_lock l(m_mtx);
            m_children.emplace_back(std::make_shared<T>());
            setDefault(m_children.back());
        }
        signalChange(cbType::postAddChild);
        return static_cast<T&>(*m_children.back().get());
    }

    template <class T>
    T& push(std::shared_ptr<T>&& ptr) {
        prePush();
        {
            std::unique_lock l(m_mtx);
            m_children.emplace_back(ptr);
            setDefault(m_children.back());
        }
        signalChange(cbType::postAddChild);
        return static_cast<T&>(*m_children.back().get());
    }

    template <class T>
    T& push(const std::shared_ptr<T>& ptr) {
        prePush();
        {
            std::unique_lock l(m_mtx);
            m_children.emplace_back(ptr);
            setDefault(m_children.back());
        }
        signalChange(cbType::postAddChild);
        return static_cast<T&>(*m_children.back().get());
    }

    void prePush() {
        if (!m_undoBuf.empty()) {
            saveState();
        }
        signalChange(cbType::preAddChild);
    }

    void setDefault(std::shared_ptr<Node> &child) {
        child->setUuid(ara::generateUUID());
        child->setParent(this);
        if (m_undoBufRoot) {
            child->setUndoBufferRoot(m_undoBufRoot);
        }
    }

    template <class T>
    void setTypeName() {
        signalChange(cbType::preChange);
        {
            std::unique_lock<std::mutex> l(m_mtx);
            setTypeName(ara::getTypeName<T>());
        }
        signalChange(cbType::postChange);
    }

    std::deque<Node*> findChildrenByType(const std::string& typeStr) {
        std::deque<Node*> outList;
        {
            std::unique_lock<std::mutex> l(m_mtx);
            iterateChildren(*this, [&outList, &typeStr] (Node& node) {
                if (node.typeName() == typeStr) {
                    outList.emplace_back(&node);
                }
            });
        }
        return std::move(outList);
    }

    std::deque<Node*> findChildrenByTypes(const std::list<std::string>& typeStrList) {
        std::deque<Node*> outList;
        {
            std::unique_lock<std::mutex> l(m_mtx);
            iterateChildren(*this, [&outList, &typeStrList] (Node& node) {
                auto res = std::find_if(typeStrList.begin(), typeStrList.end(), [&node](auto& it){
                    return it == node.typeName();
                });
                if (res != typeStrList.end()) {
                    outList.emplace_back(&node);
                }
            });
        }
        return std::move(outList);
    }

    void                                    pop();
    void                                    remove(Node*);
    void                                    removeChangeCb(cbType, void *ptr);
    std::deque<Node*>                       findChild(const std::string& name);
    void                                    clearChildren();
    nlohmann::json                          asJson();
    void                                    serialize(nlohmann::json& json);
    nlohmann::json                          serializeValues();
    void                                    deserialize(const std::string&);
    void                                    deserialize(const nlohmann::json& j);
    void                                    load(const std::filesystem::path& filePath);
    void                                    loadFromAssets(const std::filesystem::path& filePath);
    virtual void                            load(bool fromAssets);
    void                                    loadFromString(const std::string& str);
    void                                    saveAs(const std::filesystem::path& filePath);
    void                                    save();
    void                                    saveState();
    void                                    undo();
    void                                    redo();
    void                                    signalChange(cbType cbType);
    std::deque<std::function<void()>>       collectCallbacks(cbType cbType, bool withChildrenOnly);
    static void                             iterateChildren(Node& node, const std::function<void(Node&)>& f);
    Node*                                   root();
    void                                    changeVal(const std::function<void()>& f);
    void                                    setUndoBuffer(bool enabled, size_t size);
    void                                    checkAndAddWatchPath(const std::string& fn);
    virtual void                            setWatch(bool val);
    static void                             startWatchThread();
    static void                             stopWatchThread();

    std::mutex&                             mutex() { return m_mtx; }
    std::list<std::shared_ptr<Node>>&       children() const { return const_cast<std::list<std::shared_ptr<Node>>&>(m_children); }
    Node*                                   parent() { return m_parent; }
    const std::string&                      typeName() { return m_typeName; }
    std::string&                            name() { return m_name; }
    std::string&                            uuid() { return m_uuid; }
    std::deque<std::vector<std::uint8_t>>&  undoBufQueue() { return m_undoBuf; }

    std::unordered_map<cbType, std::unordered_map<void*, std::function<void()>>>&   changeCb() { return m_changeCb; }

    void setName(const std::string& name)       { changeVal([&]{ m_name = name; }); }
    void setUuid(const std::string& uuid)       { m_uuid = uuid; }
    void setTypeName(const std::string& name)   { m_typeName = name; }
    void setParent(Node* ptr)                   { m_parent = ptr; }
    void setUndoBufferRoot(Node* node)          { m_undoBufRoot = node; }
    void setOnChangeCb(Node::cbType cbType, void *ptr, std::function<void()> func)  { m_changeCb[cbType][ptr] = std::move(func); }

    static nlohmann::json getValues(const nlohmann::json& j) {
        nlohmann::json valueJson;
        for (const auto& [key, value] : j.items()) {
            if (!value.is_object() && !value.is_array()) {
                valueJson.emplace(key, value);
            }
        }
        return valueJson;
    }

    static inline NodeFactory               m_factory;
    static inline std::thread               m_watchThrd;
    static inline std::mutex                m_watchMtx;
    static inline std::atomic<bool>         m_watchThreadRunning{false};
    static inline std::list<NodeWatchFile>  m_watchFiles;

protected:
    // values
    std::string                                     m_name;
    std::string                                     m_typeName;
    std::string                                     m_uuid;
    std::string                                     m_path;

    bool                                            m_watch = false;
    NodeWatchFile*                                  m_watchFile = nullptr;

    Node*                                           m_parent = nullptr; // can't use weak pointer, since can't create weak_ptr from this without previous shared_ptr creation
    std::mutex                                      m_mtx;
    std::list<std::shared_ptr<Node>>                m_children;
    std::filesystem::path                           m_fileName;
    std::atomic<bool>                               m_undoing{false};
    Node*                                           m_undoBufRoot = nullptr;
    std::deque<std::vector<std::uint8_t>>           m_undoBuf;
    std::deque<std::vector<std::uint8_t>>::iterator m_undoBufIt;
    size_t                                          m_maxUndoBufSize=0;

    std::unordered_map<cbType, std::unordered_map<void *, std::function<void()>>> m_changeCb {
        { cbType::preChange, {}},
        { cbType::postChange, {}},
        { cbType::preAddChild, {}},
        { cbType::postAddChild, {}},
        { cbType::preRemoveChild, {}},
        { cbType::postRemoveChild, {}} };

};

}
