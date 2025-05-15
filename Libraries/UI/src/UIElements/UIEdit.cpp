#include <Asset/AssetColor.h>
#include <Asset/AssetFont.h>
#include <Asset/AssetManager.h>
#include <DataModel/PropertyItemUi.h>
#include "UIEdit.h"
#include "UIWindow.h"

using namespace glm;
using namespace std;

namespace ara {

UIEdit::UIEdit(unsigned opt, int max_count)  {
    setName(getTypeName<UIEdit>());

    m_tOpt = 0;
    m_tOpt |= single_line;
    m_tOpt |= opt;

    m_MaxCount = max_count;

    m_minInt = numeric_limits<int>::min();
    m_maxInt = numeric_limits<int>::max();

    m_minF = numeric_limits<float>::min();
    m_maxF = numeric_limits<float>::max();

    Label::setColor(1, 1, 1, 1);
}

UIEdit::~UIEdit() {
    if (m_sharedRes && m_sharedRes->win) {
        static_cast<UIWindow *>(m_sharedRes->win)->removeGlobalMouseDownLeftCb(this);
    }
}

void UIEdit::init() {
    m_canReceiveDrag = true;
    m_focusAllowed   = true;

    m_caret = addChild<Div>();
    m_caret->setVisibility(false);
    m_caret->excludeFromPadding(true);

    static_cast<UIWindow *>(m_sharedRes->win)->addGlobalMouseDownLeftCb(this, [this](hidData& data) {
        globalMouseDown(data);
    });

    m_Offset = getContentOffset();
    initSelBgShader();
}

void UIEdit::initSelBgShader() {
    std::string vert = STRINGIFY(layout(location = 0) in vec4 position;\n
        uniform nodeData { \n
            uniform mat4 mvp; \n
            uniform vec4 color; \n
        }; \n
        void main() { \n
            gl_Position = mvp * position; \n
    });
    vert = ShaderCollector::getShaderHeader() + "\n// UIEdit selection background shader, vert\n" + vert;

    std::string frag = STRINGIFY(layout(location = 0) out vec4 fragColor;\n
        uniform nodeData { \n
            uniform mat4 mvp; \n
            uniform vec4 color; \n
        }; \n
        void main() { \n
            fragColor = color; \n
    });
    frag = ShaderCollector::getShaderHeader() + "\n// UIEdit selection background shader, frag\n" + frag;

    m_selBgShader = m_shCol->add("UIEdit_bgsel", vert, frag);
}

void UIEdit::loadStyleDefaults() {
    UINode::loadStyleDefaults();

    m_setStyleFunc[state::none][styleInit::fontFontSize]   = [this]() { setFontSize(18); };
    m_setStyleFunc[state::none][styleInit::fontFontFamily] = [this]() { setFontType("regular"); };
    m_setStyleFunc[state::none][styleInit::caretColor]     = [this]() { setCaretColor(1.f, 1.f, 1.f, 1.f); };
}

bool UIEdit::draw(uint32_t& objId) {
    Label::draw(objId);

    drawSelectionBg();  // Draw the selection background
    drawGlyphs(objId);  // Draw the glyphs
    drawCaret();        // Draw caret

    return true;  // count up objId
}

bool UIEdit::drawIndirect(uint32_t& objId) {
    Div::drawIndirect(objId);
    drawSelectionBg();
    checkGlyphsPrepared(true);

    if (m_glyphsPrepared) {
        updateIndDrawData(true);
    }

    drawCaret(false);

    if (m_drawMan) {
        m_lblDB.drawSet = &m_drawMan->push(m_lblDB, this);
    }

    return true;  // count up objId
}

void UIEdit::drawSelectionBg() {
    if (getSelRange(m_charSelection) && m_state == state::selected) {
        if (!glm::all(glm::equal(m_charSelection, m_lastSelRange))) {
            m_lastSelRange = m_charSelection;

            // rebuild background VAO
            if (m_drawImmediate && m_selBgShader) {
                if (!m_uniBlockBg.isInited()) {
                    m_uniBlockBg.addVarName("mvp", getHwMvp(), GL_FLOAT_MAT4);
                    m_uniBlockBg.addVarName("color", &m_BkSelColor[0], GL_FLOAT_VEC4);
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

void UIEdit::drawGlyphs(uint32_t& objId) {
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

    if (m_vao.isInited()) m_vao.drawElements(GL_TRIANGLES);
}

void UIEdit::drawCaret(bool forceCaretVaoUpdt) {
    if (!m_caret) {
        return;
    }

    bool updtTree = false;

    if (m_state == state::selected) {
        if (!m_caret->isVisible()) {
            m_caret->setVisibility(true);
            m_caret->setSize(static_cast<int>(static_cast<float>(m_caretWidth) / getParentContentScale().x),
                            static_cast<int>(m_riFont->getPixHeight()));
            m_caret->setBackgroundColor(m_caretColor);
            updtTree = true;
        }

        m_FontDGV.getCaretPos(m_tCaretPos, m_CaretIndex);
        m_tCaretPos = glm::floor(m_tCaretPos + m_Offset + m_alignOffset);

        // if there is no text, set the caret corresponding to the text format
        if (m_Text.empty() && (m_tAlign_X == align::right || m_tAlign_X == align::center)) {
            if (m_tAlign_X == align::right) {
                m_tCaretPos.x = getContentSize().x;
            } else if (m_tAlign_X == align::center) {
                m_tCaretPos.x = getContentSize().x * 0.5f;
            }
        }

        for (int i = 0; i < 2; i++) {
            posLimit[i] = m_size[i] - (m_padding[i + 2] + static_cast<float>(m_borderWidth));
        }

        if (!glm::all(glm::equal(m_caret->getPos(), m_tCaretPos))) {
            m_caret->setPos(static_cast<int>(std::min(m_tCaretPos.x, posLimit.x)),
                            static_cast<int>(std::min(m_tCaretPos.y, posLimit.y)));
        }

    } else {
        if (m_caret->isVisible()) {
            m_caret->setVisibility(false);
            updtTree = true;
        }
    }

    if (!m_drawImmediate) {
        m_caret->updateDrawData();
        if (forceCaretVaoUpdt) {
            m_caret->pushVaoUpdtOffsets();
        }
        if (updtTree) {
            reqUpdtTree();
        }
    }
}

Font *UIEdit::UpdateDGV(bool *checkFontTexture) {
    if (!m_sharedRes || !m_sharedRes->res) {
        return nullptr;
    }

    const auto font = getSharedRes()->res->getGLFont(m_fontType, m_fontSize, getPixRatio());
    if (!font) {
        LOGE << "[ERROR] UIEdit::UpdateDGV() / Cannot get font for " << m_fontType << "   size=" << m_fontSize;
        return nullptr;
    }

    m_riFont     = font;
    m_RenderText = hasOpt(pass) ? string(m_Text.size(), '*') : m_Text;

    vec4 mask{m_Offset.x, m_Offset.y, m_Offset.x + m_tContSize.x, m_Offset.y + m_tContSize.y};
    int  lidx;

    if (!hasOpt(manual_space)) {
        m_tSize = m_tContSize;
    }

    m_FontDGV.setPixRatio(getPixRatio());
    m_FontDGV.setTabPixSize(m_TabSize);
    // process input text, break up in lines
    m_FontDGV.Process(m_riFont, m_tSize, m_tSep, m_tAlign_X, m_RenderText, !hasOpt(single_line));

    // Calculate offset
    if ((lidx = m_FontDGV.getLineIndexByCharIndex(m_CaretIndex)) >= 0) {
        vec2 cpos{};
        m_FontDGV.getCaretPos(cpos, m_CaretIndex);

        float pa = m_riFont->getPixAscent();

        // resulting x-position
        m_x1 = cpos.x + m_Offset.x;
        m_x2 = m_x1 + 2;
        m_y1 = m_FontDGV.vline[lidx].getYSelRange(0) + m_Offset.y + pa;
        m_y2 = m_FontDGV.vline[lidx].getYSelRange(1) + m_Offset.y + pa;

        // the beginning of the rendered text will be outside the mask add an
        // offset to move it into the non-mask area
        if (m_x1 < mask.x) {
            m_Offset.x = -(cpos[0] - mask.x);
        }
        if (m_x2 > mask.z) {
            m_Offset.x = std::max(m_Offset.x, -(2 + cpos[0] - mask.z));
        }
        if (m_y1 < mask.y) {
            m_Offset.y = -(pa + m_FontDGV.vline[lidx].getYSelRange(0) - mask.y);
        }
        if (m_y2 > mask.w) {
            m_Offset.y = std::max(m_Offset.y, -(pa + m_FontDGV.vline[lidx].getYSelRange(1) - mask.w));
        }
    }

    if (m_Text.empty()) {
        m_Offset = getContentOffset();
    }
    return m_riFont;
}

void UIEdit::updateFontGeo() {
    if (!m_riFont) {
        return;
    }

    memset(&m_alignOffset[0], 0, 8);

    if (m_tAlign_Y == valign::bottom) {
        m_bs            = m_FontDGV.getPixSize();
        m_bs.y          = std::max<float>(m_bs.y, m_riFont->getPixAscent());
        m_alignOffset.y = m_tContSize.y - m_bs.y;
    } else if (m_tAlign_Y == valign::center) {
        m_bs            = m_FontDGV.getPixSize();
        m_bs.y          = std::max<float>(m_bs.y, m_riFont->getPixAscent());
        m_alignOffset.y = m_tContSize.y * 0.5f - m_bs.y * 0.5f;
    }

    // take the matrix of the helper content Div, since this will use the
    // Label's content transformation
    m_bo.x = m_Offset.x + m_alignOffset.x;
    m_bo.y = m_Offset.y + m_riFont->getPixAscent() + m_alignOffset.y;
    m_mask = calculateMask();
    memcpy(&m_modMvp[0][0], &m_mvp[0][0], sizeof(float) * 16);

    m_mask *= m_riFont->getPixRatio();
    m_bo *= m_riFont->getPixRatio();
    m_modMvp = m_modMvp * scale(vec3{1.f / getPixRatio(), 1.f / getPixRatio(), 1.f});
}

void UIEdit::prepareSelBgVao() {
    list<pair<vec2, vec2>> lines;  // Left/Top,  Right/Bottom
    for (e_fontline &l : m_FontDGV.vline) {
        if (l.ptr[0] && l.ptr[1]) {
            m_tCi[0] = l.ptr[0]->chidx;  // asign the character index of the
                                         // first character in the actual line
            m_tCi[1] = l.ptr[1]->chidx;  // asign the character index of the
                                         // last character in the actual line

            // check if this line is within the range of selected charaters
            if (!(m_tCi[0] > m_charSelection[1] || m_tCi[1] < m_charSelection[0])) {
                cp.x = m_tCi[0] > m_charSelection[0] ? m_tPos.x / l.m_pixRatio
                                                     : m_FontDGV.getCaretPos(aux, m_charSelection[0])[0];
                cp.y = m_tCi[1] < m_charSelection[1] ? (m_tPos.x + m_tSize.x)
                                                     : m_FontDGV.getCaretPos(aux, m_charSelection[1])[0];

                m_posLT.x = cp.x + m_Offset.x + m_alignOffset.x;
                m_posLT.y = l.y / l.m_pixRatio + m_Offset.y + m_alignOffset.y;
                m_posRB.x = cp.y + m_Offset.x + m_alignOffset.x;
                m_posRB.y = m_posLT.y + (l.yrange[1] - l.yrange[0]) / l.m_pixRatio;

                // check if the selected part of this line is within the visible
                // range
                if (m_posLT.x < m_mask.z && m_posRB.x > m_mask.x && m_posLT.y < m_mask.w && m_posRB.y > m_mask.y) {
                    m_posLT.x = std::max<float>(m_posLT.x, m_mask.x);
                    m_posLT.y = std::max<float>(m_posLT.y, m_mask.y);
                    m_posRB.x = std::min<float>(m_posRB.x, m_mask.z);
                    m_posRB.y = std::min<float>(m_posRB.y, m_mask.w);

                    m_lSize = m_posRB - m_posLT;
                    lines.emplace_back(m_posLT, m_lSize);
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
    for (auto &it : lines)
        for (auto &v : m_vtxPos) {
            m_backPos[i] = vec4{glm::min(m_size, it.first + v * it.second) * getPixRatio(), 0.f, 1.f};
            ++i;
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

            memcpy(&dIt->color[0], &m_BkSelColor[0], sizeof(float) * 4);
            dIt->aux2.w = m_zPos;
            dIt->aux3.x = 4.f;  // type indicator (4=GenQuad)
            ++dIt;
        }
    }
}

void UIEdit::clearDs() {
    Label::clearDs();
    m_selBgDB.drawSet = nullptr;
}

void UIEdit::keyDown(hidData& data) {
    if (m_blockEdit) {
        return;
    }

    setDrawFlag();

    // enter or return
    if (data.key == GLSG_KEY_ENTER || data.key == GLSG_KEY_KP_ENTER) {
        if (hasOpt(single_line)) {
            checkLimits();
        } else {
            m_CaretIndex = insertChar('\r', m_CaretIndex, true);
        }

        clampValue();  // in case of number double that they are within the valid range
        setSelected(false, true);
        drawCaret();

        for (const auto& cb : m_onEnterCb | views::values) {
            cb(m_Text);
        }

        onLostFocus();
        return;
    }

    // ctrl + z, shift+ctrl+z and ctrl+y will most likely be used for undo / redo
    if ((data.key == GLSG_KEY_Z && data.ctrlPressed) ||
        (data.key == GLSG_KEY_Z && data.ctrlPressed && data.shiftPressed) ||
        (data.key == GLSG_KEY_Y && data.ctrlPressed)) {
        onLostFocus();
        return;
    }

    if (data.key == GLSG_KEY_TAB) {
        if (hasOpt(accept_tabs)) {
            m_CaretIndex = insertChar('\t', m_CaretIndex, true);
        } else {
            setSelected(false, true);
            drawCaret();
            clampValue();  // in case of number double that they are within the valid range
            onLostFocus();
        }
    }

    // increment decrement for integers and floats
    if (hasOpt(num_int) || hasOpt(num_fp)) {
        if (data.key == GLSG_KEY_UP) {
            incValue(1.f, data.shiftPressed ? cfState::coarse : data.ctrlPressed ? cfState::fine : cfState::normal);
        } else if (data.key == GLSG_KEY_DOWN) {
            incValue(-1.f, data.shiftPressed ? cfState::coarse : data.ctrlPressed ? cfState::fine : cfState::normal);
        }
    }

    if (data.shiftPressed) {
        if (data.key == GLSG_KEY_LEFT && m_CaretIndex > 0) {
            if (!getSelRange(m_charSelection)) {
                m_CaretRange[0] = m_CaretIndex;
            }
            --m_CaretIndex;
            m_CaretRange[1] = m_CaretIndex;
            drawCaret();
            if (!m_drawImmediate) {
                prepareSelBgVao();
                reqUpdtTree();
            }

            return;
        }

        if (data.key == GLSG_KEY_RIGHT && m_CaretIndex < static_cast<int>(m_Text.size())) {
            if (!getSelRange(m_charSelection)) {
                m_CaretRange[0] = m_CaretIndex;
            }
            ++m_CaretIndex;
            m_CaretRange[1] = m_CaretIndex;
            drawCaret();
            if (!m_drawImmediate) {
                prepareSelBgVao();
                reqUpdtTree();
            }
            return;
        }

        if (data.key == GLSG_KEY_HOME) {
            if (!getSelRange(m_charSelection)) {
                m_CaretRange[0] = m_CaretIndex;
            }
            m_CaretIndex    = 0;
            m_CaretRange[1] = m_CaretIndex;
            drawCaret();
            if (!m_drawImmediate) {
                prepareSelBgVao();
                reqUpdtTree();
            }
            return;
        }

        if (data.key == GLSG_KEY_END) {
            if (!getSelRange(m_charSelection)) {
                m_CaretRange[0] = m_CaretIndex;
            }
            m_CaretIndex = m_CaretIndex = static_cast<int>(m_Text.size());
            m_CaretRange[1]             = m_CaretIndex;
            drawCaret();
            if (!m_drawImmediate) {
                prepareSelBgVao();
                reqUpdtTree();
            }
            return;
        }

        return;
    }

    if (!getSelRange(m_charSelection)) {
        if (data.key == GLSG_KEY_BACKSPACE && (m_CaretIndex > 0 && !m_Text.empty())) {
            m_Text.erase(std::min<int>(std::max<int>((m_CaretIndex--) - 1, 0), static_cast<int>(m_Text.size()) - 1), 1);
            reqUpdtGlyphs(true);
        } else if (data.key == GLSG_KEY_DELETE && (m_CaretIndex < static_cast<int>(m_Text.size()))) {
            m_Text.erase(m_CaretIndex, 1);
            reqUpdtGlyphs(true);
        } else if (data.key == GLSG_KEY_LEFT && m_CaretIndex > 0) {
            m_CaretIndex--;
        } else if (data.key == GLSG_KEY_RIGHT && m_CaretIndex < static_cast<int>(m_Text.size())) {
            m_CaretIndex++;
        } else if (data.key == GLSG_KEY_HOME) {
            m_CaretIndex = 0;
        } else if (data.key == GLSG_KEY_END) {
            m_CaretIndex = static_cast<int>(m_Text.size());
        }
    } else {
        if (data.key == GLSG_KEY_BACKSPACE || data.key == GLSG_KEY_DELETE) {
            eraseContent(m_charSelection[0], m_charSelection[1]);
            m_CaretIndex = m_charSelection[0];
            clearSelRange();
        }

        // marco.g: got to do it this way (clearSelRange() on each) since the
        // OnChar does pass through this callback first
        //          if leaving it will then clear the sel range and onChar won't
        //          be able to use it

        else if (data.key == GLSG_KEY_LEFT && m_CaretIndex > 0) {
            clearSelRange();
            m_CaretIndex--;
        } else if (data.key == GLSG_KEY_RIGHT && m_CaretIndex < static_cast<int>(m_Text.size())) {
            clearSelRange();
            m_CaretIndex++;
        } else if (data.key == GLSG_KEY_HOME) {
            clearSelRange();
            m_CaretIndex = 0;
        } else if (data.key == GLSG_KEY_END) {
            clearSelRange();
            m_CaretIndex = static_cast<int>(m_Text.size());
        }
    }

    drawCaret();

    if (m_setTextCb) {
        m_setTextCb(m_Text);
    }
}

void UIEdit::onChar(hidData& data) {
    if (m_blockEdit) {
        return;
    }
    m_CaretIndex = insertChar(static_cast<int>(data.codepoint), m_CaretIndex, true);
    setDrawFlag();
}

void UIEdit::onLostFocus() {
    // in case a value was changed but neither tab nor enter pressed and the
    // focus was lost be sure the actual value gets treated as entered
    for (auto &[fst, snd] : m_onEnterCb) {
        snd(m_Text);
    }
    UINode::onLostFocus();
}

void UIEdit::mouseDrag(hidData& data) {
    if (m_blockEdit) {
        return;
    }
    m_mousePosCr = data.mousePosNodeRel / getParentContentScale() - m_alignOffset;

    if (m_mouseEvent & 1) {
        l_cpos = getCaretByPixPos(m_mousePosCr.x, m_mousePosCr.y);

        m_CaretIndex    = l_cpos;
        m_CaretRange[1] = l_cpos;

        if (!m_drawImmediate) {
            reqUpdtTree();
        }
        setDrawFlag();
    }
    data.consumed = true;
}

void UIEdit::mouseDown(hidData& data) {
    if (m_blockEdit) {
        return;
    }

    vec2 p = data.mousePosNodeRel / getParentContentScale();
    int cpos = getCaretByPixPos(p.x - m_alignOffset.x, p.y - m_alignOffset.y);

    if (cpos >= 0) {
        m_CaretIndex    = cpos;
        m_CaretRange[0] = m_CaretRange[1] = m_CaretIndex;
        m_mouseEvent                      = 1;

        if (!m_drawImmediate) {
            reqUpdtTree();
        }
    }

    setSelected(true, true);
    drawCaret();

    if (data.isDoubleClick) {
        setSelRangeAll();
    }
    data.consumed = true;
}

void UIEdit::mouseUp(hidData& data) {
    if (m_blockEdit) {
        return;
    }
    m_mouseEvent = 0;
    data.consumed = true;
}

void UIEdit::mouseWheel(hidData& data) {
    if (m_blockEdit) {
        return;
    }

    if (m_useWheel) {
        incValue(data.degrees, data.shiftPressed  ? cfState::coarse
                                       : data.ctrlPressed ? cfState::fine
                                                           : cfState::normal);
    }

    setDrawFlag();
    data.consumed = m_useWheel;
}

void UIEdit::incValue(float amt, cfState cf) {
    float mAmt = amt * (cf == cfState::coarse ? 10.f : cf == cfState::normal ? 1.f : 0.1f);

    if (hasOpt(num_int)) {
        setValue(std::min(std::max(m_iValue + static_cast<int>(static_cast<float>(m_stepI) * mAmt), m_minInt), m_maxInt));

        for (auto &[fst, snd] : m_onEnterCb) {
            snd(m_Text);
        }

    } else if (hasOpt(num_fp)) {
        setValue(std::min(std::max(m_fValue + m_stepF * mAmt, m_minF), m_maxF));

        for (auto &[fst, snd] : m_onEnterCb) {
            snd(m_Text);
        }
    }
}

void UIEdit::globalMouseDown(hidData& data) {
    if (m_blockEdit) {
        return;
    }

    // close the menu if it is open and the user clicked somewhere outside the menu
    if (isInited() && m_state == state::selected &&
        !(static_cast<uint32_t>(data.objId) >= getId() && static_cast<uint32_t>(data.objId) <= getMaxChildId())) {
        setSelected(false, true);
        drawCaret();
        m_sharedRes->setDrawFlag();
    }
}

void UIEdit::setText(const std::string &str) {
    bool updt = str.size() != m_Text.size();
    m_Text.assign(str, 0, std::min(m_MaxCount, static_cast<int>(str.size())));
    checkLimits();
    m_CaretIndex = static_cast<int>(m_Text.size());
    clearSelRange();
    reqUpdtGlyphs(updt);
}

void UIEdit::setValue(float val) {
    m_fValue = val;
    if (hasOpt(num_fp)) {
        m_fValue = std::max<float>(std::min<float>(val, m_maxF), m_minF);

        std::stringstream stream;
        stream << std::fixed << std::setprecision(m_precision) << m_fValue;
        bool updt = m_Text.size() != stream.str().size();
        m_Text    = stream.str();
        reqUpdtGlyphs(updt);
    }
}

void UIEdit::setValue(double val) {
    m_dValue = val;
    if (hasOpt(num_fp)) {
        m_dValue = std::max<double>(std::min<double>(val, m_maxD), m_minD);

        std::stringstream stream;
        stream << std::fixed << std::setprecision(m_precision) << m_dValue;
        bool updt = m_Text.size() != stream.str().size();
        m_Text    = stream.str();
        reqUpdtGlyphs(updt);
    }
}

void UIEdit::setValue(int val) {
    m_iValue = val;
    if (hasOpt(num_int)) {
        m_iValue            = std::max<int>(std::min<int>(val, m_maxInt), m_minInt);
        std::string newText = std::to_string(m_iValue);
        bool        updt    = newText.size() != m_Text.size();
        m_Text              = newText;
        reqUpdtGlyphs(updt);
    }
}

void UIEdit::checkLimits() {
    if (m_Text.empty()) return;

    try {
        if (hasOpt(num_fp)) {
            m_fValue = std::stof(m_Text);

            if (m_precision == -1) {
                std::stringstream stream;
                stream << std::fixed << std::setprecision(m_precision) << m_fValue;
                m_Text = stream.str();
            }
        } else if (hasOpt(num_int)) {
            m_iValue = std::stoi(m_Text);
        }
    } catch (...) {
    }
}

void UIEdit::clampValue() {
    if (m_Text.empty()) {
        return;
    }
    if (hasOpt(num_int)) {
        setValue(std::stoi(m_Text));
    }
    if (hasOpt(num_fp)) {
        setValue(std::stof(m_Text));
    }
}

bool UIEdit::validateInputToString(int ch) const {
    std::string str(m_Text);
    if (!str.empty()) {
        str.insert(std::max<size_t>(std::min<size_t>(m_CaretIndex, str.size()), 0), 1, static_cast<char>(ch));
    } else {
        str.insert(str.begin(), static_cast<char>(ch));
    }

    if (hasOpt(num_int)) {
        return isValidIntInput(str);
    }
    if (hasOpt(num_fp)) {
        return isValidFloatInput(str);
    }
    return true;
}

int UIEdit::getCaretByPixPos(float px, float py) {
    if (!m_riFont) {
        return 0;
    }
    int off_bound = 0;
    int idx = m_FontDGV.getCharIndexByPixPos(px, py - m_riFont->getPixAscent(), m_tPos[0] + m_Offset.x,
                                             m_tPos[1] + m_Offset.y, off_bound);
    m_CaretIndex = idx;
    return idx;
}

bool UIEdit::setSelRangeAll() {
    int len = static_cast<int>(m_Text.size());
    return setSelRange(0, len);
}

bool UIEdit::setSelRange(int lo_index, int hi_index) {
    int len = static_cast<int>(m_Text.size());

    lo_index = std::clamp(lo_index, 0, len);
    hi_index = std::clamp(hi_index, 0, len);

    if (lo_index > hi_index) {
        return false;
    }

    m_CaretRange[0] = lo_index;
    m_CaretRange[1] = hi_index;
    return true;
}

bool UIEdit::getSelRange(glm::ivec2 &range) {
    if (m_CaretRange[0] == m_CaretRange[1]) {
        memset(&range[0], 0, sizeof(int) * 2);
        return false;
    }

    range.x = std::min(m_CaretRange.x, m_CaretRange.y);
    range.y = std::max(m_CaretRange.x, m_CaretRange.y);

    if (range.x < 0) {
        range.x = 0;
    }
    if (range.y > static_cast<int>(m_Text.size())) {
        range.y = static_cast<int>(m_Text.size());
    }
    return true;
}

void UIEdit::clearSelRange() {
    m_CaretRange[0] = m_CaretRange[1] = 0;
    drawCaret();
}

bool UIEdit::eraseContent(int lo_index, int hi_index) {
    const int len = static_cast<int>(m_Text.size());

    if (len <= 0) {
        return false;
    }
    if (lo_index > hi_index) {
        return false;
    }

    lo_index = std::clamp(lo_index, 0, len);
    hi_index = std::clamp(hi_index, 0, len);

    m_Text.erase(lo_index, hi_index - lo_index);
    reqUpdtGlyphs(true);
    return true;
}

int UIEdit::validateCaretPos(int cpos) const {
    if (cpos < 0) {
        cpos = 0;
    }
    if (cpos > static_cast<int>(m_Text.size())) {
        cpos = static_cast<int>(m_Text.size());
    }
    return cpos;
}

int UIEdit::insertChar(int ch, int position, bool call_cb) {
    bool validNewValue = true;

    // error check caretIndex == position
    position = static_cast<int>(std::min<size_t>(std::max<size_t>(static_cast<size_t>(position), 0), m_Text.size()));

    if (position < 0 && position > static_cast<int>(m_Text.size()) && m_Text.size() < m_MaxCount) {
        return position;
    }
    std::string tempStr(1, static_cast<char>(ch));

    ivec2 cpi;
    if (getSelRange(cpi) &&
        !((hasOpt(num_int) && !isValidIntInput(tempStr)) || (hasOpt(num_fp) && !isValidFloatInput(tempStr)))) {
        eraseContent(cpi[0], cpi[1]);
        position = cpi[0];
        clearSelRange();
    }

    if (!validateInputToString(ch)) {
        return position;
    }

    // in case of an integer check if in valid range, if not skip the input.
    // Allow the minus sign (char == 45)
    if ((hasOpt(num_int) || hasOpt(num_fp)) && ch != 45) {
        std::string tempTxt(m_Text);
        tempTxt.insert(position, 1, static_cast<char>(ch));

        if (!tempTxt.empty()) {
            if (hasOpt(num_int)) {
                try {
                    int newVal = std::stoi(tempTxt);
                    if (newVal > m_maxInt && newVal < m_minInt) {
                        m_iValue = newVal;
                    } else {
                        validNewValue = false;
                    }
                } catch (std::exception &) {
                    return position;
                }
            } else if (hasOpt(num_fp)) {
                try {
                    float newVal = std::stof(tempTxt);
                    if (newVal > m_maxF && newVal < m_minF) {
                        m_fValue = newVal;
                    } else {
                        validNewValue = false;
                    }
                } catch (std::exception &) {
                    return position;
                }
            }
        } else {
            return position;
        }
    }

    m_Text.insert(position, 1, static_cast<char>(ch));

    if (call_cb && m_setTextCb && validNewValue) {
        m_setTextCb(m_Text);
    }

    reqUpdtGlyphs(true);
    return position + 1;
}

void UIEdit::updateStyleIt(ResNode *node, state st, const std::string& styleClass) {
    Label::updateStyleIt(node, st, styleClass);

    if (node->hasValue("edit-opt")) {
        ParVec p = node->splitNodeValue("edit-opt");

        unsigned opt = 0;
        for (std::string &par : p) {
            if (par == "int") opt |= num_int;
            if (par == "float") opt |= num_fp;
            if (par == "pass") opt |= pass;
            if (par == "select-all") opt |= selectall_on_focus;
            if (par == "single-line") opt |= single_line;
            if (par == "accept-tabs") opt |= accept_tabs;
            if (par == "manual-space") opt |= manual_space;
        }

        m_tOpt = opt;
    }

    if (node->hasValue("edit-align")) {
        ParVec p = node->splitNodeValue("edit-align");

        for (std::string &par : p) {
            if (par == "left") m_tAlign_X = align::left;
            if (par == "center") m_tAlign_X = align::center;
            if (par == "right") m_tAlign_X = align::right;
            if (par == "justify") m_tAlign_X = align::justify;
            if (par == "justify-ex") m_tAlign_X = align::justify_ex;
        }
    }

    if (node->hasValue("edit-valign")) {
        ParVec p   = node->splitNodeValue("edit-valign");
        l_auxAlign = valign::center;

        for (std::string &par : p) {
            if (par == "top") l_auxAlign = valign::top;
            if (par == "vcenter" || par == "center") l_auxAlign = valign::center;
            if (par == "bottom") l_auxAlign = valign::bottom;
        }

        m_tAlign_Y                                = l_auxAlign;
        m_setStyleFunc[st][styleInit::textValign] = [this]() { m_tAlign_Y = l_auxAlign; };
    }

    if (auto f = node->findNode<AssetFont>("font")) {
        int         size = f->value<int32_t>("size", 0);
        std::string font = f->getValue("font");

        m_setStyleFunc[st][styleInit::fontFontSize]   = [this, size]() { setFontSize(size); };
        m_setStyleFunc[st][styleInit::fontFontFamily] = [this, font]() { setFontType(font); };
    }

    if (auto cc = node->findNode<AssetColor>("caret-color")) {
        vec4 col                                  = cc->getColorvec4();
        m_setStyleFunc[st][styleInit::caretColor] = [this, col]() { setCaretColor(col.r, col.g, col.b, col.a); };
    }
}

void UIEdit::changeValType(unsigned long t) {
    if (t != num_int && t != num_fp) {
        return;
    }

    // remove any numeric type
    m_tOpt &= ~num_int;
    m_tOpt &= ~num_fp;

    m_tOpt |= t; // set type
}

void UIEdit::clearProp() {
    if (m_stringProp) {
        removeOnChanged<std::string>(m_stringProp);
        removeEnterCb(m_stringProp);
        setOnLostFocusCb(nullptr);
    }
}

void UIEdit::setProp(Property<std::string> *prop) {
    if (!prop) return;
    m_stringProp = prop;
    onChanged<std::string>(prop, [this](const std::any &val) { setText(std::any_cast<std::string>(val)); });
    addEnterCb([prop](const std::string &txt) { *prop = txt; }, prop);
    setOnLostFocusCb([this, prop] { *prop = m_Text; });
    setText((*prop)());
}

void UIEdit::setProp(Property<std::filesystem::path> *prop) {
    onChanged<std::filesystem::path>(
        prop, [this](const std::any &val) { setText(std::any_cast<std::filesystem::path>(val).string()); });
    addEnterCb([prop](std::filesystem::path txt) { *prop = std::filesystem::path(std::move(txt)); }, prop);
    setOnLostFocusCb([this, prop] { *prop = std::filesystem::path(m_Text); });
    setText((*prop)().string());
}

void UIEdit::setProp(Property<int> *prop) {
    setOpt(UIEdit::single_line | UIEdit::num_int);

    onChanged<int>(prop, [this](const std::any &val) { setText(std::to_string(std::any_cast<int>(val))); });
    addEnterCb([prop](const std::string &txt) { (*prop) = atoi(txt.c_str()); }, prop);
    setOnLostFocusCb([this, prop] { (*prop) = getIntValue(); });
    setMinMax(prop->getMin(), prop->getMax());
    setStep(prop->getStep());
    setText(std::to_string((*prop)()));
    setUseWheel(true);
}

void UIEdit::setProp(Property<float> *prop) {
    setOpt(UIEdit::single_line | UIEdit::num_fp);

    onChanged<float>(prop, [this](const std::any &val) { setValue(std::any_cast<float>(val)); });
    addEnterCb([prop](const std::string &txt) { *prop = static_cast<float>(atof(txt.c_str())); }, prop);
    setOnLostFocusCb([this, prop] { *prop = static_cast<float>(atof(m_Text.c_str())); });
    setMinMax(prop->getMin(), prop->getMax());
    setStep(prop->getStep());
    setValue((*prop)());
    setUseWheel(true);
}

void UIEdit::setProp(Property<glm::ivec2> *prop, int idx) {
    onChanged<glm::ivec2>(prop, [this, idx](std::any val) { setValue(std::any_cast<glm::ivec2>(val)[idx]); });
    addEnterCb(
        [prop, idx](const std::string &txt) {
            glm::ivec2 newVal = (*prop)();
            newVal[idx]       = atoi(txt.c_str());
            (*prop)           = newVal;
        },
        prop);
    setOnLostFocusCb([this, prop, idx] {
        glm::ivec2 newVal = (*prop)();
        newVal[idx]       = atoi(m_Text.c_str());
        (*prop)           = newVal;
    });

    setOpt(UIEdit::num_int);
    setMinMax(prop->getMin()[idx], prop->getMax()[idx]);
    setStep(prop->getStep()[idx]);
    setValue((*prop)()[idx]);
    setUseWheel(true);
}

void UIEdit::setProp(Property<glm::vec2> *prop, int idx) {
    onChanged<glm::vec2>(prop, [this, idx](std::any val) { setValue(std::any_cast<glm::vec2>(val)[idx]); });
    addEnterCb(
        [prop, idx](const std::string &txt) {
            glm::vec2 newVal = (*prop)();
            newVal[idx]      = static_cast<float>(atof(txt.c_str()));
            (*prop)          = newVal;
        },
        prop);
    setOnLostFocusCb([this, prop, idx] {
        glm::vec2 newVal = (*prop)();
        newVal[idx]      = static_cast<float>(atof(m_Text.c_str()));
        (*prop)          = newVal;
    });

    setMinMax(prop->getMin()[idx], prop->getMax()[idx]);
    setStep(prop->getStep()[idx]);
    setValue((*prop)()[idx]);
    setUseWheel(true);
}

void UIEdit::setProp(Property<glm::vec3> *prop, int idx) {
    onChanged<glm::vec3>(prop, [this, idx](std::any val) { setValue(std::any_cast<glm::vec3>(val)[idx]); });
    addEnterCb(
        [prop, idx](const std::string &txt) {
            glm::vec3 newVal = (*prop)();
            newVal[idx]      = static_cast<float>(atof(txt.c_str()));
            (*prop)          = newVal;
        },
        prop);
    setOnLostFocusCb([this, prop, idx] {
        glm::vec3 newVal = (*prop)();
        newVal[idx]      = static_cast<float>(atof(m_Text.c_str()));
        (*prop)          = newVal;
    });

    setMinMax(prop->getMin()[idx], prop->getMax()[idx]);
    setStep(prop->getStep()[idx]);
    setValue((*prop)()[idx]);
    setUseWheel(true);
}

void UIEdit::setProp(Property<glm::ivec3> *prop, int idx) {
    onChanged<glm::ivec3>(prop, [this, idx](std::any val) { setValue(std::any_cast<glm::ivec3>(val)[idx]); });
    addEnterCb(
        [prop, idx](const std::string &txt) {
            glm::ivec3 newVal = (*prop)();
            newVal[idx]       = atoi(txt.c_str());
            (*prop)           = newVal;
        },
        prop);
    setOnLostFocusCb([this, prop, idx] {
        glm::ivec3 newVal = (*prop)();
        newVal[idx]       = atoi(m_Text.c_str());
        (*prop)           = newVal;
    });

    setMinMax(prop->getMin()[idx], prop->getMax()[idx]);
    setStep(prop->getStep()[idx]);
    setValue((*prop)()[idx]);
    setUseWheel(true);
}

void UIEdit::setPropItem(Item *item) {
    if (item && item->isPropertyItem) {
        if (item->m_typeId == tpi::tp_string) {
            setProp( dynamic_cast<PropertyItemUi<std::string> *>(item)->getPtr());
        } else if (item->m_typeId == tpi::tp_int32) {
            setProp( dynamic_cast<PropertyItemUi<int32_t> *>(item)->getPtr());
        } else if (item->m_typeId == tpi::tp_float) {
            setProp( dynamic_cast<PropertyItemUi<float> *>(item)->getPtr());
        }
    }
}

}  // namespace ara
