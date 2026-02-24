#include <Asset/AssetColor.h>
#include <Asset/AssetFont.h>
#include <Asset/AssetManager.h>
#include <DataModel/PropertyItemUi.h>
#include "UIElements/Text/UIEdit.h"
#include "UIWindow.h"

#ifdef ARA_USE_CLIP
#include "clip.h"
#endif

using namespace glm;
using namespace std;

namespace ara {
UIEdit::UIEdit(unsigned opt, int max_count) {
    setTypeName<UIEdit>();
    setName(getTypeName<UIEdit>());
    setFocusAllowed(true);

    m_tOpt = 0;
    m_tOpt |= single_line;
    m_tOpt |= opt;

    m_maxCount = max_count;
    Label::setColor(1, 1, 1, 1);
}

UIEdit::~UIEdit() {
    if (m_sharedRes && m_sharedRes->win) {
        static_cast<UIWindow *>(m_sharedRes->win)->removeGlobalMouseDownLeftCb(this);
    }
}

void UIEdit::init() {
    m_canReceiveDrag = true;

    m_caret = &push<Div>();
    m_caret->setVisibility(false);
    m_caret->excludeFromPadding(true);
    m_caret->excludeFromObjMap(true);

    static_cast<UIWindow *>(m_sharedRes->win)->addGlobalMouseDownLeftCb(this, [this](hidData& data) {
        globalMouseDown(data);
    });

    m_offset = getContentOffset();
    initSelBgShader();
}

void UIEdit::loadStyleDefaults() {
    TextBlock::loadStyleDefaults();
    m_setStyleFunc[state::none][styleInit::caretColor]     = [this]() { setCaretColor(1.f, 1.f, 1.f, 1.f); };
}

bool UIEdit::draw(uint32_t& objId) {
    TextBlock::draw(objId);
    drawCaret();
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

        auto tCaretPos = m_fontDGV.getCaretPos(m_caretIndex);
        tCaretPos = floor(tCaretPos + m_offset + m_alignOffset);

        // if there is no text, set the caret corresponding to the text format
        if (m_text.empty() && (m_tAlignX == align::right || m_tAlignX == align::center)) {
            if (m_tAlignX == align::right) {
                tCaretPos.x = getContentSize().x;
            } else if (m_tAlignX == align::center) {
                tCaretPos.x = getContentSize().x * 0.5f;
            }
        }

        vec2 posLimit{};
        for (int i = 0; i < 2; i++) {
            posLimit[i] = m_size[i] - (m_padding[i + 2] + static_cast<float>(m_borderWidth));
        }

        if (!all(glm::equal(m_caret->getPos(), tCaretPos))) {
            m_caret->setPos(static_cast<int>(std::min(tCaretPos.x, posLimit.x)),
                            static_cast<int>(std::min(tCaretPos.y, posLimit.y)));
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

Font *UIEdit::updateDGV(bool *checkFontTexture) {
    if (!m_sharedRes || !m_sharedRes->res) {
        return nullptr;
    }

    const auto font = getSharedRes()->res->getGLFont(m_fontType, m_fontSize, getPixRatio());
    if (!font) {
        LOGE << "[ERROR] UIEdit::UpdateDGV() / Cannot get font for " << m_fontType << "   size=" << m_fontSize;
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
    m_fontDGV.setTabPixSize(m_tabSize);
    // process input text, break up in lines
    m_fontDGV.process(m_riFont, m_tSize, m_tSep, m_tAlignX, m_renderText, !hasOpt(single_line));

    // Calculate offset
    if ((lidx = m_fontDGV.getLineIndexByCharIndex(m_caretIndex)) >= 0) {
        auto cpos = m_fontDGV.getCaretPos(m_caretIndex);
        float pa = m_riFont->getPixAscent();

        // resulting x-position
        auto x1 = cpos.x + m_offset.x;
        auto x2 = x1 + 2;
        auto y1 = m_fontDGV.getFontLines(lidx).getYSelRange(0) + m_offset.y + pa;
        auto y2 = m_fontDGV.getFontLines(lidx).getYSelRange(1) + m_offset.y + pa;

        // the beginning of the rendered text will be outside the mask add an
        // offset to move it into the non-mask area
        if (x1 < mask.x) {
            m_offset.x = -(cpos[0] - mask.x);
        }
        if (x2 > mask.z) {
            m_offset.x = std::max(m_offset.x, -(2 + cpos[0] - mask.z));
        }
        if (y1 < mask.y) {
            m_offset.y = -(pa + m_fontDGV.getFontLines(lidx).getYSelRange(0) - mask.y);
        }
        if (y2 > mask.w) {
            m_offset.y = std::max(m_offset.y, -(pa + m_fontDGV.getFontLines(lidx).getYSelRange(1) - mask.w));
        }
    }

    if (m_text.empty()) {
        m_offset = getContentOffset();
    }
    return m_riFont;
}

void UIEdit::updateFontGeo() {
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
    memcpy(&m_modMvp[0][0], &m_mvp[0][0], sizeof(float) * 16);

    m_mask *= m_riFont->getPixRatio();
    m_bo *= m_riFont->getPixRatio();
    m_modMvp = m_modMvp * scale(vec3{1.f / getPixRatio(), 1.f / getPixRatio(), 1.f});
}

void UIEdit::keyDown(hidData& data) {
    TextBlock::keyDown(data);

    if (m_blockEdit) {
        return;
    }

    // enter or return
    if (data.key == ARA_KEY_ENTER || data.key == ARA_KEY_KP_ENTER) {
        if (hasOpt(single_line)) {
            checkLimits();
        } else {
            m_caretIndex = insertChar('\r', m_caretIndex, true);
        }

        clampValue();  // in case of number double that they are within the valid range
        setSelected(false, true);
        drawCaret();

        for (const auto& cb : m_onEnterCb | views::values) {
            cb(m_text);
        }

        onLostFocus();
        return;
    }

    // ctrl + z, shift+ctrl+z and ctrl+y will most likely be used for undo / redo
    if ((data.key == ARA_KEY_Z && data.ctrlPressed) ||
        (data.key == ARA_KEY_Z && data.ctrlPressed && data.shiftPressed) ||
        (data.key == ARA_KEY_Y && data.ctrlPressed)) {
        onLostFocus();
        return;
    }

    if (data.key == ARA_KEY_TAB) {
        if (hasOpt(accept_tabs)) {
            m_caretIndex = insertChar('\t', m_caretIndex, true);
        } else {
            setSelected(false, true);
            drawCaret();
            clampValue();  // in case of number double that they are within the valid range
            onLostFocus();
        }
    }

    // increment decrement for integers and floats
    if (hasOpt(num_int) || hasOpt(num_fp)) {
        if (data.key == ARA_KEY_UP) {
            incValue(1.f, data.shiftPressed ? cfState::coarse : data.ctrlPressed ? cfState::fine : cfState::normal);
        } else if (data.key == ARA_KEY_DOWN) {
            incValue(-1.f, data.shiftPressed ? cfState::coarse : data.ctrlPressed ? cfState::fine : cfState::normal);
        }
    }

    if (data.shiftPressed) {
        if (data.key == ARA_KEY_LEFT && m_caretIndex > 0) {
            if (!getSelRange(m_charSelection)) {
                m_caretRange[0] = m_caretIndex;
            }
            --m_caretIndex;
            m_caretRange[1] = m_caretIndex;
            drawCaret();
            if (!m_drawImmediate) {
                prepareSelBgVao();
                reqUpdtTree();
            }

            return;
        }

        if (data.key == ARA_KEY_RIGHT && m_caretIndex < static_cast<int>(m_text.size())) {
            if (!getSelRange(m_charSelection)) {
                m_caretRange[0] = m_caretIndex;
            }
            ++m_caretIndex;
            m_caretRange[1] = m_caretIndex;
            drawCaret();
            if (!m_drawImmediate) {
                prepareSelBgVao();
                reqUpdtTree();
            }
            return;
        }

        if (data.key == ARA_KEY_HOME) {
            if (!getSelRange(m_charSelection)) {
                m_caretRange[0] = m_caretIndex;
            }
            m_caretIndex    = 0;
            m_caretRange[1] = m_caretIndex;
            drawCaret();
            if (!m_drawImmediate) {
                prepareSelBgVao();
                reqUpdtTree();
            }
            return;
        }

        if (data.key == ARA_KEY_END) {
            if (!getSelRange(m_charSelection)) {
                m_caretRange[0] = m_caretIndex;
            }
            m_caretIndex = m_caretIndex = static_cast<int>(m_text.size());
            m_caretRange[1]             = m_caretIndex;
            drawCaret();
            if (!m_drawImmediate) {
                prepareSelBgVao();
                reqUpdtTree();
            }
            return;
        }

        return;
    }

    bool updateValue = false;
    if (!getSelRange(m_charSelection)) {
        if (data.key == ARA_KEY_BACKSPACE && m_caretIndex > 0 && !m_text.empty()) {
            eraseContent(m_caretIndex -1, m_caretIndex);
            --m_caretIndex;
            updateValue = true;
        } else if (data.key == ARA_KEY_DELETE && (m_caretIndex < static_cast<int>(m_text.size()))) {
            m_text.erase(m_caretIndex, 1);
            reqUpdtGlyphs(true);
            updateValue = true;
        } else if (data.key == ARA_KEY_LEFT && m_caretIndex > 0) {
            --m_caretIndex;
        } else if (data.key == ARA_KEY_RIGHT && m_caretIndex < static_cast<int>(m_text.size())) {
            ++m_caretIndex;
        } else if (data.key == ARA_KEY_HOME) {
            m_caretIndex = 0;
        } else if (data.key == ARA_KEY_END) {
            m_caretIndex = static_cast<int>(m_text.size());
        }
    } else {
        if (data.key == ARA_KEY_BACKSPACE || data.key == ARA_KEY_DELETE) {
            eraseContent(m_charSelection[0], m_charSelection[1]);
            m_caretIndex = m_charSelection[0];
            clearSelRange();
            updateValue = true;
        }

        // marco.g: got to do it this way (clearSelRange() on each) since the
        // OnChar does pass through this callback first
        //          if leaving it will then clear the sel range and onChar won't be able to use it

        else if (data.key == ARA_KEY_LEFT && m_caretIndex > 0) {
            clearSelRange();
            m_caretIndex--;
        } else if (data.key == ARA_KEY_RIGHT && m_caretIndex < static_cast<int>(m_text.size())) {
            clearSelRange();
            m_caretIndex++;
        } else if (data.key == ARA_KEY_HOME) {
            clearSelRange();
            m_caretIndex = 0;
        } else if (data.key == ARA_KEY_END) {
            clearSelRange();
            m_caretIndex = static_cast<int>(m_text.size());
        }
    }

    if (updateValue) {
        try {
            updateValFromText(m_text);
        } catch (std::runtime_error &e) {
            LOGE << "UIEdit::insertChar Error: " << e.what();
        }
    }

    drawCaret();

    if (m_setTextCb) {
        m_setTextCb(m_text);
    }

    setDrawFlag();
}

void UIEdit::onChar(hidData& data) {
    if (m_blockEdit) {
        return;
    }
    m_caretIndex = insertChar(static_cast<int>(data.codepoint), m_caretIndex, true);
    setDrawFlag();
}

void UIEdit::onLostFocus() {
    // in case a value was changed but neither tab nor enter pressed and the
    // focus was lost be sure the actual value gets treated as entered
    for (auto &snd: m_onEnterCb | views::values) {
        snd(m_text);
    }
    UINode::onLostFocus();
}

void UIEdit::mouseDrag(hidData& data) {
    if (m_blockEdit) {
        return;
    }
    TextBlock::mouseDrag(data);
}

void UIEdit::mouseDown(hidData& data) {
    if (m_blockEdit) {
        return;
    }
    TextBlock::mouseDown(data);
    drawCaret();
}

void UIEdit::mouseUp(hidData& data) {
    if (m_blockEdit) {
        return;
    }
    TextBlock::mouseUp(data);
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
        incValue<int32_t>(mAmt);
        for (auto &snd: m_onEnterCb | views::values) {
            snd(m_text);
        }

    } else if (hasOpt(num_fp)) {
        incValue<float>(mAmt);
        for (auto &snd: m_onEnterCb | views::values) {
            snd(m_text);
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

void UIEdit::setTextDist(const std::string &str) {
    setText(str);
}

void UIEdit::setTextDist(const std::filesystem::path& p) {
    setText(p.string());
}

void UIEdit::setText(const std::string &str) {
    bool updt = str.size() != m_text.size();
    m_text.assign(str, 0, std::min(m_maxCount, static_cast<int>(str.size())));
    checkLimits();
    m_caretIndex = static_cast<int>(m_text.size());
    clearSelRange();
    reqUpdtGlyphs(updt);
}

void UIEdit::checkLimits() {
    if (m_text.empty()) return;

    try {
        if (hasOpt(num_fp)) {
            setValue<float>(std::stof(m_text));
            if (m_precision == -1) {
                std::stringstream stream;
                stream << std::fixed << std::setprecision(m_precision) << std::get<float>(m_value);
                m_text = stream.str();
            }
        } else if (hasOpt(num_int)) {
            setValue<int32_t>(std::stoi(m_text));
        }
    } catch (...) {
    }
}

void UIEdit::clampValue() {
    if (m_text.empty()) {
        return;
    }
    if (hasOpt(num_int)) {
        setValue<int32_t>(std::stoi(m_text));
    }
    if (hasOpt(num_fp)) {
        setValue<float>(std::stof(m_text));
    }
}

bool UIEdit::validateInputToString(int ch) {
    auto str = TextBlock::validateInputToString(ch);

    if (hasOpt(num_int)) {
        return isValidIntInput(str);
    }
    if (hasOpt(num_fp)) {
        return isValidFloatInput(str);
    }
    return true;
}

int UIEdit::insertChar(int ch, int position, bool call_cb) {
    bool validNewValue = true;

    // error check caretIndex == position
    position = static_cast<int>(std::min<size_t>(std::max<size_t>(static_cast<size_t>(position), 0), m_text.size()));

    if (position < 0 && position > static_cast<int>(m_text.size()) && m_text.size() < m_maxCount) {
        return position;
    }
    auto tempStr = std::to_string(ch);

    ivec2 cpi;
    if (getSelRange(cpi)
        && !((hasOpt(num_int) && !isValidIntInput(tempStr))
            || (hasOpt(num_fp) && !isValidFloatInput(tempStr)))
    ) {
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
        std::string tempTxt(m_text);
        tempTxt.insert(position, 1, static_cast<char>(ch));
        if (tempTxt.empty()) {
            return position;
        }

        try {
            updateValFromText(tempTxt, false);
        } catch (std::runtime_error &e) {
            LOGE << "UIEdit::insertChar Error: " << e.what();
            return position;
        }
    }

    m_text.insert(position, 1, static_cast<char>(ch));

    if (call_cb && m_setTextCb && validNewValue) {
        m_setTextCb(m_text);
    }

    reqUpdtGlyphs(true);
    return position + 1;
}

void UIEdit::updateValFromText(std::string& txt, bool updateText) {
    try {
        if (hasOpt(num_int)) {
            setValue<int32_t>(txt.empty() ? 0 : std::stoi(txt), updateText);
        } else if (hasOpt(num_fp)) {
            setValue<float>(txt.empty() ? 0.f : std::stof(txt), updateText);
        }
    } catch (std::runtime_error &e) {
        LOGE << "UIEdit::updateValFromText Error: " << e.what();
    }
}

void UIEdit::updateStyleIt(ResNode *node, state st, const std::string& styleClass) {
    TextBlock::updateStyleIt(node, st, styleClass);

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

void UIEdit::setPropItem(Item *item) {
    if (item && item->isPropertyItem) {
        if (item->m_typeId == tpi::tp_string) {
            setProp<std::string>( *dynamic_cast<PropertyItemUi<std::string>*>(item)->getPtr());
        } else if (item->m_typeId == tpi::tp_int32) {
            setProp<int32_t>( *dynamic_cast<PropertyItemUi<int32_t> *>(item)->getPtr());
        } else if (item->m_typeId == tpi::tp_float) {
            setProp<float>( *dynamic_cast<PropertyItemUi<float> *>(item)->getPtr());
        }
    }
}

void UIEdit::blockEdit(bool val) {
    m_blockEdit      = val;
    m_glyphsPrepared = false;
}

void UIEdit::removeEnterCb(void* ptr) {
    if (auto it = m_onEnterCb.find(ptr); it != m_onEnterCb.end()) {
        m_onEnterCb.erase(it);
    }
}

void UIEdit::setCaretColor(vec4 c, state st) {
    m_caretColor     = c;
    m_glyphsPrepared = false;
    setStyleInitVal("caret-color",
                    "rgba(" + std::to_string(c.r * 255.f) + "," + std::to_string(c.g * 255.f) + "," +
                    std::to_string(c.b * 255.f) + "," + std::to_string(c.a * 255.f) + ")",
                    st);
}

void UIEdit::setCaretColor(float r, float g, float b, float a, state st) {
    m_caretColor.r   = r;
    m_caretColor.g   = g;
    m_caretColor.b   = b;
    m_caretColor.a   = a;
    m_glyphsPrepared = false;
    setStyleInitVal("caret-color",
                    "rgba(" + std::to_string(r * 255.f) + "," + std::to_string(g * 255.f) + "," +
                    std::to_string(b * 255.f) + "," + std::to_string(a * 255.f) + ")",
                    st);
}

}  // namespace ara
