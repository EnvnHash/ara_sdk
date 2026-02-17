//
// Created by sven on 16-02-26.
//

#include <Asset/AssetColor.h>
#include <Asset/AssetFont.h>
#include <Asset/AssetManager.h>
#include <DataModel/PropertyItemUi.h>
#include "UIElements/Text/TextBlock.h"
#include "UIWindow.h"

#ifdef ARA_USE_CLIP
#include "clip.h"
#endif

using namespace glm;
using namespace std;

namespace ara {

TextBlock::TextBlock(unsigned opt, int max_count)  {
    setTypeName<TextBlock>();
    setName(getTypeName<TextBlock>());
    setFocusAllowed(true);
    m_maxCount = max_count;

    Label::setColor(1, 1, 1, 1);
}

TextBlock::~TextBlock() {
    if (m_sharedRes && m_sharedRes->win) {
        static_cast<UIWindow *>(m_sharedRes->win)->removeGlobalMouseDownLeftCb(this);
    }
}

void TextBlock::init() {
    m_canReceiveDrag = true;

    m_caret = &push<Div>();
    m_caret->setVisibility(false);
    m_caret->excludeFromPadding(true);

    static_cast<UIWindow *>(m_sharedRes->win)->addGlobalMouseDownLeftCb(this, [this](hidData& data) {
        globalMouseDown(data);
    });

    m_offset = getContentOffset();
    initSelBgShader();
}

void TextBlock::initSelBgShader() {
    std::string vert = STRINGIFY(
        layout(location = 0) in vec4 position;  \n
        uniform nodeData {                      \n
            uniform mat4 mvp;                   \n
            uniform vec4 color;                 \n
        };                                      \n
        void main() {                           \n
            gl_Position = mvp * position;       \n
    });
    vert = ShaderCollector::getShaderHeader() + "\n// TextBlock selection background shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        layout(location = 0) out vec4 fragColor;    \n
        uniform nodeData {                          \n
            uniform mat4 mvp;                       \n
            uniform vec4 color;                     \n
        };                                          \n
        void main() {                               \n
            fragColor = color;                      \n
    });
    frag = ShaderCollector::getShaderHeader() + "\n// TextBlock selection background shader, frag\n" + frag;

    m_selBgShader = m_shCol->add("TextBlock_bgsel", vert, frag);
}

void TextBlock::loadStyleDefaults() {
    UINode::loadStyleDefaults();

    m_setStyleFunc[state::none][styleInit::fontFontSize]   = [this]() { setFontSize(18); };
    m_setStyleFunc[state::none][styleInit::fontFontFamily] = [this]() { setFontType("regular"); };
}

bool TextBlock::draw(uint32_t& objId) {
    Label::draw(objId);

    drawSelectionBg();  // Draw the selection background
    drawGlyphs(objId);  // Draw the glyphs

    return true;  // count up objId
}

bool TextBlock::drawIndirect(uint32_t& objId) {
    Div::drawIndirect(objId);
    drawSelectionBg();
    checkGlyphsPrepared(true);

    if (m_glyphsPrepared) {
        updateIndDrawData(true);
    }

    if (m_drawMan) {
        m_lblDB.drawSet = &m_drawMan->push(m_lblDB, this);
    }

    return true;  // count up objId
}

void TextBlock::drawSelectionBg() {
    if (getSelRange(m_charSelection)
        && m_state == state::selected) {
        if (!all(glm::equal(m_charSelection, m_lastSelRange))) {
            m_lastSelRange = m_charSelection;

            // rebuild background VAO
            if (m_drawImmediate && m_selBgShader) {
                if (!m_uniBlockBg.isInited()) {
                    m_uniBlockBg.addVarName("mvp", getHwMvp(), GL_FLOAT_MAT4);
                    m_uniBlockBg.addVarName("color", &m_bkSelColor[0], GL_FLOAT_VEC4);
                    m_uniBlockBg.init(m_selBgShader->getProgram(), "nodeData");
                }

                m_uniBlockBg.update();
            }

            prepareSelBgVao();
        }

        if (m_drawImmediate) {
            if (m_selBgShader) {
                m_selBgShader->begin();
            }
            m_uniBlockBg.bind();

            if (m_backVao.isInited()) {
                m_backVao.drawElements(GL_TRIANGLES, nullptr, GL_TRIANGLES, static_cast<int>(m_backIndices.size()));
            }
        } else {
            if (m_drawMan) {
                m_selBgDB.drawSet = &m_drawMan->push(m_selBgDB, this);
            }
        }
    }
}

