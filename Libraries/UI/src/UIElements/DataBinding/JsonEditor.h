//
// Created by sven on 25-02-26.
//

#pragma once

#include <UIElements/Div.h>

#include "UIElements/Text/UIEdit.h"

namespace ara {

class Label;
class UIEdit;

class JsonEditor : public Div {
public:
    JsonEditor();
    void init() override;
    virtual void onEnter(const std::string &str);
    void updateStyleIt(ResNode* node, state st, const std::string& styleClass) override;

    void addExpandButt();
    void loadFile(const std::filesystem::path& p);
    void initChild(JsonEditor& je);
    static void setLineOffset(JsonEditor* nd, int32_t& yOffsCntr, int32_t& lineHeight);
    static void rebuildCollapseState(JsonEditor* nd, int32_t& yOffsCntr, int32_t& lineHeight);
    int32_t getLineHeight(ResNode* node) const;

    bool isExpanded() const { return m_expanded; }
    void setExpanded(const bool val) { m_expanded = val; }
    void setSpacing(const int32_t& s);
    void setLabelWidth(const int32_t& w);
    void setLineHeight(const int32_t& h);
    void setYOffs(const JsonEditor* parent, int32_t& yOffsCntr, int32_t& lineHeight);

    auto getYOffs() const { return m_yOffs; }
    auto getCustPos() const { return m_custPos; }
    auto getDepth() const { return m_treeDepth; }
    auto getLabel() const { return m_label; }
    auto getEdit() const { return m_edit; }

    auto getJsonRoot() const {
        auto p = dynamic_cast<JsonEditor*>(parent());
        while (p && p->getNodeValueType() != nodeValueType::root) {
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
    Node& createNewElement() override;

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

    static inline int32_t m_yOffsCntr = 0;
};

}
