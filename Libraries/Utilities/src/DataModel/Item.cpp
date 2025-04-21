//
// Created by sven on 11/30/20.
//
#include "Item.h"
#include "ItemFactory.h"

using namespace std;
using json = nlohmann::json;

namespace ara {

// take a snapshot of the objects state
void Item::saveState() {
    // if there were new changes after one or several undos
    // remove all entries which lie in the future after the actual entry
    if (!m_undoBuf.empty() && m_undoBufIt != m_undoBuf.end() - 1) {
        m_undoBuf.erase(m_undoBufIt, m_undoBuf.end());
    }

    // if the undo queue is filled, delete the first element
    if (m_undoBuf.size() + 1 >= m_maxUndoBufSize) {
        m_undoBuf.erase(m_undoBuf.begin());
    }

    // serialize the actual state to a stream and push it to the undo queue
    m_undoBuf.emplace_back();

    serializeToJson(m_undoBuf.back());

    // when pressing undo, always go back to the last entry in the queue
    m_undoBufIt = m_undoBuf.end() - 1;
}

/** parse xml snapshot from memory and update the project */
void Item::undo() {
    if (m_undoBuf.empty()) {
        return;
    }

    // we need to be able to reproduce the last step, so also create and undo
    // copy in case we are at the end of the undoBuf queue
    if (--m_undoBuf.end() == m_undoBufIt) {
        saveState();
        --m_undoBufIt;
    }

    auto root = (*m_undoBufIt)[itemType().c_str()];
    parseFromJson(root);

    if (m_undoBufIt != m_undoBuf.begin()) {
        --m_undoBufIt;
    }
}

void Item::redo() {
    if (m_undoBufIt != --m_undoBuf.end()) {
        ++m_undoBufIt;
        auto root = (*m_undoBufIt)[itemType().c_str()];
        parseFromJson(root);
    }
}

void Item::serializeProperties(json &node) {
}

void Item::parseProperties(json& node) {
}

void Item::serializeToJson(json &root) {
    root.emplace(itemType().c_str(), json());
    auto& node = root[itemType().c_str()];
    serializeProperties(node);
    for (auto &it : m_children) {
        it->serializeToJson(node);
    }
}

void Item::parseFromJson(json &root) {
    parseProperties(root);

    auto it = m_children.begin();

    // iterate through xml doc
    for (auto &jsonChild : root.items()) {
        if (jsonChild.value().is_object()) {
            // if the type names of the actual xml-node and item-node are equal (and
            // we are not at the end) parse this node and count up the item tree
            // iterator
            if (it != m_children.end() && jsonChild.key() == it->get()->itemType()) {
                (it++)->get()->parseFromJson(jsonChild.value());
            } else if (it == m_children.end()) {
                // if we are at the end, create a new entry in the Item tree and
                // parse the node
                if (m_itemFactory) {
                    auto fact_child = m_itemFactory->Create(jsonChild.key());
                    if (fact_child) {
                        m_children.emplace_back(std::move(fact_child));
                    }
                } else {
                    add<Item>();
                }

                auto& child = m_children.back();
                child->parent        = this;
                child->m_itemFactory = m_itemFactory;
                child->onItemChanged(this, [this] { signalItemChanged(); });

                it = next(m_children.end(), -1);
                (it++)->get()->parseFromJson(jsonChild.value());
            }
        }
    }
}

void Item::load(const std::filesystem::path& filePath) {
    saveState();

    // if the undo queue is filled, delete the first element
    if (m_undoBuf.size() + 1 >= m_maxUndoBufSize) {
        m_undoBuf.erase(m_undoBuf.begin());
    }

    // serialize the actual state to a stream and push it to the undo queue
    m_undoBuf.emplace_back();
    m_fileName = filePath;

    std::ifstream i(filePath);
    i >> m_undoBuf.back();
    parseFromJson(m_undoBuf.back().front());

    m_undoBufIt = --m_undoBuf.end();
}

void Item::saveAs(const std::filesystem::path& filePath) {
    m_fileName = filePath;
    save();
}

void Item::save() {
    saveState();
    std::ofstream o(m_fileName);
    o << std::setw(4) << *m_undoBufIt << std::endl;
}

ItemUi *ItemUi::getChildById(int idx, Item *item) {
    Item *itItem = item ? item : this;

    // check the children, if nothing search their children
    for (auto &it : reinterpret_cast<std::list<std::unique_ptr<ItemUi>> &>(itItem->children()))
        if (it->id() == idx) {
            return it.get();
        }

    for (auto &it : reinterpret_cast<std::list<std::unique_ptr<ItemUi>> &>(itItem->children())) {
        return it->getChildById(idx, it.get());
    }

    return nullptr;
}

void ItemUi::serializeProperties(json &node) {
    Item::serializeProperties(node);
    node["name"] = name().c_str();
    node["displayName"] = displayName().c_str();
    node["id"] = id();
    node["visible"] = visible();
}

void ItemUi::parseProperties(json& node) {
    Item::parseProperties(node);

    if (!node["name"].is_null()) {
        name = static_cast<std::string>(node["name"]);
    }
    if (!node["displayName"].is_null()) {
        displayName = static_cast<std::string>(node["displayName"]);
    }
    if (!node["id"].is_null()) {
        id = static_cast<int>(node["id"]);
    }
    if (!node["visible"].is_null()) {
        visible = static_cast<bool>(node["visible"]);
    }
}


}  // namespace ara