void TextBlock::drawGlyphs(uint32_t& objId) {
    checkGlyphsPrepared();

    if (!m_glyphShader) {
        m_glyphShader = getSharedRes()->shCol->getStdGlyphShdr();

        m_uniBlockLbl.addVarName("mvp", getHwMvp(), GL_FLOAT_MAT4);
        m_uniBlockLbl.addVarName("tcolor", &m_color[0], GL_FLOAT_VEC4);
        m_uniBlockLbl.addVarName("mask", &m_mask[0], GL_FLOAT_VEC4);

        m_uniBlockLbl.init(m_glyphShader->getProgram(), "nodeData");
        m_updtUniBlock = true;
    }

    if (m_updtUniBlock) {
        m_uniBlockLbl.update();
        m_updtUniBlock = false;
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_riFont->getTexId());

    m_glyphShader->begin();
    m_glyphShader->setUniform1i("stex", 0);
    m_uniBlockLbl.bind();

    if (m_vao.isInited()) {
        m_vao.drawElements(GL_TRIANGLES);
    }
}

Font *TextBlock::UpdateDGV(bool *checkFontTexture) {
    if (!m_sharedRes || !m_sharedRes->res) {
        return nullptr;
    }

    const auto font = getSharedRes()->res->getGLFont(m_fontType, m_fontSize, getPixRatio());
    if (!font) {
        LOGE << "[ERROR] TextBlock::UpdateDGV() / Cannot get font for " << m_fontType << "   size=" << m_fontSize;
        return nullptr;
    }

    m_riFont     = font;
    m_renderText = hasOpt(pass) ? string(m_text.size(), '*') : m_text;

    vec4 mask{m_offset.x, m_offset.y, m_offset.x + m_tContSize.x, m_offset.y + m_tContSize.y};
    int  lidx;

    if (!hasOpt(manual_space)) {
        m_tSize = m_tContSize;
    }

    m_fontDGV.setPixRatio(getPixRatio());
    m_fontDGV.setTabPixSize(m_TabSize);
    // process input text, break up in lines
    m_fontDGV.Process(m_riFont, m_tSize, m_tSep, m_tAlignX, m_renderText, !hasOpt(single_line));

    // Calculate offset
    if ((lidx = m_fontDGV.getLineIndexByCharIndex(m_caretIndex)) >= 0) {
        auto cpos = m_fontDGV.getCaretPos(m_caretIndex);
        auto pa = m_riFont->getPixAscent();

        // resulting x-position
        auto x1 = cpos.x + m_offset.x;
        auto x2 = x1 + 2;
        auto y1 = m_fontDGV.m_vline[lidx].getYSelRange(0) + m_offset.y + pa;
        auto y2 = m_fontDGV.m_vline[lidx].getYSelRange(1) + m_offset.y + pa;

        // the beginning of the rendered text will be outside the mask add an
        // offset to move it into the non-mask area
        if (x1 < mask.x) {
            m_offset.x = -(cpos[0] - mask.x);
        }
        if (x2 > mask.z) {
            m_offset.x = std::max(m_offset.x, -(2 + cpos[0] - mask.z));
        }
        if (y1 < mask.y) {
            m_offset.y = -(pa + m_fontDGV.m_vline[lidx].getYSelRange(0) - mask.y);
        }
        if (y2 > mask.w) {
            m_offset.y = std::max(m_offset.y, -(pa + m_fontDGV.m_vline[lidx].getYSelRange(1) - mask.w));
        }
    }

    if (m_text.empty()) {
        m_offset = getContentOffset();
    }
    return m_riFont;
}

