//
// Created by sven on 25-02-26.
//

#pragma once

#include <UIElements/Div.h>

#include "UIElements/Text/UIEdit.h"

namespace ara {

class Label;
class UIEdit;

enum class JsonEntryType { nullObject = 0, boolean, string, float_number, int_number, array, object, rootObject };

class JsonEditor : public Div {
public:
    JsonEditor();
    void init() override;
    void addExpandButt();
    void loadFile(const std::filesystem::path& p);
    void parseNonClassEntries(const nlohmann::json& j, std::list<std::function<void()>*>* postLoadCbsArg) override;
    static void rebuildCollapseState(JsonEditor* nd, int32_t& yOffsCntr);

    bool isExpanded() const { return m_expanded; }

    void setExpanded(const bool val) { m_expanded = val; }
    void setJsonObjectType(const JsonEntryType& tp) { m_jsonEntryType = tp; }
    void setYOffs(const int32_t& val);

    auto getJsonEntryType() const { return m_jsonEntryType; }
    auto getYOffs() const { return m_yOffs; }

    auto getJsonRoot() const {
        auto p = dynamic_cast<JsonEditor*>(parent());
        while (p && p->getJsonEntryType() != JsonEntryType::rootObject) {
            p = dynamic_cast<JsonEditor*>(p->parent());
        }
        return p;
    }

    auto getTreeDepth() {
        size_t cntr = 0;
        const auto thisTp = getTypeName<JsonEditor>();
        Node* p = this;
        while (p && p->typeName() == thisTp) {
            ++cntr;
            p = p->parent();
        }
        return std::max<size_t>(0, cntr -2);
    }

private:
    JsonEntryType m_jsonEntryType = JsonEntryType::nullObject;
    Label* m_label = nullptr;
    UIEdit* m_edit = nullptr;
    int32_t m_lineHeight = 22;
    int32_t m_ySpacing = 3;
    int32_t m_xIdent = 50;
    int32_t m_yOffs = 0;
    bool m_expanded = false;

    static inline int32_t m_yOffsCntr = 0;
};

}
