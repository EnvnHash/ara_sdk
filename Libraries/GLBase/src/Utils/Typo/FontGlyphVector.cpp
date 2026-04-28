//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "Utils/Typo/FontGlyphVector.h"
#include "Utils/Typo/Font.h"
#include "Utils/Typo/UtfIterator.h"

using namespace glm;
using namespace std;

namespace ara {

unsigned FontGlyphVector::calculateBoundingBoxHwp(vec4 &bb) const {
    memset(&bb[0], 0, 16);

    unsigned i = 0;
    for (const auto &[ptr, y, width, characterIdx, yrange, yselrange, pixRatio, maxCharIdx] : m_vline) {
        if (!i) {
            bb.x = ptr[0]->pos.x;
            bb.z = ptr[0]->pos.x + width;
            bb.y = yrange[0];
            bb.w = yrange[1];
        } else {
            bb.x = std::min(bb.x, ptr[0]->pos.x);
            bb.z = std::max(bb.z, ptr[0]->pos.x + width);
            bb.y = std::min(bb.y, yrange[0]);
            bb.w = std::max(bb.w, yrange[1]);
        }
        ++i;
    }
    return i;
}

unsigned FontGlyphVector::calculateBoundingBox(vec4 &bb) const {
    const auto res = calculateBoundingBoxHwp(bb);
    for (int i = 0; i < 4; i++) {
        bb[i] /= m_pixRatio;
    }
    return res;
}

vec4 FontGlyphVector::calculateBoundingBoxHwp() const {
    vec4 bb;
    calculateBoundingBoxHwp(bb);
    return bb;
}

vec2 FontGlyphVector::getPixSize() const {
    const auto bb = calculateBoundingBoxHwp();
    return {(bb.z - bb.x) / m_pixRatio, (bb.w - bb.y) / m_pixRatio};
}

vec2 FontGlyphVector::getPixSizeHwp() const {
    const auto bb = calculateBoundingBoxHwp();
    return {(bb.z - bb.x), (bb.w - bb.y)};
}

void FontGlyphVector::reset(const Font *font) {
    m_glyphs.clear();
    m_vline.clear();

    if (font != nullptr) {
        m_pixLineHeight  = font->getPixHeightHwp();
        m_pixVMetrics[0] = font->getPixAscentHwp();
        m_pixVMetrics[1] = font->getPixDescentHwp();
        m_pixVMetrics[2] = font->getPixLineGapHwp();
    }
}

Fontdglyph FontGlyphVector::findByCharIndex(const int idx) const {
    const auto r = std::ranges::find_if(m_glyphs, [idx](auto& glyph) { return glyph.characterIdx == idx; });
    return r != m_glyphs.end() ? *r : Fontdglyph();
}

int FontGlyphVector::getLineIndexByPixPos(const float pix_x, float pix_y, float off_x, const float off_y) {
    int i = 0;
    pix_y -= off_y;
    const vec2 hwPix{ pix_x * m_pixRatio, pix_y * m_pixRatio };

    // convert to hw pixels
    if (hwPix.x < m_bb[1]) {
        return -1;
    }

    if (hwPix.y > m_bb[3]) {
        return -2;
    }

    for (auto &[ptr, y, width, characterIdx, yrange, yselrange, pixRatio, maxCharIdx] : m_vline) {
        if (hwPix.y >= yselrange[0] && hwPix.y < yselrange[1]) {
            return i;
        }
        i++;
    }

    return -1;
}

int FontGlyphVector::getLineIndexByCharIndex(const int ch_index) const {
    int i = 0;
    for (const auto &[ptr, y, width, characterIdx, yrange, yselrange, pixRatio, maxCharIdx] : m_vline) {
        if (ch_index >= characterIdx[0] && ch_index <= characterIdx[1]) {
            return i;
        }
        ++i;
    }

    return -1;
}

int FontGlyphVector::getCharIndexByPixPos(float pix_x, const float pix_y, const float off_x, const float off_y, int &off_bound) {
    const auto lineIndex = getLineIndexByPixPos(pix_x, pix_y, off_x, off_y);
    off_bound = 0;

    if (lineIndex < 0) {
        if (lineIndex == -1) {
            return 0;
        }

        if (lineIndex == -2) {
            return static_cast<int>(m_glyphs.size());
        }

        return 0;
    }

    pix_x -= off_x;
    const float hwPix_x = pix_x * m_pixRatio;
    auto e = m_vline[lineIndex].ptr[0];

    if (e == nullptr || !m_vline[lineIndex].ptr[1]) {
        return -1;
    }

    if (hwPix_x <= m_bb[0]) {
        return m_vline[lineIndex].ptr[0]->characterIdx;
    }

    if (hwPix_x >= m_bb[2]) {
        return m_vline[lineIndex].ptr[1]->characterIdx + (m_vline.size() > 1 ? 0 : 1);
    }

    if (hwPix_x < e[0].pos.x) {
        return e[0].characterIdx;
    }

    e = m_vline[lineIndex].ptr[0];

    while (e < m_vline[lineIndex].ptr[1]) {
        if (hwPix_x >= e[0].pos.x && hwPix_x < e[1].pos.x) {
            return e[0].characterIdx;
        }
        ++e;
    }

    if (e == m_vline[lineIndex].ptr[1]) {
        if (hwPix_x >= e[0].pos.x && hwPix_x <= e[0].pos.x + e[0].size.x) {
            return e[0].characterIdx;
        }
    }

    return m_vline[lineIndex].ptr[1]->characterIdx;
}

pair<vec2, vec2> FontGlyphVector::getCaretPosAndSize(const int caretIndex) const {
    pair<vec2, vec2> outPair {};
    const auto numChars = static_cast<int>(size());
    if (!numChars) {
        return outPair;
    }

    auto [charAsCodepoint, pos, size, glyphPtr, color, characterIdx, pixRatio] = findByCharIndex(caretIndex);
    outPair.second = size;
    if (glyphPtr) {
        outPair.second.x = glyphPtr->xadv;
    }

    if (caretIndex < numChars) {
        if (charAsCodepoint > 0) {
            outPair.first.x = pos.x;
            outPair.first.y = pos.y + size.y;
        }
    } else {
        outPair.first = m_glyphs[numChars - 1].pos + m_glyphs[numChars - 1].size;
    }

    if (caretIndex < 0) {
        outPair.first.y = m_vline[0].y;
    } else if (caretIndex > static_cast<int>(m_glyphs.size() -1)) {
        outPair.first.y = m_vline[m_vline.size() - 1].y;
    } else {
        if (const auto lineIndex = getLineIndexByCharIndex(caretIndex); lineIndex < 0) {
            outPair.first.y = m_vline[0].y;
        } else {
            outPair.first.y = m_vline[lineIndex].y;
        }
    }

    outPair.first /= m_pixRatio;  // convert to virtual pixels
    return outPair;
}

int FontGlyphVector::jumpToLine(const int caret_index, const int line_delta) {
    int ci   = caret_index;
    const int lineIndex = getLineIndexByCharIndex(ci) + line_delta;

    if (lineIndex < 0 || lineIndex >= static_cast<int>(m_vline.size())) {
        return ci;
    }

    m_tCaretPos = getCaretPosAndSize(ci).first;
    const auto ve = m_vline[lineIndex].ptr[0];

    if (ve == nullptr || !m_vline[lineIndex].ptr[1]) {
        return ci;
    }

    if (m_tCaretPos.x <= m_bb.x) {
        return m_vline[lineIndex].ptr[0]->characterIdx;
    }

    if (m_tCaretPos.x >= m_bb.z) {
        return m_vline[lineIndex].ptr[1]->characterIdx;
    }

    if (m_tCaretPos.x < ve[0].pos.x) {
        return ve[0].characterIdx;
    }

    ci = ve[0].characterIdx;
    float dist, mdist = 1e10;

    while (ve < m_vline[lineIndex].ptr[1]) {
        if ((dist = fabsf(ve[0].pos.x - m_tCaretPos.x)) < mdist) {
            ci    = ve[0].characterIdx;
            mdist = dist;
        }
    }

    return ci;
}

int FontGlyphVector::jumpToBeginOfLine(const int caret_index) const {
    const auto lineIndex = getLineIndexByCharIndex(caret_index);

    if (lineIndex < 0 || lineIndex >= static_cast<int>(m_vline.size())) {
        return caret_index;
    }

    if (m_vline[lineIndex].ptr[0] == nullptr) {
        return caret_index;
    }

    return m_vline[lineIndex].ptr[0]->characterIdx;
}

int FontGlyphVector::jumpToEndOfLine(const int caret_index) const {
    const auto lineIndex = getLineIndexByCharIndex(caret_index);
    if (lineIndex < 0 || lineIndex >= static_cast<int>(m_vline.size())) {
        return caret_index;
    }

    if (m_vline[lineIndex].ptr[1] == nullptr) {
        return caret_index;
    }

    return m_vline[lineIndex].ptr[1]->characterIdx;
}

bool FontGlyphVector::process(Font *font, const vec2 &size, const vec2 &sep, const align textAlignX,
                              const std::string &str, const bool wordWrap) {
    if (!font || size.x == 0.f || size.y == 0.f) {
        return {};
    }

    reset(font);  // gets font metrics
    if (!font->isOK()) {
        return {};
    }

    auto par = procPar{
        .text = str,
        .lineheight = m_pixLineHeight,
        .text_align_x = textAlignX,
        .l_sep = sep * m_pixRatio,
        .l_size = size * m_pixRatio,
        .textColIt = m_textColors.begin()
    };

    par.sep  = par.l_sep;

    // resize the glyph vector to the size of the input string
    m_glyphs.resize(str.size() + 2);

    const auto vorig = par.ve = &m_glyphs[0];
    bool hasOverflowText = false;
    for (par.textIt = par.text.begin(); par.textIt != par.text.end(); ++par.textIt) {
        UtfIterator iter(par.textIt);
        par.charAsCodepoint = codepoint(*iter);

        if (par.charAsCodepoint == 9) {
            procTab(par);
        } else if (par.charAsCodepoint == 13 || par.charAsCodepoint == 10) {
            procCrAndNl(par);
        } else if (par.charAsCodepoint == 32) {
            procSpace(par, font);
        } else if (par.charAsCodepoint > 32 && par.charAsCodepoint < 255) {
            procChar(par, font, wordWrap);
        }

        if (par.pos.x > size.x && par.maxCharIdx == -1) {
            par.maxCharIdx = static_cast<int32_t>( std::distance(par.text.begin(), par.textIt) );
            hasOverflowText = true;
        }
        par.lch = par.charAsCodepoint;
    }

    if (par.linep[0]) {
        par.ve[0] = addDGlyph(par, {0.f, -par.spheight}, {0, par.spheight}, nullptr);
        par.linep[1] = par.ve;
        par.fword.emplace_back(Fontword{par.ws, par.ve + 1});
        addLine(par, false);
        par.fword.clear();
        par.maxCharIdx = -1;
    }

    m_glyphs.resize(par.ve - vorig);
    calculateBoundingBoxHwp(m_bb);

    return hasOverflowText;
}

void FontGlyphVector::glyphPtrCheck(procPar &par) {
    if (!par.ws) {
        par.ws = par.ve;
    }

    if (!par.linep[0]) {
        par.linep[0] = par.linep[1] = par.ve;
    }
}

Fontdglyph FontGlyphVector::addDGlyph(procPar &par, const vec2& offs, const vec2& size, Fontglyph* g) const {
    return {
        .charAsCodepoint = par.charAsCodepoint,
        .pos = par.pos + offs,
        .size = size,
        .glyphPtr = g,
        .color = getCharColor(par),
        .characterIdx = std::min(static_cast<int>(par.text.size()) -1, static_cast<int>(par.textIt - par.text.begin())),
        .pixRatio = m_pixRatio
    };
}

vec4* FontGlyphVector::getCharColor(procPar& par) const {
    if (m_textColors.empty()) {
        return nullptr;
    }

    if (const auto absCharIdx = par.textIt - par.text.begin(); par.textIt != par.text.end() && par.textColIt->first.y < absCharIdx) {
        ++par.textColIt;
    }
    return &par.textColIt->second;
}

void FontGlyphVector::procTab(procPar& par) const {
    float d = m_tabSize > 0 ? m_tabSize - fmodf(par.pos.x, m_tabSize) : 0;

    glyphPtrCheck(par);
    par.ve[0] = addDGlyph(par, {0, -par.spheight}, {d, par.spheight});

    ++par.ve;
    par.pos.x += d;
}

void FontGlyphVector::procCrAndNl(procPar& par) {
    glyphPtrCheck(par);
    par.ve[0] = addDGlyph(par, {0, -par.spheight}, {0.f, par.spheight});

    par.linep[1] = par.ve;
    ++par.ve;

    if (par.ws) {
        par.fword.emplace_back(Fontword{par.ws, par.ve});
    }

    addLine(par, true);

    par.fword.clear();

    par.linep[0] = par.linep[1] = nullptr;
    par.ws                  = nullptr;

    par.pos.x = 0;
    par.pos.y += par.lineheight + par.l_sep.x;
}

void FontGlyphVector::procSpace(procPar& par, Font* font) const {
    glyphPtrCheck(par);

    if (const auto g = font->getGlyph(par.charAsCodepoint)) {
        par.ve[0] = addDGlyph(par, {g->off[0].x, -par.spheight}, {g->xadv,  par.spheight}, g);
        ++par.ve;
        par.pos.x += g->xadv + par.l_sep.x;
    }
}

void FontGlyphVector::procChar(procPar& par, Font* font, const bool word_wrap) {
    if (!par.linep[0]) {
        par.linep[0] = par.linep[1] = par.ve;
    }

    if (par.lch <= 32) {
        if (par.ws) {
            par.fword.emplace_back(Fontword{par.ws, par.ve});
        }
        par.ws = par.ve;
    }

    if (const auto g  = font->getGlyph(par.charAsCodepoint)) {
        par.ve[0]  = addDGlyph(par, g->off[0], g->outpixsize, g);
        if (par.pos.x + par.ve[0].size.x >= par.l_size.x && word_wrap) {
            if (!par.fword.empty()) {
                par.linep[1] = par.ws;

                addLine(par, false, -1);
                par.fword.clear();

                par.pos.x = 0;
                par.pos.y += par.lineheight + par.l_sep.y;

                if (par.ws) {
                    par.ve = par.ws;
                    par.textIt  = par.text.begin() + par.ws->characterIdx -1;
                } else {
                    ++par.ve;
                }

                par.linep[0] = par.linep[1] = nullptr;
            } else {
                par.linep[1] = par.ve;

                par.fword.emplace_back(Fontword{par.linep[0], par.linep[1]});
                addLine(par, false);
                par.fword.clear();

                par.pos.x = 0;
                par.pos.y += par.lineheight + par.l_sep.y;

                par.linep[0] = par.linep[1] = nullptr;
                par.ws                  = par.ve;
                ++par.ve;
            }
        } else {
            par.pos.x += g->xadv + par.l_sep.x;
            ++par.ve;
        }
    }
}

/**  y and width are in hw pixels */
Fontline *FontGlyphVector::addLine(const procPar& par, const bool eol, const int lineEndOffs) {
    const auto p_begin = par.linep[0];
    const auto p_end = par.linep[1] + lineEndOffs;
    const float y = par.pos.y;
    auto &inFword = par.fword;
    const align text_align_x = par.text_align_x;
    const float width = par.l_size.x;

    Fontline   *fline = nullptr;

    if (!p_begin || !p_end) {
        return nullptr;
    }

    if (p_begin == p_end) {
        m_vline.emplace_back(Fontline{p_begin, p_end, y, 0, p_begin->characterIdx, p_end->characterIdx, y - m_pixVMetrics[0],
                                      y - m_pixVMetrics[1], y - m_pixVMetrics[0],
                                      y - m_pixVMetrics[0] + m_pixLineHeight + par.sep.y, m_pixRatio, par.maxCharIdx});
        fline = &m_vline.back();
        return fline;
    }

    const float lw = p_end->pos.x - p_begin->pos.x;

    m_vline.emplace_back(Fontline{p_begin, p_end, y, lw, p_begin->characterIdx, p_end->characterIdx, y - m_pixVMetrics[0],
                                  y - m_pixVMetrics[1], y - m_pixVMetrics[0],
                                  y - m_pixVMetrics[0] + m_pixLineHeight + par.sep.y, m_pixRatio, par.maxCharIdx});
    fline = &m_vline.back();

    if (text_align_x != align::left) {
        Fontdglyph *dg;
        if (text_align_x == align::center)  { // CENTER
            const float xo = width * 0.5f - lw * 0.5f;
            for (const auto &[ptr, w, characterIdx] : inFword) {
                for (dg = ptr[0]; dg < ptr[1]; dg++) {
                    dg->pos.x += xo;
                }
            }
        } else if (text_align_x == align::right) {  // RIGHT
            const float xo = width - lw;
            for (const auto &[ptr, w, characterIdx] : inFword) {
                for (dg = ptr[0]; dg < ptr[1]; dg++) {
                    dg->pos.x += xo;
                }
            }
        } else if (text_align_x == align::justify ||
                   text_align_x == align::justify_ex) {  // JUSTIFIED (4 is whole, even EOL)
            bool doit = true;
            if (text_align_x == align::justify && eol) {
                doit = false;
            }

            if (doit) {
                constexpr float xo = 0;
                dg       = p_begin;

                while (dg->charAsCodepoint <= 32 && dg < p_end) {
                    dg->size.x = 0;
                    dg->pos.x  = xo;
                    ++dg;
                }

                if (dg < p_end && dg->charAsCodepoint > 32) {
                    Fontdglyph *rr[2]{};
                    rr[0] = dg;
                    dg    = p_end;

                    while (dg->charAsCodepoint <= 32 && dg > p_begin) {
                        dg->size.x = 0;
                        dg->pos.x  = width;
                        --dg;
                    }

                    if (dg > p_begin && dg->charAsCodepoint > 32) {
                        rr[1] = dg;

                        float tx = 0;
                        int   nc = 0;

                        for (dg = rr[0]; dg <= rr[1]; ++dg) {
                            if (dg->charAsCodepoint <= 32) {
                                ++nc;
                            }
                            if (dg->charAsCodepoint > 32) {
                                tx += dg->size.x;
                            }
                        }

                        if (nc > 0) {
                            const auto dx = (width - tx) / static_cast<float>(nc);
                            auto x  = xo;

                            for (dg = rr[0]; dg <= rr[1]; ++dg) {
                                dg->pos.x = x;
                                if (dg->charAsCodepoint <= 32) {
                                    dg->size.x = dx;
                                }
                                x += dg->size.x;
                            }
                        }
                    }
                }
            }
        }
    }

    return fline;
}

int FontGlyphVector::codepoint(const std::string& u) {
    const auto l = static_cast<int>(u.length());
    if (l < 1) {
        return -1;
    }

    const unsigned char u0 = u[0];
    if (u0 >= 0 && u0 <= 127) {
        return u0;
    }

    if (l < 2) {
        return -1;
    }

    const unsigned char u1 = u[1];
    if (u0 >= 192 && u0 <= 223) {
        return (u0 - 192) * 64 + (u1 - 128);
    }

    if (static_cast<unsigned char>(u[0]) == 0xed && (u[1] & 0xa0) == 0xa0) {
        return -1;
    }  // code points, 0xd800 to 0xdfff

    if (l < 3) {
        return -1;
    }

    const unsigned char u2 = u[2];
    if (u0 >= 224 && u0 <= 239) {
        return (u0 - 224) * 4096 + (u1 - 128) * 64 + (u2 - 128);
    }

    if (l < 4) {
        return -1;
    }

    const unsigned char u3 = u[3];
    if (u0 >= 240 && u0 <= 247) {
        return (u0 - 240) * 262144 + (u1 - 128) * 4096 + (u2 - 128) * 64 + (u3 - 128);
    }
    return -1;
}

}