void TextBlock::updateFontGeo() {
    if (!m_riFont) {
        return;
    }

    memset(&m_alignOffset[0], 0, 8);

    if (m_tAlignY == valign::bottom) {
        m_bs            = m_fontDGV.getPixSize();
        m_bs.y          = std::max<float>(m_bs.y, m_riFont->getPixAscent());
        m_alignOffset.y = m_tContSize.y - m_bs.y;
    } else if (m_tAlignY == valign::center) {
        m_bs            = m_fontDGV.getPixSize();
        m_bs.y          = std::max<float>(m_bs.y, m_riFont->getPixAscent());
        m_alignOffset.y = m_tContSize.y * 0.5f - m_bs.y * 0.5f;
    }

    // take the matrix of the helper content Div, since this will use the
    // Label's content transformation
    m_bo.x = m_offset.x + m_alignOffset.x;
    m_bo.y = m_offset.y + m_riFont->getPixAscent() + m_alignOffset.y;
    m_mask = calculateMask();
    //memcpy(&m_modMvp[0][0], &m_mvp[0][0], sizeof(float) * 16);
    std::copy_n(&m_mvp[0][0], 16, &m_modMvp[0][0]);

    m_mask *= m_riFont->getPixRatio();
    m_bo *= m_riFont->getPixRatio();
    m_modMvp = m_modMvp * scale(vec3{1.f / getPixRatio(), 1.f / getPixRatio(), 1.f});
}

void TextBlock::prepareSelBgVao() {
    list<pair<vec2, vec2>> lines;  // Left/Top,  Right/Bottom
    for (e_fontline &l : m_fontDGV.m_vline) {
        if (l.ptr[0] && l.ptr[1]) {
            std::array<int32_t, 2> ci{};  /// first and last character index

            ci[0] = l.ptr[0]->chidx;  // asign the character index of the
                                         // first character in the actual line
            ci[1] = l.ptr[1]->chidx;  // asign the character index of the
                                         // last character in the actual line

            // check if this line is within the range of selected charaters
            if (!(ci[0] > m_charSelection[1] || ci[1] < m_charSelection[0])) {
                cp.x = ci[0] > m_charSelection[0] ? m_tPos.x / l.m_pixRatio
                                                  : m_fontDGV.getCaretPos(m_charSelection[0])[0];
                cp.y = ci[1] < m_charSelection[1] ? (m_tPos.x + m_tSize.x)
                                                  : m_fontDGV.getCaretPos(m_charSelection[1])[0];

                vec2 posLT{};
                posLT.x = cp.x + m_offset.x + m_alignOffset.x;
                posLT.y = l.y / l.m_pixRatio + m_offset.y + m_alignOffset.y;

                vec2 posRB{};
                posRB.x = cp.y + m_offset.x + m_alignOffset.x;
                posRB.y = posLT.y + (l.yrange[1] - l.yrange[0]) / l.m_pixRatio;

                // check if the selected part of this line is within the visible
                // range
                if (posLT.x < m_mask.z && posRB.x > m_mask.x && posLT.y < m_mask.w && posRB.y > m_mask.y) {
                    posLT.x = std::max<float>(posLT.x, m_mask.x);
                    posLT.y = std::max<float>(posLT.y, m_mask.y);
                    posRB.x = std::min<float>(posRB.x, m_mask.z);
                    posRB.y = std::min<float>(posRB.y, m_mask.w);

                    auto lSize = posRB - posLT;
                    lines.emplace_back(posLT, lSize);
                }
            }
        }
    }

    if (m_backPos.size() != lines.size() * 4) {
        m_backPos.resize(lines.size() * 4);
    }
    if (m_backIndices.size() != lines.size() * 6) {
        m_backIndices.resize(lines.size() * 6);
    }

    int i = 0;
    for (auto &[start, end] : lines) {
        for (auto &v : m_vtxPos) {
            m_backPos[i] = vec4{glm::min(m_size, start + v * end) * getPixRatio(), 0.f, 1.f};
            ++i;
        }
    }

    i = 0;
    for (auto &it : m_backIndices) {
        it = m_elmInd[i % 6] + (i / 6) * 4;
        ++i;
    }

    if (m_drawImmediate) {
        if (!m_backVao.isInited()) {
            m_backVao.init("position:4f");
        }
        if (m_backVao.getNrVertices() < lines.size() * 4) {
            m_backVao.resize(static_cast<GLuint>(lines.size()) * 4);
        }
        if (!m_backPos.empty()) {
            m_backVao.upload(CoordType::Position, &m_backPos[0][0], static_cast<uint32_t>(lines.size()) * 4);
        }
        if (!m_backIndices.empty()) {
            m_backVao.setElemIndices(static_cast<uint32_t>(m_backIndices.size()), &m_backIndices[0]);
        }
    } else {
        if (!m_sharedRes || !m_sharedRes->objSel) {
            return;
        }
        if (m_selBgDB.vaoData.size() != m_backPos.size()) {
            m_selBgDB.vaoData.resize(m_backPos.size());
        }
        if (m_selBgDB.indices.size() != m_backIndices.size()) {
            m_selBgDB.indices.resize(m_backIndices.size());
        }

        auto dIt = m_selBgDB.vaoData.begin();

        for (auto &it : m_backPos) {
            dIt->pos = m_mvpHw * it;
            memcpy(&dIt->color[0], &m_bkSelColor[0], sizeof(float) * 4);
            dIt->aux2.w = m_zPos;
            dIt->aux3.x = 4.f;  // type indicator (4=GenQuad)
            dIt->aux3.w = 1.f;  // alpha
            ++dIt;
        }
    }
}

