//
// Created by sven on 01-04-26.
//

#pragma once

#include "UIElements/Div.h"
#include "UIElements/Text/Label.h"
#include "UIElements/Text/UIEdit.h"

namespace ara {

class NodeMemberVariableEdit : public Div {
public:
    NodeMemberVariableEdit();

    void init() override;

    //void updateStyleIt(ResNode* node, const state st, const std::string& styleClass) override;
    //void setLineOffset(NodeMemberVariableEdit* nd, int32_t& yOffsCntr, int32_t& lineHeight);
    //void setYOffs(const NodeMemberVariableEdit* parent, int32_t& yOffsCntr, const int32_t& lineHeight);
    //int32_t getLineHeight(ResNode* node) const;

    void setEditValFromMemberVar();
    void setLabelText(const std::string& text);

    void setSpacing(const int32_t& s) { m_xSpacing = s; }
    void setLabelWidth(const int32_t& w) { m_labelWidth = w; }
    void setLineHeight(const int32_t& h) { m_lineHeight = h; }
    void setMemberVar(const memberVar& v) { m_memVar = &v; setEditValFromMemberVar(); }

    auto getYOffs() const { return m_yOffs; }
    auto getLabel() const { return m_label; }
    auto getEdit() const { return m_edit; }

protected:
    Label*              m_label = nullptr;
    UIEdit*             m_edit = nullptr;
    int32_t             m_labelWidth = 180;
    int32_t             m_lineHeight = 22;
    int32_t             m_xSpacing = 3;
    int32_t             m_ySpacing = 3;
    int32_t             m_yOffs = 0;
    glm::ivec2          m_custPos {};
    std::string         m_text;
    const memberVar*    m_memVar = nullptr;
};

}
