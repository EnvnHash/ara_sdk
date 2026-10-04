//
// Created by sven on 04-10-26.
//

#include "NumberEdit.h"
#include "UIElements/Text/UIEdit.h"
#include "UIElements/Button/Button.h"

using namespace glm;
using namespace std;

namespace ara {

NumberEdit::NumberEdit() {
    setTypeName<NumberEdit>();
    setName(ara::getTypeName<NumberEdit>());
}

NumberEdit::NumberEdit(const UINodePars& initData, const NumberEditPars& numberEditPars) {
    m_fontType = "regular";
    setFocusAllowed(false);
    setTypeName<NumberEdit>();
    setName(getTypeName<NumberEdit>());

    parseInitPars(initData);
    m_pars = numberEditPars;
}

void NumberEdit::init() {
    m_fontType = "regular";

    m_label = &push<Label>(
        UINodePars{
            .pos = ivec2{ 0, 0 },
            .size = ivec2{ m_pars.labelWidth, m_pars.lineHeight },
            .fgColor = vec4{ 1.f, 1.f, 1.f, 1.f },
            .style = getStyleClass()+".label",
            .align = align::left,
            .valign = valign::top,
        },
        LabelPars{
            .text = m_pars.labelText + ":",
            .textAlignX = align::left,
            .textAlignY = valign::center,
            .fontType = "regular",
            .fontHeight = 22
        }
    );

    m_edit = &push<UIEdit>(UINodePars{
        .pos = ivec2{ m_pars.labelWidth + m_pars.spacing.x, 0 },
        .size = { ivec2{ -m_pars.labelWidth - m_pars.buttWidth * 2, m_pars.lineHeight } },
        //.bgColor = ,
        .style = getStyleClass()+".edit",
        .borderWidth = m_pars.borderWidth,
        .borderRadius = m_pars.borderRadius,
        .borderColor = m_pars.borderColor,
        .padding = vec4{2.f, 2.f, 2.f, 2.f},
    });

    m_edit->setFontSize(m_pars.fontSize);
    m_edit->setUseWheel(true);

    for (int i=0; i<2; i++) {
        auto& b = push<Button>(UINodePars{
            .pos = ivec2{-m_pars.buttWidth * i, 0 },
            .size = ivec2{m_pars.buttWidth, m_pars.lineHeight},
            .fgColor = vec4{1.f, 1.f, 1.f, 1.f},
            //.bgColor = m_stdBgColor,
            .align = align::right,
            .borderWidth = m_pars.borderWidth,
            .borderRadius = m_pars.borderRadius,
            .borderColor = m_pars.borderColor,
        });
        b.setText(i ? "-" : "+");
        b.setFontSize(m_pars.fontSize -2);
        b.setFontType("regular");
        b.setClickedCb([this, i] {
            if (i == 0) {
                m_edit->incValue(1.f, cfState::normal);
            } else {
                m_edit->incValue(-1.f, cfState::normal);
            }
        });
    }

    if (m_bindFunc) {
        m_bindFunc();
    }
}


}