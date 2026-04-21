//
// Created by sven on 01-04-26.
//

#include "UIElements/Div.h"
#include "UIElements/Text/Label.h"
#include "UIElements/DataBinding/NodeMemberVariableEdit.h"
#include "UIElements/Button/Button.h"

using namespace std;
using namespace glm;

namespace ara {

NodeMemberVariableEdit::NodeMemberVariableEdit() {
    setTypeName<NodeMemberVariableEdit>();
    setName(getTypeName<NodeMemberVariableEdit>());
}

NodeMemberVariableEdit::NodeMemberVariableEdit(const EditPar& par) {
    setTypeName<NodeMemberVariableEdit>();
    setName(getTypeName<NodeMemberVariableEdit>());

    UINodeStyle::addStyleClass(par.style);
    m_memVar = par.memVar;
    m_memVarName = par.memVarName;
    m_node = par.node;
    setLabelText(par.labelText);
    setSpacing(par.spacing);
    setLineHeight(par.lineHeight);
    setLabelWidth(par.labelWidth);
    setEditAlign(par.editAlign);
    setY(par.yOffs);
    if (par.options.has_value()) {
        m_options = par.options.value();
        setEditAlign(m_options->arr);
    }
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

    for (const auto& it : m_arrayEdit) {
        remove(it);
    }
    m_arrayEdit.clear();

    auto stdWidth = -m_labelWidth - m_spacing.x;
    static std::unordered_map<tpi, std::function<void(NodeMemberVariableEdit* ctx)>> createMap = {
        { tpi::tp_bool, [] (NodeMemberVariableEdit* ctx) { ctx->createCheckBox(std::any_cast<bool>(ctx->getMemVar()->get())); }},
        { tpi::tp_float, [stdWidth] (NodeMemberVariableEdit* ctx) { ctx->createSingleEdit<float>(std::any_cast<float>(ctx->getMemVar()->get()), stdWidth); }},
        { tpi::tp_int32, [stdWidth] (NodeMemberVariableEdit* ctx) { ctx->createSingleEdit<int32_t>(std::any_cast<int32_t>(ctx->getMemVar()->get()), stdWidth); }},
        { tpi::tp_string, [stdWidth] (NodeMemberVariableEdit* ctx) { ctx->createSingleEdit<std::string>( std::any_cast<std::string>(ctx->getMemVar()->get()), stdWidth); }},
        { tpi::tp_path, [stdWidth] (NodeMemberVariableEdit* ctx) { ctx->createPathEdit( std::any_cast<std::filesystem::path>(ctx->getMemVar()->get()), stdWidth); }},
        { tpi::tp_vector_float, [] (NodeMemberVariableEdit* ctx) { ctx->createVector<vector<float>>(); }},
        { tpi::tp_vector_int32, [] (NodeMemberVariableEdit* ctx) { ctx->createVector<vector<int32_t>>(); }},
        { tpi::tp_vector_string, [] (NodeMemberVariableEdit* ctx) { ctx->createVector<vector<std::string>>(); }},
        { tpi::tp_vec2, [] (NodeMemberVariableEdit* ctx) { ctx->createArrayEdit<vec2>(2); }},
        { tpi::tp_vec3, [] (NodeMemberVariableEdit* ctx) { ctx->createArrayEdit<vec3>(3); }},
        { tpi::tp_vec4, [] (NodeMemberVariableEdit* ctx) { ctx->createArrayEdit<vec4>(4); }},
        { tpi::tp_ivec2, [] (NodeMemberVariableEdit* ctx) { ctx->createArrayEdit<ivec2>(2); }},
        { tpi::tp_ivec3, [] (NodeMemberVariableEdit* ctx)  { ctx->createArrayEdit<ivec3>(3); }},
        { tpi::tp_ivec4, [] (NodeMemberVariableEdit* ctx) { ctx->createArrayEdit<ivec4>(4); }},
        { tpi::none, [] (NodeMemberVariableEdit* ctx) { LOGE << "NodeMemberVariableEdit::setEditValFromMemberVar Error: tpi is none"; }},
    };

    if (createMap.contains(m_memVar->typeIndex)) {
        createMap[m_memVar->typeIndex](this);
    } else {
        LOGE << "NodeMemberVariableEdit::setEditValFromMemberVar Error: tpi not found";
    }
}

void NodeMemberVariableEdit::createCheckBox(const bool val) {
    if (m_checkBoxButt) {
        remove(m_checkBoxButt);
    }

    m_checkBoxButt = &push<Button>(UINodePars{
        .pos = ivec2{ m_labelWidth + m_spacing.x, 0 },
        .size = { ivec2{ m_lineHeight, m_lineHeight } },
        .bgColor = m_stdBgColor,
        .style = getStyleClass()+".checkbox",
        .borderWidth = m_stdBorderWidth,
        .borderRadius = m_stdBorderRadius,
        .borderColor = m_stdBorderColor,
    });

    m_checkBoxButt->setIsToggle(true);
    m_checkBoxButt->toggle(val);
    m_checkBoxButt->setToggleCb([&](bool newVal) {
        if (m_memVar && !m_blockMemVarSet) {
            std::any anyVal = newVal;
            m_memVar->set(anyVal, 0);
        }
        m_blockMemVarSet = false;
    });

    // two-way binding
    m_node->setOnChangeCb(cbType::postChange, this, [this](Node*, const string& varName) {
        if (const auto actVal = std::any_cast<bool>(m_memVar->get());
            varName == m_memVarName && (*m_checkBoxButt->m_prop)() != actVal) {
            m_blockMemVarSet = true;
            m_checkBoxButt->toggle(actVal);
        }
    });
}

void NodeMemberVariableEdit::createPathEdit(const std::filesystem::path &val, const int32_t stdWidth) {
    if (m_pathLabel) {
        remove(m_pathLabel);
    }

    m_pathLabel = &push<Label>(LabelPars{
        .pos = ivec2{ m_labelWidth + m_spacing.x, 0 },
        .size = { ivec2{ stdWidth - m_stdBrowseButtWidth - m_spacing.x, m_lineHeight } },
        .style = getStyleClass()+".path",
        .bgColor = m_stdBgColor,
        .borderWidth = m_stdBorderWidth,
        .borderRadius = m_stdBorderRadius,
        .borderColor = m_stdBorderColor,
        .text = val.string(),
    });
    m_pathLabel->setOpt(Label::single_line | Label::front_ellipsis);

    // two way binding
    m_node->setOnChangeCb(cbType::postChange, this, [this](Node*, const string& varName) {
        if (const auto actVal = std::any_cast<filesystem::path>(m_memVar->get());
            varName == m_memVarName && m_pathLabel->getText() != actVal.string()) {
            m_pathLabel->setText(actVal.string());
        }
    });

    if (m_browseButt) {
        remove(m_browseButt);
    }

    m_browseButt = &push<Button>(LabelPars{
        .size = { ivec2{ m_stdBrowseButtWidth, m_lineHeight } },
        .style = getStyleClass()+".browseButton",
        .align = align::right,
        .bgColor = m_stdButtBgColor,
        .borderWidth = m_stdBorderWidth,
        .borderRadius = m_stdBorderRadius,
        .borderColor = m_stdBorderColor,
        .text = "Browse"
    });

    m_browseButt->setBackgroundColor(0.4f, 0.4f, 0.4f, 1.f, state::highlighted);
    m_browseButt->setClickedCb([&] {
        if (m_memVar) {
            const auto& suffixes = !m_options->allowSuffixes.empty() ? m_options->allowSuffixes : m_stdSuffixes;
            if (const auto fn = getWindow()->openFileDialog(suffixes); !fn.empty()) {
                std::any anyVal = std::filesystem::path(fn);
                m_memVar->set(anyVal, 0);
            }
        }
    });
}

void NodeMemberVariableEdit::setLabelText(const std::string& text) {
    m_text = text;
    if (m_label) {
        m_label->setText(text);
    }
}

int32_t NodeMemberVariableEdit::getEditWidth(const size_t nrEditFields) {
    return m_editAlign == arrange::horizontal ?
        static_cast<int32_t>((getContentSize().x - static_cast<float>(m_labelWidth + m_spacing.x))
                                / static_cast<float>(std::min(nrEditFields, m_numEditsPerRow)))
        : static_cast<int32_t>(getContentSize().x - static_cast<float>(m_labelWidth + m_spacing.x));
}

int32_t NodeMemberVariableEdit::getUnitHeight() const {
    if (!m_memVar) {
        return 0;
    }

    if (m_memVar->typeIndex <= tpi::tp_bool) {
        return 1;
    }

    size_t nrEdits = 1;
    if (tpi::tp_bool < m_memVar->typeIndex && m_memVar->typeIndex < tpi::tp_ivec2) {
        unordered_map<tpi, std::function<size_t()>> vecSizeMap {
            { tpi::tp_vector_float, [this] { return std::any_cast<vector<float>>(m_memVar->get()).size(); } },
            { tpi::tp_vector_int32, [this] { return std::any_cast<vector<int32_t>>(m_memVar->get()).size(); } },
            { tpi::tp_vector_string, [this] { return std::any_cast<vector<std::string>>(m_memVar->get()).size(); } },
        };
        nrEdits = vecSizeMap[m_memVar->typeIndex]();
    } else {
        static unordered_map<tpi, size_t> glmSizeMap {
            { tpi::tp_vec2, 2 },
            { tpi::tp_vec3, 3 },
            { tpi::tp_vec4, 4 },
            { tpi::tp_ivec2, 2 },
            { tpi::tp_ivec3, 3 },
            { tpi::tp_ivec4, 4 },
        };
        nrEdits = glmSizeMap[m_memVar->typeIndex];
    }

    if (m_editAlign == arrange::horizontal) {
        return static_cast<int32_t>(std::ceil( static_cast<float>(nrEdits) /  static_cast<float>(m_numEditsPerRow) ));
    }

    return static_cast<int32_t>(nrEdits);
}

}