void TextBlock::clearDs() {
    Label::clearDs();
    m_selBgDB.drawSet = nullptr;
}

void TextBlock::mouseDrag(hidData& data) {
    m_mousePosCr = data.mousePosNodeRel / getParentContentScale() - m_alignOffset;

    if (m_mouseEvent & 1) {
        auto cpos = getCaretByPixPos(m_mousePosCr.x, m_mousePosCr.y);

        m_caretIndex    = cpos;
        m_caretRange[1] = cpos;

        if (!m_drawImmediate) {
            reqUpdtTree();
        }
        setDrawFlag();
    }
    data.consumed = true;
}

void TextBlock::mouseDown(hidData& data) {
    vec2 p = data.mousePosNodeRel / getParentContentScale();
    int cpos = getCaretByPixPos(p.x - m_alignOffset.x, p.y - m_alignOffset.y);

    if (cpos >= 0) {
        m_caretIndex    = cpos;
        m_caretRange[0] = m_caretRange[1] = m_caretIndex;
        m_mouseEvent                      = 1;

        if (!m_drawImmediate) {
            reqUpdtTree();
        }
    }

    setSelected(true, true);

    if (data.isDoubleClick) {
        setSelRangeAll();
    }
    data.consumed = true;
}

void TextBlock::mouseUp(hidData& data) {
    m_mouseEvent = 0;
    data.consumed = true;
}

void TextBlock::keyDown(hidData& data) {
    if (data.key == ARA_KEY_C && data.ctrlPressed) {
#ifdef ARA_USE_CLIP
        LOG << "TextBlock::keyDown " << m_renderText.substr(m_charSelection.x, m_charSelection.y - m_charSelection.x);
        clip::set_text(m_renderText.substr(m_charSelection.x, m_charSelection.y - m_charSelection.x));
#endif
    }
}

void TextBlock::globalMouseDown(hidData& data) {
    if (isInited()
        && m_state == state::selected
        && !(static_cast<uint32_t>(data.objId) >= getId() && static_cast<uint32_t>(data.objId) <= getMaxChildId())) {
        setSelected(false, true);
        m_sharedRes->setDrawFlag();
    }
}

void TextBlock::setText(const std::string &str) {
    bool updt = str.size() != m_text.size();
    m_text.assign(str, 0, std::min(m_maxCount, static_cast<int>(str.size())));
    m_caretIndex = static_cast<int>(m_text.size());
    clearSelRange();
    reqUpdtGlyphs(updt);
}

bool TextBlock::validateInputToString(int ch) const {
    std::string str(m_text);
    if (!str.empty()) {
        str.insert(std::max<size_t>(std::min<size_t>(m_caretIndex, str.size()), 0), 1, static_cast<char>(ch));
    } else {
        str.insert(str.begin(), static_cast<char>(ch));
    }

    return true;
}

int TextBlock::getCaretByPixPos(float px, float py) {
    if (!m_riFont) {
        return 0;
    }
    int off_bound = 0;
    int idx = m_fontDGV.getCharIndexByPixPos(px, py - m_riFont->getPixAscent(), m_tPos[0] + m_offset.x,
                                             m_tPos[1] + m_offset.y, off_bound);
    m_caretIndex = idx;
    return idx;
}

