//
// Created by sven on 01-04-26.
//

#include "UIElements/Div.h"
#include "UIElements/Text/Label.h"
#include "UIElements/DataBinding/NodeMemberVariableEdit.h"

using namespace std;
using namespace glm;

namespace ara {

NodeMemberVariableEdit::NodeMemberVariableEdit() {
    setTypeName<NodeMemberVariableEdit>();
    setName(getTypeName<NodeMemberVariableEdit>());
}

void NodeMemberVariableEdit::init() {
    setHeight(m_lineHeight);

    m_label = &push<Label>(LabelPars{
        .pos = ivec2{ 0, 0 },
        .size = ivec2{ m_labelWidth, m_lineHeight },
        .style = getStyleClass()+".label",
        .align = align::left,
        .valign = valign::top,
        .color = vec4{ 1.f, 1.f, 1.f, 1.f },
        .text = m_text + ":",
        .textAlignX = align::left,
        .textAlignY = valign::center,
        .fontType = "regular",
        .fontHeight = 22
    });

    if (m_memVar) {
        setEditValFromMemberVar();
    }
}

void NodeMemberVariableEdit::setEditValFromMemberVar() {
    if (!m_memVar) return;

    if (m_edit) {
        remove(m_edit);
    }

    for (const auto& it : m_arrayEdit) {
        remove(it);
    }
    m_arrayEdit.clear();

    auto stdWidth = -m_labelWidth -m_xSpacing;
    static std::unordered_map<tpi, std::function<void(NodeMemberVariableEdit* ctx)>> createMap = {
        { tpi::tp_float, [stdWidth] (NodeMemberVariableEdit* ctx) { ctx->createSingleEdit<float>(std::any_cast<float>(ctx->getMemVar()->get()), stdWidth); }},
        { tpi::tp_int32, [stdWidth] (NodeMemberVariableEdit* ctx) { ctx->createSingleEdit<int32_t>(std::any_cast<int32_t>(ctx->getMemVar()->get()), stdWidth); }},
        { tpi::tp_string, [stdWidth] (NodeMemberVariableEdit* ctx) { ctx->createSingleEdit<std::string>( std::any_cast<std::string>(ctx->getMemVar()->get()), stdWidth); }},
        { tpi::tp_vector_float, [] (NodeMemberVariableEdit* ctx) { ctx->createVector<vector<float>>(); }},
        { tpi::tp_vector_int32, [] (NodeMemberVariableEdit* ctx) { ctx->createVector<vector<int32_t>>(); }},
        { tpi::tp_vector_string, [] (NodeMemberVariableEdit* ctx) { ctx->createVector<vector<std::string>>(); }},
        { tpi::tp_vec2, [] (NodeMemberVariableEdit* ctx) { ctx->createArrayEdit<vec2>(2); }},
        { tpi::tp_vec3, [] (NodeMemberVariableEdit* ctx) { ctx->createArrayEdit<vec3>(3); }},
        { tpi::tp_vec4, [] (NodeMemberVariableEdit* ctx) { ctx->createArrayEdit<vec4>(4); }},
        { tpi::tp_ivec2, [] (NodeMemberVariableEdit* ctx) { ctx->createArrayEdit<ivec2>(2); }},
        { tpi::tp_ivec3, [] (NodeMemberVariableEdit* ctx)  { ctx->createArrayEdit<ivec3>(3); }},
        { tpi::tp_ivec4, [] (NodeMemberVariableEdit* ctx) { ctx->createArrayEdit<ivec4>(4); }},
    };

    createMap[m_memVar->typeIndex](this);
}

void NodeMemberVariableEdit::setLabelText(const std::string& text) {
    m_text = text;
    if (m_label) {
        m_label->setText(text);
    }
}

int32_t NodeMemberVariableEdit::getEditWidth(const size_t nrEditFields) {
    return static_cast<int32_t>((getContentSize().x - static_cast<float>(m_labelWidth + m_xSpacing)) / static_cast<float>(std::min(nrEditFields, m_numEditsPerRow)));
}


}