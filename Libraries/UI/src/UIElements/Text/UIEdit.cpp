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
UIEdit::UIEdit(const unsigned opt, const int max_count) {
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

void UIEdit::drawCaret(const bool forceCaretVaoUpdt) {
    if (!m_caret) {
        return;
    }

    bool updtTree = false;

    if (m_state == state::selected) {
        drawCaretSelectedState(updtTree);
    } else {
        if (m_caret->isVisible()) {
            m_caret->setVisibility(false);
            updtTree = true;
        }
    }

    if (!m_drawImmediate) {
        drawCaretIndirect(updtTree, forceCaretVaoUpdt);
    }
}

void UIEdit::drawCaretIndirect(const bool updtTree, const bool forceCaretVaoUpdt) const {
    m_caret->updateDrawData();
    if (forceCaretVaoUpdt) {
        m_caret->pushVaoUpdtOffsets();
    }
    if (updtTree) {
        reqUpdtTree();
    }
}

void UIEdit::drawCaretSelectedState(bool& updtTree) {
    if (!m_caret->isVisible()) {
        m_caret->setVisibility(true);
        m_caret->setSize(static_cast<int>(m_caretWidth / getParentContentScale().x),
                        static_cast<int>(m_riFont->getPixAscentHwp()));
        m_caret->setBackgroundColor(m_caretColor);
        updtTree = true;
    }

    auto tCaretPos = m_fontDGV.getCaretPosAndSize(m_caretIndex).first + m_bo;
    tCaretPos.x -= m_borderWidth;
    tCaretPos.y -= m_borderWidth + m_riFont->getPixAscentHwp() + m_riFont->getPixDescentHwp();

    setCaretRespectAlignment(tCaretPos);

    vec2 posLimit{};
    for (int i = 0; i < 2; i++) {
        posLimit[i] = m_size[i] - (m_padding[i + 2] + static_cast<float>(m_borderWidth *2));
    }

    if (!all(glm::equal(ivec2(m_caret->getPos()), ivec2(tCaretPos)))) {
        m_caret->setPos(static_cast<int>(std::min(tCaretPos.x, posLimit.x)),
                        static_cast<int>(std::min(tCaretPos.y, posLimit.y)));
    }
}

void UIEdit::setCaretRespectAlignment(vec2& tCaretPos) {
    // if there is no text, set the caret corresponding to the text format
    if (m_text.empty() && m_tAlignX == align::right) {
        tCaretPos.x = getContentSize().x;
    }
    if (m_text.empty() && m_tAlignX == align::center) {
        tCaretPos.x = getContentSize().x * 0.5f;
    }
}

Font *UIEdit::updateDGV(bool *checkFontTexture) {
    m_riFont     = m_riFont = checkAndGetFont();
    if (!m_riFont) {
        return nullptr;
    }

    m_renderText = hasOpt(pass) ? string(m_text.size(), '*') : m_text;

    if (!hasOpt(manual_space)) {
        m_tSize = m_tContSize;
    }
    m_tSize -= m_borderWidth * 2;

    m_fontDGV.setPixRatio(getPixRatio());
    m_fontDGV.setTabPixSize(m_tabSize);
    // process input text, break up in lines
    m_needsOverflowHandling = m_fontDGV.process(m_riFont, m_tSize, m_tSep, m_tAlignX, m_renderText, !hasOpt(single_line));

    m_offset = getContentOffset();
    calculateOffset();

    return m_riFont;
}

void UIEdit::calculateOffset() {
    const vec4 mask{m_offset.x, m_offset.y, m_offset.x + m_tContSize.x, m_offset.y + m_tContSize.y};

    if (int lineIndex; (lineIndex = m_fontDGV.getLineIndexByCharIndex(m_caretIndex)) >= 0) {
        const auto cpos = m_fontDGV.getCaretPosAndSize(m_caretIndex).first;
        const float pa = m_riFont->getPixAscent();

        // resulting x-position
        const auto x1 = cpos.x + m_offset.x;
        const auto x2 = x1 + 2;
        const auto y1 = m_fontDGV.getFontLines(lineIndex).getYSelRange(0) + m_offset.y + pa;
        const auto y2 = m_fontDGV.getFontLines(lineIndex).getYSelRange(1) + m_offset.y + pa;

        // in case the beginning of the rendered text will be outside the mask, add an
        // offset to move it into the non-mask area
        if (x1 < mask.x) {
            m_offset.x = -(cpos[0] - mask.x);
        }
        if (x2 > mask.z) {
            m_offset.x = std::max(m_offset.x, -(2 + cpos[0] - mask.z));
        }
        if (y1 < mask.y) {
            m_offset.y = -(pa + m_fontDGV.getFontLines(lineIndex).getYSelRange(0) - mask.y);
        }
        if (y2 > mask.w) {
            m_offset.y = std::max(m_offset.y, -(pa + m_fontDGV.getFontLines(lineIndex).getYSelRange(1) - mask.w));
        }
    }
}

void UIEdit::keyDown(hidData& data) {
    TextBlock::keyDown(data);

    if (m_blockEdit) {
        return;
    }

    if (data.key == ARA_KEY_ENTER || data.key == ARA_KEY_KP_ENTER) {
        procEnterAndReturn();
        return;
    }

    // ctrl + z, shift+ctrl+z and ctrl+y will most likely be used for undo / redo
    if ((data.key == ARA_KEY_Z && data.ctrlPressed) ||
        (data.key == ARA_KEY_Z && data.ctrlPressed && data.shiftPressed) ||
        (data.key == ARA_KEY_Y && data.ctrlPressed)) {
        onLostFocus();
        return;
    }

    if (data.key == ARA_KEY_V && data.ctrlPressed) {
        pasteText();
    }

    if (data.key == ARA_KEY_TAB) {
        procTab();
    }

    // increment decrement for integers and floats
    if (hasOpt(num_int) || hasOpt(num_fp)) {
        if (data.key == ARA_KEY_UP) {
            incValue(1.f, data.shiftPressed ? cfState::coarse : data.ctrlPressed ? cfState::fine : cfState::normal);
            getSharedRes()->reqRedraw();
        } else if (data.key == ARA_KEY_DOWN) {
            incValue(-1.f, data.shiftPressed ? cfState::coarse : data.ctrlPressed ? cfState::fine : cfState::normal);
            getSharedRes()->reqRedraw();
        }
    }

    if (data.shiftPressed) {
        procShiftPlusArrowSelect(data);
        return;
    }

    bool updateValue = false;
    if (!getSelRange(m_charSelection)) {
        moveCaret(data);
    } else {
        keyModifySelection(data, updateValue);
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

void UIEdit::keyModifySelection(const hidData& data, bool& updateValue) {
    auto backspaceAndDeleteAction = [&] {
        eraseContent(m_charSelection[0], m_charSelection[1]);
        m_caretIndex = m_charSelection[0];
        clearSelRange();
        updateValue = true;
    };

    unordered_map<int, function<void()>> keyMap {
        { ARA_KEY_BACKSPACE, [&]  { backspaceAndDeleteAction(); }},
        { ARA_KEY_DELETE,    [&] { backspaceAndDeleteAction(); }},
        { ARA_KEY_LEFT,      [&] {
            if (m_caretIndex > 0) {
                clearSelRange();
                m_caretIndex--;
            }
        }},
        { ARA_KEY_RIGHT,     [&] {
            if (m_caretIndex < static_cast<int>(m_text.size())) {
                clearSelRange();
                m_caretIndex++;
            }
        }},
        { ARA_KEY_HOME,     [&] {
            clearSelRange();
            m_caretIndex = 0;
        }},
        { ARA_KEY_END,     [&] {
            clearSelRange();
            m_caretIndex = static_cast<int>(m_text.size());
        }}
    };

    if (keyMap.contains(data.key)) {
        keyMap[data.key]();
    }
}

void UIEdit::pasteText() {
#ifdef ARA_USE_CLIP
    string clipText;
    clip::get_text(clipText);

    if (clipText.empty()) {
        return;
    }

    if (ivec2 selRange; getSelRange(selRange)) {
        eraseContent(selRange[0], selRange[1]);
        m_caretIndex = selRange[0];
        clearSelRange();
    }

    for (const auto ch : clipText) {
        m_caretIndex = insertChar(static_cast<int>(ch), m_caretIndex, true);
    }

    reqUpdtGlyphs(true);
    drawCaret();
    setDrawFlag();
#endif
}

void UIEdit::moveCaret(const hidData& data) {
    unordered_map<int, function<void()>> keyMap {
        { ARA_KEY_BACKSPACE,    [&] { moveCaretBackspace(); }},
        { ARA_KEY_DELETE,       [&] { moveCaretDel(); }},
        { ARA_KEY_LEFT,         [&] { moveCaretLeft(); }},
        { ARA_KEY_RIGHT,        [&] { moveCaretRight(); }},
        { ARA_KEY_HOME,         [&] { moveCaretHome(); }},
        { ARA_KEY_END,          [&] { moveCaretEnd(); }},
        { ARA_KEY_UP,           [&] { moveCaretLine(false); }},
        { ARA_KEY_DOWN,         [&] { moveCaretLine(true); }}
    };

    if (keyMap.contains(data.key)) {
        keyMap[data.key]();
    }
}

void UIEdit::moveCaretBackspace() {
    if (m_caretIndex > 0 && !m_text.empty()) {
        eraseContent(m_caretIndex -1, m_caretIndex);
        --m_caretIndex;
    }
}

void UIEdit::moveCaretDel() {
    if (m_caretIndex < static_cast<int>(m_text.size())) {
        m_text.erase(m_caretIndex, 1);
        reqUpdtGlyphs(true);
    }
}

void UIEdit::moveCaretLeft() {
    if (m_caretIndex > 0) {
        --m_caretIndex;
        if (m_needsOverflowHandling && m_lineOverflowOffset.first >= m_caretIndex) {
            calcLeftLineOffset();
        }
    }
}

void UIEdit::moveCaretRight() {
    if (m_caretIndex < static_cast<int>(m_text.size())) {
        ++m_caretIndex;
        if (m_needsOverflowHandling) {
            calcRightLineOffset();
        }
    }
}

void UIEdit::moveCaretHome() {
    m_caretIndex = 0;
    if (hasOpt(single_line) && m_lineOverflowOffset.first >= 0) {
        calcLeftLineOffset();
    }
}

void UIEdit::moveCaretEnd() {
    m_caretIndex = static_cast<int>(m_text.size());
    if (const auto line = m_fontDGV.getFontLines()[0];
        hasOpt(single_line)
        && line.maxCharIdx != -1
        && line.maxCharIdx < static_cast<int>(m_text.size())) {
        calcRightLineOffset();
    }
}

void UIEdit::moveCaretLine(const bool down) {
    if (hasOpt(single_line)) {
        return;
    }

    const auto& lines = m_fontDGV.getFontLines();
    const auto lineIndex = m_fontDGV.getLineIndexByCharIndex(m_caretIndex);
    const auto targetLineIndex = lineIndex + (down ? 1 : -1);
    if (lineIndex < 0 || targetLineIndex < 0 || targetLineIndex >= static_cast<int>(lines.size())) {
        return;
    }

    const auto targetX = m_fontDGV.getCaretPosAndSize(m_caretIndex).first.x;
    const auto& targetLine = lines[targetLineIndex];
    if (!targetLine.ptr[0] || !targetLine.ptr[1]) {
        return;
    }

    int newCaretIndex = targetLine.ptr[0]->characterIdx;
    float minDist = std::numeric_limits<float>::max();

    for (auto glyph = targetLine.ptr[0]; glyph <= targetLine.ptr[1]; ++glyph) {
        const auto glyphX = glyph->pos.x / glyph->pixRatio;
        if (const auto dist = std::abs(glyphX - targetX); dist < minDist) {
            minDist = dist;
            newCaretIndex = glyph->characterIdx;
        }

        const auto glyphEndX = (glyph->pos.x + glyph->size.x) / glyph->pixRatio;
        if (const auto endDist = std::abs(glyphEndX - targetX); endDist < minDist) {
            minDist = endDist;
            newCaretIndex = glyph->characterIdx + 1;
        }
    }

    m_caretIndex = std::clamp(newCaretIndex, targetLine.characterIdx[0], targetLine.characterIdx[1]);
}

void UIEdit::calcLeftLineOffset() {
    m_lineOverflowOffset.first = m_caretIndex;
    m_lineOverflowOffset.second = 0;
    for (int i = 0; i < m_lineOverflowOffset.first; ++i) {
        m_lineOverflowOffset.second -= m_fontDGV.getCaretPosAndSize(i).second.x;
    }
    reqUpdtGlyphs(true);
}

void UIEdit::calcRightLineOffset() {
    m_offset.x = getContentOffset().x;

    if (const auto line = m_fontDGV.getFontLines()[0]; line.maxCharIdx < m_caretIndex) {
        m_lineOverflowOffset.first = m_caretIndex - line.maxCharIdx;
        m_lineOverflowOffset.second = 0;

        for (int i = line.maxCharIdx + 1;
             i < std::min(static_cast<int32_t>(m_fontDGV.getGlyphs().size()), m_caretIndex + 1);
             ++i) {
            m_lineOverflowOffset.second -= m_fontDGV.getCaretPosAndSize(i).second.x;
        }

        reqUpdtGlyphs(true);
    }
}

void UIEdit::procEnterAndReturn() {
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
}

void UIEdit::procTab() {
    if (hasOpt(accept_tabs)) {
        m_caretIndex = insertChar('\t', m_caretIndex, true);
    } else {
        setSelected(false, true);
        drawCaret();
        clampValue();  // in case of number double that they are within the valid range
        onLostFocus();
    }
}

void UIEdit::procShiftPlusArrowSelect(const hidData& data) {
    auto finishUpdate = [this] {
        drawCaret();
        if (!m_drawImmediate) {
            prepareSelBgVao();
            reqUpdtTree();
        }
    };

    auto ensureSelectionStart = [this] {
        if (!getSelRange(m_charSelection)) {
            m_caretRange[0] = m_caretIndex;
        }
    };

    const std::unordered_map<int, std::function<bool()>> handlers {
        { ARA_KEY_LEFT, [this, &ensureSelectionStart, &finishUpdate] {
            if (m_caretIndex <= 0) {
                return false;
            }
            ensureSelectionStart();
            --m_caretIndex;
            m_caretRange[1] = m_caretIndex;
            finishUpdate();
            return true;
        }},
        { ARA_KEY_RIGHT, [this, &ensureSelectionStart, &finishUpdate] {
            if (m_caretIndex >= static_cast<int>(m_text.size())) {
                return false;
            }
            ensureSelectionStart();
            ++m_caretIndex;
            m_caretRange[1] = m_caretIndex;
            finishUpdate();
            return true;
        }},
        { ARA_KEY_HOME, [this, &ensureSelectionStart, &finishUpdate] {
            ensureSelectionStart();
            m_caretIndex    = 0;
            m_caretRange[1] = m_caretIndex;
            finishUpdate();
            return true;
        }},
        { ARA_KEY_END, [this, &ensureSelectionStart, &finishUpdate] {
            ensureSelectionStart();
            m_caretIndex    = static_cast<int>(m_text.size());
            m_caretRange[1] = m_caretIndex;
            finishUpdate();
            return true;
        }},
    };

    if (const auto it = handlers.find(data.key); it != handlers.end()) {
        it->second();
    }
}

void UIEdit::onChar(hidData& data) {
    if (m_blockEdit) {
        return;
    }
    m_caretIndex = insertChar(static_cast<int>(data.codepoint), m_caretIndex, true);
    setDrawFlag();
}

void UIEdit::onLostFocus() {
    // in case a value was changed but neither tab nor enter pressed and the focus was lost, be sure the actual value
    // gets treated as entered
    for (auto &snd: m_onEnterCb | views::values) {
        snd(m_text);
    }
    updateValFromText(m_text);
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
        if (hasOpt(single_line) && m_needsOverflowHandling) {
            m_lineOverflowOffset = {};
            calcRightLineOffset();
        }
    }

    getSharedRes()->reqRedraw();
    data.consumed = m_useWheel;
}

void UIEdit::incValue(const float amt, const cfState cf) {
    const float mAmt = amt * (cf == cfState::coarse ? 10.f : cf == cfState::normal ? 1.f : 0.1f);

    if (hasOpt(num_int)) {
        incValue<int32_t>(mAmt);
    } else if (hasOpt(num_fp)) {
        incValue<float>(mAmt);
    }

    if (hasOpt(num_int) || hasOpt(num_fp)) {
        for (auto &cb: m_onEnterCb | views::values) {
            cb(m_text);
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
    const bool updt = str.size() != m_text.size();
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
                stream << std::fixed << std::setprecision(m_precision) << std::get<float>(m_val);
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

bool UIEdit::validateNumInputToString(const int ch) {
    auto str = TextBlock::validateInputToString(ch);

    if (hasOpt(num_int)) {
        return isValidIntInput(str);
    }
    if (hasOpt(num_fp)) {
        return isValidFloatInput(str);
    }
    return true;
}

int UIEdit::insertChar(const int ch, int position, const bool call_cb) {
    // error check caretIndex == position
    position = static_cast<int>(std::min<size_t>(std::max<size_t>(static_cast<size_t>(position), 0), m_text.size()));

    if (position < 0 && position > static_cast<int>(m_text.size()) && m_text.size() < m_maxCount) {
        return position;
    }
    auto tempStr = std::to_string(ch);

    if (ivec2 cpi; getSelRange(cpi) && !((hasOpt(num_int) && !isValidIntInput(tempStr))
                                            || (hasOpt(num_fp) && !isValidFloatInput(tempStr)))) {
        eraseContent(cpi[0], cpi[1]);
        position = cpi[0];
        clearSelRange();
    }

    if (!validateNumInputToString(ch)) {
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

    if (call_cb && m_setTextCb) {
        m_setTextCb(m_text);
    }

    reqUpdtGlyphs(true);
    return position + 1;
}

void UIEdit::updateValFromText(const std::string& txt, const bool updateText) {
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

void UIEdit::updateStyleIt(ResNode *node, const state st, const std::string& styleClass) {
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

    if (const auto cc = node->findNode<AssetColor>("caret-color")) {
        vec4 col                                  = cc->getColorVec4();
        m_setStyleFunc[st][styleInit::caretColor] = [this, col]() { setCaretColor(col.r, col.g, col.b, col.a); };
    }
}

void UIEdit::changeValType(const unsigned long t) {
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

void UIEdit::blockEdit(const bool val) {
    m_blockEdit      = val;
    m_glyphsPrepared = false;
}

void UIEdit::removeEnterCb(void* ptr) {
    if (const auto it = m_onEnterCb.find(ptr); it != m_onEnterCb.end()) {
        m_onEnterCb.erase(it);
    }
}

void UIEdit::setCaretColor(const vec4 c, const state st) {
    m_caretColor     = c;
    m_glyphsPrepared = false;
    setStyleInitVal("caret-color",
                    "rgba(" + std::to_string(c.r * 255.f) + "," + std::to_string(c.g * 255.f) + "," +
                    std::to_string(c.b * 255.f) + "," + std::to_string(c.a * 255.f) + ")",
                    st);
}

void UIEdit::setCaretColor(const float r, const float g, const float b, const float a, const state st) {
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
