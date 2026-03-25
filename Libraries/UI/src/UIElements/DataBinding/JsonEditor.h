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
    void updateStyleIt(ResNode* node, state st, const std::string& styleClass) override;

    void addExpandButt();
    void loadFile(const std::filesystem::path& p);
    void parseNonClassEntries(const nlohmann::json& j, std::list<std::function<void()>*>* postLoadCbsArg) override;
    void initChild(JsonEditor& je, const std::string& key);
    void setLineOffset(JsonEditor* nd, int32_t& yOffsCntr, int32_t& lineHeight);
    void rebuildCollapseState(JsonEditor* nd, int32_t& yOffsCntr, int32_t& lineHeight);
    int32_t getLineHeight(ResNode* node) const;

    bool isExpanded() const { return m_expanded; }
    void setExpanded(const bool val) { m_expanded = val; }
    void setJsonObjectType(const JsonEntryType& tp) { m_jsonEntryType = tp; }
    void setSpacing(const int32_t& s);
    void setLabelWidth(const int32_t& w);
    void setLineHeight(const int32_t& h);
    void setYOffs(const JsonEditor* parent, int32_t& yOffsCntr, int32_t& lineHeight);

    auto getJsonEntryType() const { return m_jsonEntryType; }
    auto getYOffs() const { return m_yOffs; }
    auto getCustPos() const { return m_custPos; }
    auto getDepth() const { return m_treeDepth; }
    auto getLabel() const { return m_label; }
    auto getEdit() const { return m_edit; }

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

protected:
    JsonEntryType m_jsonEntryType = JsonEntryType::nullObject;
    Label* m_label = nullptr;
    UIEdit* m_edit = nullptr;
    int32_t m_labelWidth = 180;
    int32_t m_lineHeight = 22;
    int32_t m_xSpacing = 3;
    int32_t m_ySpacing = 3;
    int32_t m_xIdent = 50;
    int32_t m_yOffs = 0;
    int32_t m_treeDepth = 0;
    glm::ivec2 m_custPos {};
    bool m_expanded = false;
    bool m_enableExpandButton = true;
    std::function<void(const std::string& str)> m_valChangedCb;

    static inline int32_t m_yOffsCntr = 0;
};

}
