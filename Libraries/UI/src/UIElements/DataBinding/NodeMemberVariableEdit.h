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

    template<typename T>
    UIEdit* createSingleEdit(T val, const int32_t& width, const int32_t& idx=-1) {
        auto& edit = push<UIEdit>(UINodePars{
            .pos = glm::ivec2{ m_labelWidth + m_xSpacing + (idx >= 0 ? idx *width : 0), 0 },
            .size = { glm::ivec2{ width, m_lineHeight } },
            .bgColor = glm::vec4{.15f, .15f, .15f, 1.f},
            .style = getStyleClass()+".edit",
            .borderWidth = 2,
            .borderRadius = 4,
            .borderColor = glm::vec4{.3f, .3f, .3f, 1.f},
            .padding = glm::vec4{2.f, 2.f, 2.f, 2.f},
        });
        edit.setFontSize(22);
        edit.setUseWheel(true);

        if constexpr (std::is_same_v<T, std::string>) {
            edit.setText(std::any_cast<std::string>(val));
        } else {
            edit.setValue(std::any_cast<T>(val));
        }

        edit.addEnterCb([&, idx](const std::string &str) {
            if (m_memVar) {
                std::any anyVal;
                if constexpr (std::is_same_v<T, std::string>) {
                    anyVal = str;
                } else {
                    anyVal = std::any(edit.getValue<T>());
                }
                m_memVar->set(anyVal, idx);
            }
        }, this);

        return &edit;
    }

    template<typename T>
    void createArrayEdit() {
        auto vec = std::any_cast<std::vector<T>>(m_memVar->get());
        size_t idx = 0;
        auto width = static_cast<int32_t>((getContentSize().x -m_labelWidth -m_xSpacing) / static_cast<float>(std::min(vec.size(), m_numEditsPerRow)));
        for (auto &it : vec) {
            createSingleEdit(vec[idx], width, idx);
            ++idx;
        }
    }

    template<typename T>
    void createGlmEdit(const size_t sz) {
        auto vec = std::any_cast<T>(m_memVar->get());
        auto width = static_cast<int32_t>((getContentSize().x -m_labelWidth -m_xSpacing) / static_cast<float>(std::min(sz, m_numEditsPerRow)));
        for (auto i=0; i<sz; ++i) {
            createSingleEdit(vec[i], width, i);
        }
    }

    //void updateStyleIt(ResNode* node, const state st, const std::string& styleClass) override;
    //void setLineOffset(NodeMemberVariableEdit* nd, int32_t& yOffsCntr, int32_t& lineHeight);
    //void setYOffs(const NodeMemberVariableEdit* parent, int32_t& yOffsCntr, const int32_t& lineHeight);
    //int32_t getLineHeight(ResNode* node) const;

    void setEditValFromMemberVar();
    void setLabelText(const std::string& text);

    void setSpacing(const int32_t& s) { m_xSpacing = s; }
    void setLabelWidth(const int32_t& w) { m_labelWidth = w; }
    void setLineHeight(const int32_t& h) { m_lineHeight = h; }
    void setMemberVar(const memberVar& v) { m_memVar = &v; }
    void setNumEditsPerRow(const int32_t& v) { m_numEditsPerRow = v; }

    auto getYOffs() const { return m_yOffs; }
    auto getLabel() const { return m_label; }
    auto getEdit() const { return m_edit; }

protected:
    Label*                  m_label = nullptr;
    UIEdit*                 m_edit = nullptr;
    std::vector<UIEdit*>    m_arrayEdit;
    int32_t                 m_labelWidth = 180;
    int32_t                 m_lineHeight = 22;
    int32_t                 m_xSpacing = 3;
    int32_t                 m_ySpacing = 3;
    int32_t                 m_yOffs = 0;
    size_t                  m_numEditsPerRow = 4;
    glm::ivec2              m_custPos {};
    std::string             m_text;
    const memberVar*        m_memVar = nullptr;
};

}
