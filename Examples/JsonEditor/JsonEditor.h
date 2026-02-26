//
// Created by sven on 25-02-26.
//

#pragma once

#include <UIElements/Div.h>

namespace ara {

class JsonEditor : public Div {
public:
    void updateMatrix() override;
    void traverse(nlohmann::json& j, const std::function<void(const nlohmann::json&)>& callback);
    void rebuild();
    void loadFile(const std::filesystem::path& p);

    void parseString(const std::string& s) {
        //m_json = nlohmann::json::parse(s);
    }

private:
    Node                    m_json;
    //nlohmann::ordered_json  m_json;
    bool                    m_reqRebuild = false;
};

}
