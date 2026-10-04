//
// Created by sven on 04-10-26.
//

#pragma once

#include <UIElements/Div.h>
#include <UIElements/Text/UIEdit.h>

namespace ara {

class Label;

struct NumberEditPars {
    std::string             labelText;
    glm::ivec2              spacing = { 3, 3 };
    int32_t                 fontSize = 22;
    int32_t                 labelWidth = 150;
    int32_t                 buttWidth = 23;
    int32_t                 lineHeight = 22;
    int32_t                 borderWidth = 2;
    int32_t                 borderRadius = 4;
    glm::vec4               borderColor{.3f, .3f, .3f, 1.f};
};

class NumberEdit : public Div {
public:
    NumberEdit();
    NumberEdit(const UINodePars&, const NumberEditPars& initData);

    void init() override;

    template <CoordinateType T>
    void bind(T& var) {
        m_bindFunc = [this, &var]() {
            m_edit->setValue<T>(var);
            m_edit->addEnterCb([this, &var](const std::string&) {
                var = m_edit->getValue<T>();
                if (m_cbFunc) {
                    m_cbFunc();
                }
            }, this);
        };

        if (m_edit) {
            m_bindFunc();
        }
    }

    void onChange(const std::function<void()>& func) {
        m_cbFunc = func;
    }

private:
    Label*                  m_label=nullptr;
    NumberEditPars          m_pars{};
    UIEdit*                 m_edit=nullptr;
    std::function<void()>   m_bindFunc;
    std::function<void()>   m_cbFunc;
};

}
