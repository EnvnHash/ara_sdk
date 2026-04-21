//
// Created by sven on 01-04-26.
//

#pragma once

#include "UIElements/Div.h"
#include "UIElements/Text/Label.h"
#include "UIElements/Text/UIEdit.h"

namespace ara {

class Button;

class NodeMemberVariableEdit : public Div {
public:
    struct EditPar {
        std::string style{};
        memberVar* memVar{};
        const std::string& memVarName{};
        Node* node{};
        std::string labelText;
        glm::ivec2 spacing{};
        int32_t lineHeight{};
        int32_t labelWidth{};
        arrange editAlign{};
        int32_t yOffs{};
        std::optional<VariableEditOption<>*> options{};
    };

    NodeMemberVariableEdit();
    explicit NodeMemberVariableEdit(const EditPar& par);

    void init() override;
    void createCheckBox(bool val);
    void createPathEdit(const std::filesystem::path &val, int32_t stdWidth);

    template<typename T>
    UIEdit* createSingleEdit(T val, const int32_t& width, const int32_t& idx=-1) {
        const glm::ivec2 offset = {
            m_editAlign == arrange::horizontal ? (idx >= 0 ? idx * width : 0) : 0,
            m_editAlign == arrange::vertical ? (idx >= 0 ? idx * (m_lineHeight + m_spacing.y) : 0) : 0
        };

        auto& edit = push<UIEdit>(UINodePars{
            .pos = glm::ivec2{ m_labelWidth + m_spacing.x + offset.x, offset.y },
            .size = { glm::ivec2{ width, m_lineHeight } },
            .bgColor = m_stdBgColor,
            .style = getStyleClass()+".edit",
            .borderWidth = m_stdBorderWidth,
            .borderRadius = m_stdBorderRadius,
            .borderColor = m_stdBorderColor,
            .padding = glm::vec4{2.f, 2.f, 2.f, 2.f},
        });
        edit.setFontSize(22);
        edit.setUseWheel(true);

        setUIEditValues(edit, val);
        setTwoWayBinding<T>(edit, idx);
        return &edit;
    }

    template<typename T>
    void setUIEditValues(UIEdit& edit, T val) {
        if (m_options && m_options->min) {
            edit.setMin(m_options->min);
        }
        if (m_options && m_options->max) {
            edit.setMax(m_options->max);
        }
        if (m_options && m_options->step) {
            edit.setStep(m_options->step);
        }
        if (m_options && m_options->step) {
            edit.setPrecision(m_options->precision);
        }
        setSingleEditValue(edit, val);
    }

    template <typename T>
    void setSingleEditValue(UIEdit& edit, T val) {
        if constexpr (std::is_same_v<T, std::string>) {
            edit.setText(std::any_cast<std::string>(val));
        } else {
            edit.setValue(std::any_cast<T>(val));
        }
    }

    template <typename T>
    void setTwoWayBinding(UIEdit& edit, const int32_t& idx) {
        edit.addEnterCb([&, idx](const std::string &str) {
            if (m_memVar && !m_blockMemVarSet) {
                std::any anyVal;
                if constexpr (std::is_same_v<T, std::string>) {
                    anyVal = str;
                } else {
                    anyVal = std::any(edit.getValue<T>());
                    if (m_options && m_options->syncEdits && idx > -1) {
                        for (int i=0; i<m_arrayEdit.size(); ++i) {
                            if (i != idx) {
                                m_arrayEdit[i]->setValue(edit.getValue<T>());
                                m_memVar->set(anyVal, i);
                            }
                        }
                    }
                }
                m_memVar->set(anyVal, idx);
            }
            m_blockMemVarSet = false;
        }, this);

        // two-way binding
        m_node->setOnChangeCb(cbType::postChange, this, [this, &edit](Node*, const std::string& varName) {
            if (const auto actVal = std::any_cast<T>(m_memVar->get());
                varName == m_memVarName && edit.value<T>() != actVal) {
                m_blockMemVarSet = true;
                setSingleEditValue(edit, actVal);
            }
        });
    }

    template<typename T>
    void createVector() {
        createArrayEdit<T>(std::any_cast<T>(m_memVar->get()).size());
    }

    template<typename T>
    void createArrayEdit(const size_t sz) {
        auto vec = std::any_cast<T>(m_memVar->get());
        for (auto i=0; i<sz; ++i) {
            m_arrayEdit.emplace_back(createSingleEdit(vec[i], getEditWidth(sz), i));
        }
    }

    template<typename T>
    void setElementWidths(T val, state) {
        if (m_label) {
            m_label->setWidth(val);
        }

        if (!m_arrayEdit.empty()) {
            const auto width = getEditWidth(m_arrayEdit.size());
            for (const auto it : m_arrayEdit) {
                it->setWidth(width);
            }
        } else if (m_edit) {
            m_edit->setWidth(getEditWidth(1));
        }
    }

    int32_t getEditWidth(size_t nrEditFields);

    void setEditValFromMemberVar();
    void setLabelText(const std::string& text);

    void setSpacing(const glm::ivec2& s) { m_spacing = s; }
    void setLabelWidth(const int32_t& w) { m_labelWidth = w; }
    void setLineHeight(const int32_t& h) { m_lineHeight = h; }
    void setNumEditsPerRow(const int32_t& v) { m_numEditsPerRow = v; }
    void setEditAlign(const arrange& a) { m_editAlign = a; }

    int32_t getUnitHeight() const;
    auto getYOffs() const { return m_yOffs; }
    auto getLabel() const { return m_label; }
    auto getEdit() const { return m_edit; }
    auto getMemVar() const { return m_memVar; }
    auto getPathLabel()  { return m_pathLabel; }
    auto getCheckBox()  { return m_checkBoxButt; }
    auto getLabel()  { return m_label; }
    auto getEdit()  { return m_edit; }
    auto getBrowseButt()  { return m_browseButt; }

protected:
    Label*                  m_label = nullptr;
    Button*                 m_checkBoxButt = nullptr;
    Button*                 m_browseButt = nullptr;
    Label*                  m_pathLabel = nullptr;
    UIEdit*                 m_edit = nullptr;
    std::vector<UIEdit*>    m_arrayEdit;
    int32_t                 m_labelWidth = 180;
    int32_t                 m_lineHeight = 22;
    int32_t                 m_stdBorderWidth = 2;
    int32_t                 m_stdBorderRadius = 4;
    int32_t                 m_stdBrowseButtWidth = 60;
    glm::vec4               m_stdBorderColor = glm::vec4{.3f, .3f, .3f, 1.f};
    glm::vec4               m_stdBgColor = glm::vec4{.15f, .15f, .15f, 1.f};
    glm::vec4               m_stdButtBgColor = glm::vec4{.25f, .25f, .25f, 1.f};
    glm::ivec2              m_spacing = { 3, 3 };
    int32_t                 m_yOffs = 0;
    size_t                  m_numEditsPerRow = 4;
    glm::ivec2              m_custPos {};
    std::string             m_text;
    const memberVar*        m_memVar = nullptr;
    Node*                   m_node = nullptr;
    std::string             m_memVarName{};
    bool                    m_blockMemVarSet = false;
    arrange                 m_editAlign{};
    VariableEditOption<>*   m_options = nullptr;

    std::vector<std::pair<std::string, std::string>> m_stdSuffixes = {{ "All Files", "*" } };
};

}
