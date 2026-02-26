//
// Created by sven on 25-02-26.
//

#include "JsonEditor.h"

#include "UISharedRes.h"

using namespace std;
using namespace nlohmann;

namespace ara {

void JsonEditor::updateMatrix() {
    if (m_reqRebuild) {
        rebuild();
        m_reqRebuild = false;
    }
    UINode::updateMatrix();
}

void JsonEditor::traverse(json& j, const std::function<void(const json&)>& callback) {
    for (auto it = j.begin(); it != j.end(); ++it) {
        std::cout << it.key() << " : " << it.value() << "\n";
    }
    /*for (auto& [key, value] : j.items()) {
        LOG << "traverse key: " << key;
    }*/
    /*
    if (j.is_object() || j.is_array()) {
        LOG << "object, size: " << j.size();
        for (auto& [key, value] : j.items()) {
            std::cout << "Key: " << key << ", Value: " << value.dump() << "\n";
            //callback(value);  // Process each array element
            traverse(value, callback);  // Recurse
        }
    } else {
        //callback(j);  // Leaf node (string, number, bool, null)
    }*/
}

void JsonEditor::rebuild() {
    clearChildren();
    int32_t yOffs = 0;
    /*traverse(m_json, [](const json& j) {
        if (j.is_string()) {
            LOG << "String: " << j.get<std::string>();
        } else if (j.is_number()) {
            LOG << "Number: " << j.get<int>();
        }
        // Add more cases as needed
    });*/
}

void JsonEditor::loadFile(const filesystem::path& p) {
    m_json.load(p);
    m_geoChanged = true;
    m_reqRebuild = true;

    /*
    if (filesystem::exists(p)) {
        ifstream f(p);
        m_json = ordered_json::parse(f);
        m_geoChanged = true;
        m_reqRebuild = true;
        getSharedRes()->reqRedraw();
    }*/
}


}