bool TextBlock::setSelRangeAll() {
    int len = static_cast<int>(m_text.size());
    return setSelRange(0, len);
}

bool TextBlock::setSelRange(int loIndex, int highIndex) {
    int len = static_cast<int>(m_text.size());

    loIndex = std::clamp(loIndex, 0, len);
    highIndex = std::clamp(highIndex, 0, len);

    if (loIndex > highIndex) {
        return false;
    }

    m_caretRange[0] = loIndex;
    m_caretRange[1] = highIndex;
    return true;
}

bool TextBlock::getSelRange(ivec2 &range) {
    if (m_caretRange[0] == m_caretRange[1]) {
        memset(&range[0], 0, sizeof(int) * 2);
        return false;
    }

    range.x = std::min(m_caretRange.x, m_caretRange.y);
    range.y = std::max(m_caretRange.x, m_caretRange.y);

    if (range.x < 0) {
        range.x = 0;
    }
    if (range.y > static_cast<int>(m_text.size())) {
        range.y = static_cast<int>(m_text.size());
    }
    return true;
}

void TextBlock::clearSelRange() {
    m_caretRange[0] = m_caretRange[1] = 0;
}

bool TextBlock::eraseContent(int lo_index, int hi_index) {
    const int len = static_cast<int>(m_text.size());

    if (len <= 0) {
        return false;
    }
    if (lo_index > hi_index) {
        return false;
    }

    lo_index = std::clamp(lo_index, 0, len);
    hi_index = std::clamp(hi_index, 0, len);

    m_text.erase(lo_index, hi_index - lo_index);
    reqUpdtGlyphs(true);
    return true;
}

int TextBlock::validateCaretPos(int cpos) const {
    if (cpos < 0) {
        cpos = 0;
    }
    if (cpos > static_cast<int>(m_text.size())) {
        cpos = static_cast<int>(m_text.size());
    }
    return cpos;
}

void TextBlock::updateStyleIt(ResNode *node, state st, const std::string& styleClass) {
    Label::updateStyleIt(node, st, styleClass);

    if (node->hasValue("edit-align")) {
        auto p = node->splitNodeValue("edit-align");

        for (auto &par : p) {
            if (par == "left") m_tAlignX = align::left;
            else if (par == "center") m_tAlignX = align::center;
            else if (par == "right") m_tAlignX = align::right;
            else if (par == "justify") m_tAlignX = align::justify;
            else if (par == "justify-ex") m_tAlignX = align::justify_ex;
        }
    }

    if (node->hasValue("edit-valign")) {
        ParVec p   = node->splitNodeValue("edit-valign");
        auto auxAlign = valign::center;

        for (std::string &par : p) {
            if (par == "top") auxAlign = valign::top;
            else if (par == "vcenter" || par == "center") auxAlign = valign::center;
            else if (par == "bottom") auxAlign = valign::bottom;
        }

        m_tAlignY                                = auxAlign;
        m_setStyleFunc[st][styleInit::textValign] = [this, auxAlign] { m_tAlignY = auxAlign; };
    }

    if (auto f = node->findNode<AssetFont>("font")) {
        int         size = f->value<int32_t>("size", 0);
        std::string font = f->getValue("font");

        m_setStyleFunc[st][styleInit::fontFontSize]   = [this, size]() { setFontSize(size); };
        m_setStyleFunc[st][styleInit::fontFontFamily] = [this, font]() { setFontType(font); };
    }

}

void TextBlock::clearProp() {
    if (m_stringProp) {
        removeOnChanged<std::string>(*m_stringProp);
        setOnLostFocusCb(nullptr);
    }
}

void TextBlock::setPropItem(Item *item) {
    if (item && item->isPropertyItem) {
        if (item->m_typeId == tpi::tp_string) {
            setProp<std::string>( *dynamic_cast<PropertyItemUi<std::string>*>(item)->getPtr());
        }
    }
}

void TextBlock::setBkSelColor(vec4 c) {
    m_bkSelColor     = c;
    m_glyphsPrepared = false;
}

}  // namespace ara
