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
#include <string_utils.h>

#include "AssetLoader.h"

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
            auto canLock = m_watchMtx.try_lock();
            m_watchFiles.erase(r);
            if (canLock) {
                m_watchMtx.unlock();
            }
        }
    }
}

void Node::pop() {
    if (m_undoBufRoot) {
        m_undoBufRoot->saveState();
    }

    if (!m_children.empty()) {
        auto preRemoveCbs = collectCallbacks(cbType::preRemoveChild, true);
        auto postRemoveCbs = collectCallbacks(cbType::postRemoveChild, true);
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
        auto res = ranges::find_if(m_children,
                                        [&](auto& it) { return it.get() == node; });
        if (res != m_children.end()) {
            auto preRemoveCbs = collectCallbacks(cbType::preRemoveChild, true);
            auto postRemoveCbs = collectCallbacks(cbType::postRemoveChild, true);
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

    auto preRemoveCbs = collectCallbacks(cbType::preRemoveChild, true);
    auto postRemoveCbs = collectCallbacks(cbType::postRemoveChild, true);
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

json Node::asJson() {
    json root;
    {
        unique_lock l(m_mtx);
        serialize(root);
    }
    return root;
}

json Node::serializeClassValues() { // helper function for comparing during deserialization
    json j;
    serializeClassValues(j);
    return j;
}

// Serialize the node tree to JSON
void Node::serialize(json& j)  {
    serializeClassValues(j);

    if (!m_children.empty()) {
        j["children"] = json::array();
        for (const auto& child : m_children) {
            j["children"].emplace_back(json{});
            child->serialize(j["children"].back());
        }
    }
}

void Node::deserialize(const string& str) {
    json j = json::parse(str);
    deserialize(j);
}

void Node::deserialize(const json& j, std::optional<std::list<std::function<void()>*>*> postLoadCbs) {
    if (serializeClassValues() != getValues(j)) {
        deserializeClassValues(j);
        for (const auto &func: m_changeCb[cbType::postChange] | views::values) {
            func(std::nullopt);
        }
    }

    const auto postLoadCbsArg = postLoadCbs.value_or(&m_postCbList);
    if (m_postLoadCb.has_value()) {
        postLoadCbsArg->emplace_back(&m_postLoadCb.value());
    }

    if (!m_parseAsGenericJson) {
        unordered_map<string, Node*> existingChildren;
        for (const auto& child : m_children) {
            existingChildren[child->uuid()] = child.get();
        }

        if (j.contains("children") && j["children"].is_array()) {
            parseClassChildren(j["children"], existingChildren, postLoadCbsArg);
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

    parseNonClassEntries(j, postLoadCbsArg);
}

void Node::parseClassChildren(const json& j, unordered_map<string, Node*>& existingChildren, std::list<std::function<void()>*>* postLoadCbsArg) {
    for (auto jChild = j.begin(); jChild != j.end(); ++jChild) {
        const bool skipChildCheck = !jChild->contains("uuid") || existingChildren.empty();
        if (const auto it = skipChildCheck ? existingChildren.end() : existingChildren.find(jChild->at("uuid"));
            it != existingChildren.end()) {
            it->second->deserialize(*jChild, postLoadCbsArg);
            // assure correct order
            auto childIt = ranges::find_if(m_children, [&](auto& el){ return el.get() == it->second; });
            const auto childIdx = distance(m_children.begin(), childIt);
            const auto jChildIdx = distance(j.begin(), jChild);
            if (childIdx != jChildIdx) {
                m_children.splice(next(m_children.begin(), jChildIdx), m_children, childIt);
            }
            existingChildren.erase(it);
        } else {
            // Create a new child and add it to the node
            if (auto fact_child = m_factory.create(jChild->contains("typeName") ? jChild->at("typeName") : "Node")) {
                auto& newChild = push(std::move(fact_child));
                newChild.deserialize(*jChild, postLoadCbsArg);
            }
        }
    }
}

void Node::parseNonClassEntries(const nlohmann::json& j, std::list<std::function<void()>*>* postLoadCbsArg) {
    for (auto& [key, value] : j.items()) {
        if (!m_parseAsGenericJson
            && (key == "children"
                || std::ranges::find(m_classKeys[m_typeName].first, key) != m_classKeys[m_typeName].first.end())) {
            continue;
        }

        if (value.is_array() || value.is_object()) {
            auto& newChild = push<Node>();
            newChild.setKey(key);
            newChild.deserialize(value, postLoadCbsArg);
            // TODO: missing check if node to add already exists
        } else {
            auto childIt = std::ranges::find_if(m_children, [&](auto& el){ return el.get()->key() == key; });
            if (childIt == m_children.end()) {
                push<Node>();
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
    }
}

void Node::load(const filesystem::path& filePath) {
    m_fileName = filePath;
    load(false);
}

void Node::loadFromAssets(const filesystem::path& filePath) {
    m_fileName = filePath;
    load(true);
}

void Node::load() {
    load(m_useAssetLoader);
}

void Node::load(bool fromAssets) {
    m_useAssetLoader = fromAssets;

    if (m_undoBufRoot) {
        saveState();
    }

    if (!fromAssets && filesystem::exists(m_fileName)) {
        json j;
        ifstream i(m_fileName);
        i >> j;
        deserialize(j);
        m_fileNameForWatcher = m_fileName;
    } else {
        AssetLoader al;
        auto str = al.loadAssetAsString(m_fileName);
        json j = json::parse(str);
        deserialize(j);
        m_fileNameForWatcher = al.getAssetPath() / m_fileName;
    }

    if (m_watchFile && m_watchFile->time == filesystem::file_time_type{}) {
        m_watchFile->time = filesystem::last_write_time(m_watchFile->path);
    }

    // update file watching
    if (m_watch) {
        setWatch(true);
    }
}

void Node::loadFromString(const string& str) {
    if (m_undoBufRoot) {
        saveState();
    }

    if (!str.empty()) {
        json j = json::parse(str);
        deserialize(j);
    }
}

void Node::loadFromJson(const nlohmann::json& json) {
    if (m_undoBufRoot) {
        saveState();
    }
    deserialize(json);
}

void Node::saveAs(const filesystem::path& filePath) {
    m_fileName = filePath;
    save();
}

void Node::save() {
    ofstream o(m_fileName);
    o << setw(4) << asJson() << endl;

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
    for (const auto& it: node.children()) {
        if (!iterateChildren(*it, f)) {
            return false;
        }
    }
    return true;
}

bool Node::iterateChildren(const function<void(Node&)>& f) {
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

    {
        unique_lock l(m_mtx);
        f();
    }

    for (auto &val: m_changeCb[cbType::postChange] | views::values) {
        val(std::nullopt);
    }
}

void Node::setUndoBuffer(bool enabled, size_t size) {
    m_maxUndoBufSize = size;
    iterateChildren(*this, [this](Node& node){
        node.setUndoBufferRoot(this);
        return true;
    });
}

void Node::checkAndAddWatchPath(const string& fn) {
    auto r = ranges::find_if(m_watchFiles, [&](auto& it) {
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
        for (auto &wfIt : m_watchFiles) {
            if (exists(wfIt.path)) {
                auto ft = filesystem::last_write_time(wfIt.path);
                if (ft != m_initFt && ft != wfIt.time) {
                    // the file may actually being written to, file size needs to be constant
                    if (wfIt.fileSize != file_size(wfIt.path)) {
                        wfIt.fileSize = file_size(wfIt.path);
                    } else {
                        LOG << "Detected File change: " << wfIt.path;
                        wfIt.node->load();
                        LOG << " loading finished after File change: " << wfIt.path;
                        wfIt.time = ft;
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
