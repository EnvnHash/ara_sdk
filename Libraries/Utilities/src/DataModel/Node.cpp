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

#include <DataModel/Node.h>
#include "AssetLoader.h"
#include "UIElements/DataBinding/JsonEditor.h"

using json = nlohmann::json;
using namespace std::chrono_literals;
using namespace std;

namespace ara {

Node::Node() {
    setTypeName<Node>();
}

Node::~Node() {
    if (!m_fileName.empty()) {
        const auto r = ranges::find_if(m_watchFiles, [&](auto& it) {
            return it.path.string() == m_fileName;
        });
        if (r != m_watchFiles.end()) {
            const auto canLock = m_watchMtx.try_lock();
            m_watchFiles.erase(r);
            if (canLock) {
                m_watchMtx.unlock();
            }
        }
    }
}

void Node::setValue(const nodeValue& val) {
    m_nodeValue = std::move(val);
    if (std::holds_alternative<int32_t>(m_nodeValue)) {
        m_nodeValueType = nodeValueType::integer;
    } else if (std::holds_alternative<float>(m_nodeValue)) {
        m_nodeValueType = nodeValueType::floating;
    } else if (std::holds_alternative<std::string>(m_nodeValue)) {
        m_nodeValueType = nodeValueType::string;
    } else if (std::holds_alternative<bool>(m_nodeValue)) {
        m_nodeValueType = nodeValueType::boolean;
    }
}

void Node::pop() {
    if (m_undoBufRoot) {
        m_undoBufRoot->saveState();
    }

    if (!m_children.empty()) {
        const auto preRemoveCbs = collectCallbacks(cbType::preRemoveChild, true);
        const auto postRemoveCbs = collectCallbacks(cbType::postRemoveChild, true);
        for (auto &it : preRemoveCbs) {
            it(std::nullopt);
        }
        {
            unique_lock l(m_mtx);
            m_children.pop_back();
        }
        for (auto &it : postRemoveCbs) {
            it(std::nullopt);
        }
    }
}

void Node::remove(Node& node) {
    remove(&node);
}

void Node::remove(Node* node) {
    if (m_undoBufRoot) {
        m_undoBufRoot->saveState();
    }

    if (!m_children.empty()) {
        const auto res = ranges::find_if(m_children,
                                        [&](auto& it) { return it.get() == node; });
        if (res != m_children.end()) {
            const auto preRemoveCbs = collectCallbacks(cbType::preRemoveChild, true);
            const auto postRemoveCbs = collectCallbacks(cbType::postRemoveChild, true);
            for (auto &it : preRemoveCbs) {
                it(std::nullopt);
            }
            {
                unique_lock l(m_mtx);
                m_children.erase(res);
            }
            for (auto &it : postRemoveCbs) {
                it(std::nullopt);
            }
        }
    }
}

void Node::clearChildren() {
    if (m_undoBufRoot) {
        m_undoBufRoot->saveState();
    }

    const auto preRemoveCbs = collectCallbacks(cbType::preRemoveChild, true);
    const auto postRemoveCbs = collectCallbacks(cbType::postRemoveChild, true);
    for (auto &it : preRemoveCbs) {
        it(std::nullopt);
    }
    {
        unique_lock l(m_mtx);
        children().clear();
    }
    for (auto &it : postRemoveCbs) {
        it(std::nullopt);
    }
}

deque<Node*> Node::findChild(const string& name) {
    deque<Node*> list;
    if (m_name == name) {
        list.emplace_back(this);
    }
    iterateChildren(*this, [&](Node& nd) {
        if (nd.name() == name) {
            list.emplace_back(&nd);
        }
        return true;
    });
    return list;
}

void Node::removeChangeCb(const cbType cbType, void *ptr) {
    if (const auto c = m_changeCb[cbType].find(ptr); c != m_changeCb[cbType].end()) {
        m_changeCb[cbType].erase(c);
    }
}

void Node::signalChange(const cbType cbType, const std::optional<Node*> node) {
    for (auto &val: m_changeCb[cbType] | views::values) {
        val(node);
    }
}

json Node::asJson(const bool skipClassEntries) {
    json root;
    unique_lock l(m_mtx);
    serialize(root, skipClassEntries);
    return root;
}

json Node::serializeClassValues() {
    json j;
    serializeClassValues(j);
    return j;
}

json& Node::serializeNonClassValue(json& j, const pushToType pushTo) {
    // check if there is a parent and if it is of nodeType array
    if (m_nodeValueType == nodeValueType::integer) {
        serializeByType<int32_t>(pushTo, j);
    } else if (m_nodeValueType == nodeValueType::floating) {
        serializeByType<float>(pushTo, j);
    } else if (m_nodeValueType == nodeValueType::string) {
        serializeByType<std::string>(pushTo, j);
    } else if (m_nodeValueType == nodeValueType::boolean) {
        serializeByType<bool>(pushTo, j);
    } else if (m_nodeValueType == nodeValueType::array || m_nodeValueType == nodeValueType::object) {
        auto dest = pushTo == pushToType::array ? &j[std::stoi(m_key)] : &j[m_key];
        *dest = m_nodeValueType == nodeValueType::array ? json::array() : json::object();
        return *dest;
    }
    return j;
}

// Serialize the node tree to JSON
void Node::serialize(json& j, const bool skipClassEntries)  {
    if (!skipClassEntries) {
        serializePerClass(j, skipClassEntries);
    }
    serializeNonClass(j);
}

void Node::serializePerClass(json& j, const bool skipClassEntries) {
    serializeClassValues(j);
    if (!m_children.empty()) {
        j["children"] = json::array();
        for (const auto& child : m_children) {
            j["children"].emplace_back(json{});
            child->serializePerClass(j["children"].back(), skipClassEntries);
        }
    }
}

void Node::serializeNonClass(json& j, const pushToType pushTo) {
    json& sj = serializeNonClassValue(j, pushTo);

    for (const auto& child : m_children) {
        if (m_nodeValueType == nodeValueType::array) {
            child->serializeNonClass(sj, pushToType::array);
        } else if (m_nodeValueType == nodeValueType::object || m_nodeValueType == nodeValueType::root) {
            child->serializeNonClass(sj, pushToType::object);
        }
    }
}

void Node::deserialize(const string& str, const bool skipClassEntries) {
    const auto j = json::parse(str);
    setNodeValueType(nodeValueType::root);
    deserialize(j, skipClassEntries);
}

void Node::deserialize(const json& j, const bool skipClassEntries, std::optional<std::list<std::function<void()>*>*> postLoadCbs) {
    if (!skipClassEntries && serializeClassValues() != getValues(j)) {
        deserializeClassValues(j);
        for (const auto &func: m_changeCb[cbType::postChange] | views::values) {
            func(std::nullopt);
        }
    }

    const auto postLoadCbsArg = postLoadCbs.value_or(&m_postCbList);
    if (m_postLoadCb.has_value()) {
        postLoadCbsArg->emplace_back(&m_postLoadCb.value());
    }

    if (!skipClassEntries) {
        unordered_map<string, Node*> existingChildren;
        for (const auto& child : m_children) {
            existingChildren[child->uuid()] = child.get();
        }

        if (j.contains("children") && j["children"].is_array()) {
            parseClassChildren(j["children"], skipClassEntries, existingChildren, postLoadCbsArg);
        }

        // Remove remaining existing children that were not found in the JSON input
        for (const auto &val: existingChildren | views::values) {
            remove(val);
        }

        if (!postLoadCbs.has_value()) {
            for (const auto it : m_postCbList) {
                (*it)();
            }
        }
    }

    parseNonClassEntries(j, skipClassEntries, postLoadCbsArg);
}

void Node::parseClassChildren(const json& j, const bool skipClassEntries, unordered_map<string, Node*>& existingChildren, std::list<std::function<void()>*>* postLoadCbsArg) {
    for (auto jChild = j.begin(); jChild != j.end(); ++jChild) {
        const bool skipChildCheck = !jChild->contains("uuid") || existingChildren.empty();
        if (const auto it = skipChildCheck ? existingChildren.end() : existingChildren.find(jChild->at("uuid"));
            it != existingChildren.end()) {
            it->second->deserialize(*jChild, skipClassEntries, postLoadCbsArg);
            // assure correct order
            auto childIt = ranges::find_if(m_children, [&](auto& el){ return el.get() == it->second; });
            const auto childIdx = distance(m_children.begin(), childIt);
            if (const auto jChildIdx = distance(j.begin(), jChild); childIdx != jChildIdx) {
                m_children.splice(next(m_children.begin(), jChildIdx), m_children, childIt);
            }
            existingChildren.erase(it);
        } else {
            // Create a new child and add it to the node
            if (auto fact_child = m_factory.create(jChild->contains("typeName") ? jChild->at("typeName") : "Node")) {
                auto& newChild = push(std::move(fact_child));
                newChild.deserialize(*jChild, skipClassEntries, postLoadCbsArg);
            }
        }
    }
}

void Node::parseNonClassEntries(const nlohmann::json& j, const bool skipClassEntries, std::list<std::function<void()>*>* postLoadCbsArg) {
    for (auto& [key, value] : j.items()) {
        if (!skipClassEntries
            && (key == "children"
                || std::ranges::find(m_classKeys[m_typeName].first, key) != m_classKeys[m_typeName].first.end())) {
            continue;
        }

        if (value.is_array() || value.is_object()) {
            parseArrayOrObjectChild(value, key, skipClassEntries, postLoadCbsArg);
        } else {
            parseSingleValueChild(value, key);
        }
    }
}

void Node::parseArrayOrObjectChild(const json& value, const std::string& key, const bool skipClassEntries, list<function<void()>*>* postLoadCbsArg) {
    auto& newChild = createNewElement();
    newChild.setKey(key);
    newChild.setNodeValueType(value.is_array() ? nodeValueType::array : nodeValueType::object);
    newChild.deserialize(value, skipClassEntries, postLoadCbsArg);
    // TODO: missing check if node to add already exists
}

void Node::parseSingleValueChild(const json& value, const std::string& key) {
    auto childIt = std::ranges::find_if(m_children, [&](auto& el){ return el.get()->key() == key; });
    if (childIt == m_children.end()) {
        auto& je = createNewElement();
        childIt = --m_children.end();
        childIt->get()->setKey(key);
    }

    if (value.is_boolean()) {
        childIt->get()->setValue(value.get<bool>());
    } else if (value.is_number_float()) {
        childIt->get()->setValue(value.get<float>());
    } else if (value.is_number()) {
        childIt->get()->setValue(value.get<int32_t>());
    } else if (value.is_string()) {
        childIt->get()->setValue(value.get<std::string>());
    }
}

void Node::load(const filesystem::path& filePath, const bool skipNonClass) {
    m_fileName = filePath;
    load(false, skipNonClass);
}

void Node::loadFromAssets(const filesystem::path& filePath, const bool skipNonClass) {
    m_fileName = filePath;
    load(true, skipNonClass);
}

void Node::load() {
    load(m_useAssetLoader);
}

void Node::load(bool fromAssets, const bool skipNonClass) {
    m_useAssetLoader = fromAssets;
    setNodeValueType(nodeValueType::root);

    if (m_undoBufRoot) {
        saveState();
    }

    if (!fromAssets && filesystem::exists(m_fileName)) {
        json j;
        ifstream i(m_fileName);
        i >> j;
        deserialize(j, skipNonClass); // skip the root node which is always empty
        m_fileNameForWatcher = m_fileName;
    } else {
        auto str = AssetLoader::loadAssetAsString(m_fileName);
        auto j = json::parse(str);
        deserialize(j, skipNonClass);
        m_fileNameForWatcher = AssetLoader::getAssetPath() / m_fileName;
    }

    if (m_watchFile && m_watchFile->time == filesystem::file_time_type{}) {
        m_watchFile->time = filesystem::last_write_time(m_watchFile->path);
    }

    // update file watching
    if (m_watch) {
        setWatch(true);
    }
}

void Node::loadFromString(const string& str, const bool skipClassEntries) {
    if (m_undoBufRoot) {
        saveState();
    }

    if (!str.empty()) {
        deserialize(json::parse(str), skipClassEntries);
    }
}

void Node::loadFromJson(const nlohmann::json& json, const bool skipClassEntries) {
    if (m_undoBufRoot) {
        saveState();
    }
    deserialize(json, skipClassEntries);
}

void Node::saveAs(const filesystem::path& filePath) {
    m_fileName = filePath;
    save();
}

void Node::save(const bool skipNonClass) {
    if (m_fileName.empty()) {
        return;
    }

    ofstream o(m_fileName);
    o << setw(4) << asJson(skipNonClass) << endl;

    if (m_watchFile) {
        m_watchFile->time = filesystem::last_write_time(m_watchFile->path);
    }
}

void Node::saveState() {
    if (m_undoing) {
        return;
    }

    // if there were new changes after one or several undos
    // remove all entries which lie in the future after the actual entry
    if (!m_undoBuf.empty() && m_undoBufIt != m_undoBuf.end() - 1) {
        m_undoBuf.erase(m_undoBufIt, m_undoBuf.end());
    }

    // if the undo queue is filled, delete the first element
    if (m_undoBuf.size() + 1 >= m_maxUndoBufSize) {
        m_undoBuf.erase(m_undoBuf.begin());
    }

    // serialize the actual state to a binary json and push it to the undo queue
    m_undoBuf.emplace_back(json::to_bson(asJson()));
    m_undoBufIt = m_undoBuf.end() - 1; // set the undoBufPtr to the new entry
}

void Node::undo() {
    if (m_undoBuf.empty()) {
        return;
    }

    // we need to be able to reproduce the last step, so also create an undo
    // copy in case we are at the end of the undoBuf queue
    if (--m_undoBuf.end() == m_undoBufIt) {
        saveState();    // will set the undoBufIt tp the end of the queue
    }

    --m_undoBufIt;  // set back one entry

    m_undoing = true;
    deserialize(json::from_bson(*m_undoBufIt));
    m_undoing = false;
}

void Node::redo() {
    if (m_undoBuf.empty()) {
        return;
    }

    if (m_undoBufIt != --m_undoBuf.end()) {
        ++m_undoBufIt;
        m_undoing = true;
        deserialize(json::from_bson(*m_undoBufIt));
        m_undoing = false;
    }
}

bool Node::iterateChildren(Node& node, const function<void(Node&)>& f) {
    f(node);
    const auto r = ranges::all_of(node.children(), [f](auto& it) {
        if (!iterateChildren(*it, f)) {
            return false;
        }
        return true;
    });
    return r;
}

bool Node::iterateChildren(const function<void(Node&)>& f) const {
    for (const auto& it: children()) {
        if (!iterateChildren(*it, f)) {
            return false;
        }
    }
    return true;
}

deque<function<void(std::optional<Node*>)>> Node::collectCallbacks(cbType cbType, bool withChildrenOnly) {
    deque<function<void(std::optional<Node*>)>> list;
    if  (!withChildrenOnly || !m_children.empty()) {
        for (auto &val: m_changeCb[cbType] | views::values) {
            list.emplace_back(val);
        }
    }

    iterateChildren(*this, [&withChildrenOnly, &list, &cbType](Node& node) {
        if  (!withChildrenOnly || !node.children().empty()) {
            for (auto &val: node.changeCb()[cbType] | views::values) {
                list.emplace_back(val);
            }
        }
        return true;
    });

    return list;
}

Node* Node::root() {
    unique_lock l(m_mtx);
    auto currentParent = m_parent;
    if (!currentParent) {
        return this;
    }
    while (currentParent->parent()) {
        currentParent = currentParent->parent();
    }
    return currentParent;
}

void Node::changeVal(const function<void()>& f) {
    if (m_undoBufRoot) {
        m_undoBufRoot->saveState();
    }

    for (auto &val: m_changeCb[cbType::preChange] | views::values) {
        val(std::nullopt);
    }

    { unique_lock l(m_mtx); f(); }

    for (auto &val: m_changeCb[cbType::postChange] | views::values) {
        val(std::nullopt);
    }
}

void Node::setUndoBuffer(bool enabled, const size_t size) {
    m_maxUndoBufSize = size;
    iterateChildren(*this, [this](Node& node){
        node.setUndoBufferRoot(this);
        return true;
    });
}

void Node::checkAndAddWatchPath(const string& fn) {
    const auto r = ranges::find_if(m_watchFiles, [&](auto& it) {
        return it.path.string() == fn;
    });

    if (r == m_watchFiles.end()) {
        m_watchFile = &m_watchFiles.emplace_back(NodeWatchFile{this, fn});
        if (filesystem::exists(m_watchFile->path)) {
            m_watchFile->time = filesystem::last_write_time(m_watchFile->path);
        }
    }
}

void Node::checkWatchThreadRunning() {
    if (!m_watchThreadRunning && m_useWatchThread) {
        m_watchThreadRunning = true;
        startWatchThread();
    }
}

void Node::setWatch(bool val) {
#ifndef ARA_USE_CMRC
    m_watch = val;
    if (!m_fileNameForWatcher.empty()) {
        checkAndAddWatchPath(m_fileNameForWatcher.string());
        iterateChildren(*this, [&val, this](Node& nd){
            if (&nd != this) {
                nd.setWatch(val);
            }
            return true;
        });

        if (val) {
            checkWatchThreadRunning();
        }
    }
#endif
}

void Node::startWatchThread() {
#ifndef ARA_USE_CMRC
    m_watchThrd = thread([]{
        while (m_watchThreadRunning) {
            watchThreadIterate();
            this_thread::sleep_for(0.7s);
        }
    });
    m_watchThrd.detach();
#endif
}

void Node::watchThreadIterate() {
    unique_lock lock(m_watchMtx);
    try {
        for (auto &[node, path, time, fileSize] : m_watchFiles) {
            if (exists(path)) {
                if (auto ft = filesystem::last_write_time(path); ft != m_initFt && ft != time) {
                    // the file may actually being written to, file size needs to be constant
                    if (fileSize != file_size(path)) {
                        fileSize = file_size(path);
                    } else {
                        LOG << "Detected File change: " << path;
                        node->load();
                        time = ft;
                    }
                }
            }
        }
    } catch (...) {
        LOGE << "Node file watcher caught an error checking files";
    }
}

void Node::stopWatchThread() {
    m_watchThreadRunning = false;
}

}
