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

namespace ara {

unsigned FontGlyphVector::calculateBoundingBoxHwp(vec4 &bb) {
    memset(&bb[0], 0, 16);

    unsigned i = 0;
    for (e_fontline &fl : m_vline) {
        if (!i) {
            bb.x = fl.ptr[0]->pos.x;
            bb.z = fl.ptr[0]->pos.x + fl.width;
            bb.y = fl.yrange[0];
            bb.w = fl.yrange[1];
        } else {
            bb.x = std::min(bb.x, fl.ptr[0]->pos.x);
            bb.z = std::max(bb.z, fl.ptr[0]->pos.x + fl.width);
            bb.y = std::min(bb.y, fl.yrange[0]);
            bb.w = std::max(bb.w, fl.yrange[1]);
        }
        ++i;
    }
    return i;
}

unsigned FontGlyphVector::calculateBoundingBox(glm::vec4 &bb) {
    unsigned res = calculateBoundingBoxHwp(bb);
    for (int i = 0; i < 4; i++) {
        bb[i] /= m_pixRatio;
    }
    return res;
}

vec4 FontGlyphVector::calculateBoundingBoxHwp() {
    vec4 bb;
    calculateBoundingBoxHwp(bb);
    return bb;
}

vec2 FontGlyphVector::getPixSize() {
    vec4 bb = calculateBoundingBoxHwp();
    return {(bb.z - bb.x) / m_pixRatio, (bb.w - bb.y) / m_pixRatio};
}

vec2 FontGlyphVector::getPixSizeHwp() {
    vec4 bb = calculateBoundingBoxHwp();
    return {(bb.z - bb.x), (bb.w - bb.y)};
}

void FontGlyphVector::reset(const Font *font) {
    m_v.clear();
    m_vline.clear();

    if (font != nullptr) {
        m_pixLineHeight  = font->getPixHeightHwp();
        m_pixVMetrics[0] = font->getPixAscentHwp();
        m_pixVMetrics[1] = font->getPixDescentHwp();
        m_pixVMetrics[2] = font->getPixLineGapHwp();
    }
}

e_fontdglyph FontGlyphVector::findByCharIndex(int idx) {
    for (auto &gIt : m_v) {
        if (gIt.characterIdx == idx) {
            return gIt;
        }
    }

    return e_fontdglyph{};
}

int FontGlyphVector::getLineIndexByPixPos(float pix_x, float pix_y, float off_x, float off_y) {
    int i = 0;

    pix_y -= off_y;

    vec2 hwPix{ pix_x * m_pixRatio, pix_y * m_pixRatio };

    // convert to hw pixels
    if (hwPix.x < m_bb[1]) {
        return -1;
    }

    if (hwPix.y > m_bb[3]) {
        return -2;
    }

    for (auto &l : m_vline) {
        if (hwPix.y >= l.yselrange[0] && hwPix.y < l.yselrange[1]) {
            return i;
        }
        i++;
    }

    return -1;
}

int FontGlyphVector::getLineIndexByCharIndex(int ch_index) {
    int i = 0;
    for (e_fontline &l : m_vline) {
        if (ch_index >= l.chidx[0] && ch_index <= l.chidx[1]) {
            return i;
        }
        ++i;
    }

    return -1;
}

int FontGlyphVector::getCharIndexByPixPos(float pix_x, float pix_y, float off_x, float off_y, int &off_bound) {
    auto lidx = getLineIndexByPixPos(pix_x, pix_y, off_x, off_y);
    off_bound = 0;

    if (lidx < 0) {
        if (lidx == -1) {
            return 0;
        }

        if (lidx == -2) {
            return static_cast<int>(m_v.size());
        }

        return 0;
    }

    pix_x -= off_x;

    float hwPix_x = pix_x * m_pixRatio;

    auto e = m_vline[lidx].ptr[0];

    if (e == nullptr || !m_vline[lidx].ptr[1]) {
        return -1;
    }

    if (hwPix_x <= m_bb[0]) {
        return m_vline[lidx].ptr[0]->characterIdx;
    }

    if (hwPix_x >= m_bb[2]) {
        return m_vline[lidx].ptr[1]->characterIdx;
    }

    if (hwPix_x < e[0].pos.x) {
        return e[0].characterIdx;
    }

    e = m_vline[lidx].ptr[0];

    while (e < m_vline[lidx].ptr[1]) {
        if (hwPix_x >= e[0].pos.x && hwPix_x < e[1].pos.x) {
            return e[0].characterIdx;
        }
        ++e;
    }

    if (e == m_vline[lidx].ptr[1]) {
        if (hwPix_x >= e[0].pos.x && hwPix_x <= e[0].pos.x + e[0].size.x) {
            return e[0].characterIdx;
        }
    }

    return m_vline[lidx].ptr[1]->characterIdx;
}

vec2 FontGlyphVector::getCaretPos(int caret_index) {
    vec2 pos{};
    auto nchars = static_cast<int>(size());
    memset(&pos[0], 0, sizeof(float) * 2);

    if (!nchars) {
        return pos;
    }

    if (nchars > 0) {
        auto pr = findByCharIndex(caret_index);

        if (caret_index < nchars) {
            if (pr.codepoint > 0) {
                pos.x = pr.pos.x;
                pos.y = pr.pos.y + pr.size.y;
            }
        } else {
            pos = m_v[nchars - 1].pos + m_v[nchars - 1].size;
        }

        if (caret_index < 0) {
            pos.y = m_vline[0].y;
        } else if (caret_index >= static_cast<int>(m_v.size())) {
            pos.y = m_vline[m_vline.size() - 1].y;
        } else {
            auto lidx = getLineIndexByCharIndex(caret_index);
            if (lidx < 0) {
                pos.y = m_vline[0].y;
            } else {
                pos.y = m_vline[lidx].y;
            }
        }
    }

    pos /= m_pixRatio;  // convert to virtual pixels
    return pos;
}

int FontGlyphVector::jumpToLine(int caret_index, int line_delta) {
    int ci   = caret_index;
    int lidx = getLineIndexByCharIndex(ci) + line_delta;

    if (lidx < 0 || lidx >= static_cast<int>(m_vline.size())) {
        return ci;
    }

    m_tCaretPos = getCaretPos(ci);
    auto ve = m_vline[lidx].ptr[0];

    if (ve == nullptr || !m_vline[lidx].ptr[1]) {
        return ci;
    }

    if (m_tCaretPos.x <= m_bb.x) {
        return m_vline[lidx].ptr[0]->characterIdx;
    }

    if (m_tCaretPos.x >= m_bb.z) {
        return m_vline[lidx].ptr[1]->characterIdx;
    }

    if (m_tCaretPos.x < ve[0].pos.x) {
        return ve[0].characterIdx;
    }

    ci = ve[0].characterIdx;
    float dist, mdist = 1e10;

    while (ve < m_vline[lidx].ptr[1]) {
        if ((dist = fabsf(ve[0].pos.x - m_tCaretPos.x)) < mdist) {
            ci    = ve[0].characterIdx;
            mdist = dist;
        }
    }

    return ci;
}

int FontGlyphVector::jumpToBeginOfLine(int caret_index) {
    auto lidx = getLineIndexByCharIndex(caret_index);

    if (lidx < 0 || lidx >= static_cast<int>(m_vline.size())) {
        return caret_index;
    }

    if (m_vline[lidx].ptr[0] == nullptr) {
        return caret_index;
    }

    return m_vline[lidx].ptr[0]->characterIdx;
}

int FontGlyphVector::jumpToEndOfLine(int caret_index) {
    auto lidx = getLineIndexByCharIndex(caret_index);

    if (lidx < 0 || lidx >= static_cast<int>(m_vline.size())) {
        return caret_index;
    }

    if (m_vline[lidx].ptr[1] == nullptr) {
        return caret_index;
    }

    return m_vline[lidx].ptr[1]->characterIdx;
}

bool FontGlyphVector::Process(Font *font, const vec2 &size, const vec2 &sep, align text_align_x, const std::string &str, bool word_wrap) {
    if (!font || size.x == 0.f || size.y == 0.f || str.empty()) {
        return false;
    }

    reset(font);  // gets font metrics

    if (str.empty() || !font->isOK()) {
        return false;
    }

    for (int i = 0; i < 2; i++) {
        m_sc[i] = 1.f;
    }

    auto par = procPar{
       // .pos = vec2{0.f, 0.f},
        .ws = nullptr,
        .ve = nullptr,
        .linep{nullptr, nullptr},
        .ch = 0,
        .spheight = 0,
        .eorg = str,
        .lineheight = m_pixLineHeight,
        .text_align_x = text_align_x,
        .l_sep = sep * m_pixRatio,
        .l_size = size * m_pixRatio
    };

    par.e = par.eorg.begin();
    m_sep  = par.l_sep;

    // resize the glyph vector to the size of the input string
    m_v.resize(str.size() + 2);

    auto vorig = par.ve = &m_v[0];

    if (par.e == par.eorg.end()) {
        return false;
    }

    while (par.e != par.eorg.end()) {
        UtfIterator iter(par.e);
        par.ch = codepoint(*iter);

        if (par.ch == 9) {
            procTab(par);
        } else if (par.ch == 13 || par.ch == 10) {
            procCrAndNl(par);
        } else if (par.ch == 32) {
            procSpace(par, font);
        } else if (par.ch > 32 && par.ch < 255) {
            procChar(par, font, word_wrap);
        } else {
            ++par.e;
        }

        par.lch = par.ch;
    }

    if (par.linep[0]) {
        par.ve[0] = e_fontdglyph{
            .codepoint = 0,
            .pos = vec2{floor((par.pos.x) + 0.5f), floor((par.pos.y - par.spheight) + 0.5f)},
            .size = vec2{0, par.spheight},
            .gptr = nullptr,
            .characterIdx = static_cast<int>(par.e - par.eorg.begin()),
            .pixRatio = m_pixRatio
        };

        par.linep[1] = par.ve;

        par.fword.push_back(e_fontword{par.ws, par.ve + 1});

        AddLine(par.linep[0], par.linep[1], par.pos.y, par.fword, text_align_x, par.l_size.x, false);
        par.fword.clear();
    }

    m_v.resize(par.ve - vorig);
    calculateBoundingBoxHwp(m_bb);

    return true;
}

void FontGlyphVector::procTab(procPar& par) {
    float d = m_tabSize > 0 ? m_tabSize - fmodf(par.pos.x, m_tabSize) : 0;

    if (!par.ws) {
        par.ws = par.ve;
    }

    if (!par.linep[0]) {
        par.linep[0] = par.linep[1] = par.ve;
    }

    par.ve[0] = e_fontdglyph{
        .codepoint = par.ch,
        .pos = vec2{floor(par.pos.x + 0.5f), floor(par.pos.y - par.spheight + 0.5f)},
        .size = vec2{d, par.spheight},
        .gptr = nullptr,
        .characterIdx = static_cast<int>(par.e - par.eorg.begin()),
        .pixRatio = m_pixRatio
    };

    ++par.ve;
    par.pos.x += d;
    ++par.e;
}

void FontGlyphVector::procCrAndNl(procPar& par) {
    if (!par.ws) {
        par.ws = par.ve;
    }

    if (!par.linep[0]) {
        par.linep[0] = par.linep[1] = par.ve;
    }

    par.ve[0] = e_fontdglyph{
        .codepoint = par.ch,
        .pos = vec2{floor(par.pos.x + 0.5f), floor((par.pos.y - par.spheight) + 0.5f)},
        .size = vec2{0, par.spheight},
        .gptr = nullptr,
        .characterIdx = static_cast<int>(par.e - par.eorg.begin()),
        .pixRatio = m_pixRatio
    };

    par.linep[1] = par.ve;
    ++par.ve;

    if (par.ws) {
        par.fword.push_back(e_fontword{par.ws, par.ve});
    }

    AddLine(par.linep[0], par.linep[1], par.pos.y, par.fword, par.text_align_x, par.l_size.x, true);

    par.fword.clear();

    par.linep[0] = par.linep[1] = nullptr;
    par.ws                  = nullptr;

    par.pos.x = 0;
    par.pos.y += par.lineheight + par.l_sep.x;
    ++par.e;
}

void FontGlyphVector::procSpace(procPar& par, Font* font) {
    if (!par.ws) {
        par.ws = par.ve;
    }

    if (!par.linep[0]) {
        par.linep[0] = par.linep[1] = par.ve;
    }

    e_fontglyph            *g  = nullptr;
    if ((g = font->getGlyph(par.ch)) != nullptr) {
        par.ve[0] = e_fontdglyph{
            .codepoint = par.ch,
            .pos = vec2{floor((par.pos.x + g->off[0]) + 0.5f), floor(par.pos.y - par.spheight + 0.5f)},
            .size = vec2{g->xadv, par.spheight},
            .gptr =         g,
            .characterIdx = static_cast<int>(par.e - par.eorg.begin()),
            .pixRatio = m_pixRatio
        };
        ++par.ve;
        par.pos.x += g->xadv + par.l_sep.x;
    }
    ++par.e;
}

void FontGlyphVector::procChar(procPar& par, Font* font, bool word_wrap) {
    if (!par.linep[0]) {
        par.linep[0] = par.linep[1] = par.ve;
    }

    if (par.lch <= 32) {
        if (par.ws) {
            par.fword.emplace_back(e_fontword{par.ws, par.ve});
        }
        par.ws = par.ve;
    }

    e_fontglyph            *g  = nullptr;
    if ((g = font->getGlyph(par.ch)) != nullptr) {
        par.ve[0]  = e_fontdglyph{
            .codepoint = par.ch,
            .pos = vec2{floor((par.pos.x + g->off[0]) + 0.5f), floor((par.pos.y + g->off[1]) + 0.5f)},
            .size = vec2{g->outpixsize[0], g->outpixsize[1]},
            .gptr = g,
            .characterIdx = static_cast<int>(par.e - par.eorg.begin()),
            .pixRatio = m_pixRatio
        };

        if (par.pos.x + par.ve[0].size.x >= par.l_size.x && word_wrap) {
            if (!par.fword.empty()) {
                par.linep[1] = par.ws;

                AddLine(par.linep[0], par.linep[1] - 1, par.pos.y, par.fword, par.text_align_x, par.l_size.x, false);
                par.fword.clear();

                par.pos.x = 0;
                par.pos.y += par.lineheight + par.l_sep.y;

                if (par.ws) {
                    par.ve = par.ws;
                    par.e  = par.eorg.begin() + par.ws->characterIdx;
                } else {
                    ++par.ve;
                    ++par.e;
                }

                par.linep[0] = par.linep[1] = nullptr;
            } else {
                par.linep[1] = par.ve;

                par.fword.push_back(e_fontword{par.linep[0], par.linep[1]});
                AddLine(par.linep[0], par.linep[1], par.pos.y, par.fword, par.text_align_x, par.l_size.x, false);
                par.fword.clear();

                par.pos.x = 0;
                par.pos.y += par.lineheight + par.l_sep.y;

                par.linep[0] = par.linep[1] = nullptr;
                par.ws                  = par.ve;
                ++par.ve;
                ++par.e;
            }
        } else {
            par.pos.x += g->xadv + par.l_sep.x;
            ++par.ve;
            ++par.e;
        }
    } else {
        ++par.e;
    }
}

/**  y and width are in hw pixels */
e_fontline *FontGlyphVector::AddLine(e_fontdglyph *p_begin, e_fontdglyph *p_end, float y,
                                     std::vector<e_fontword> &inFword, align text_align_x, float width, bool eol) {
//e_fontline *FontGlyphVector::addLine(procPar& par, bool eol) {

    e_fontline   *fline = nullptr;
    e_fontdglyph *rr[2]{};

    if (!p_begin || !p_end) {
        return nullptr;
    }

    if (p_begin == p_end) {
        m_vline.emplace_back(e_fontline{p_begin, p_end, y, 0, p_begin->characterIdx, p_end->characterIdx, y - m_pixVMetrics[0],
                                      y - m_pixVMetrics[1], y - m_pixVMetrics[0],
                                      y - m_pixVMetrics[0] + m_pixLineHeight + m_sep[1], m_pixRatio});
        fline = &m_vline.back();
        return fline;
    }

    float lw = p_end->pos.x - p_begin->pos.x;

    m_vline.emplace_back(e_fontline{p_begin, p_end, y, lw, p_begin->characterIdx, p_end->characterIdx, y - m_pixVMetrics[0],
                                  y - m_pixVMetrics[1], y - m_pixVMetrics[0],
                                  y - m_pixVMetrics[0] + m_pixLineHeight + m_sep[1], m_pixRatio});
    fline = &m_vline.back();

    if (text_align_x != align::left) {
        e_fontdglyph *dg;
        if (text_align_x == align::center)  // CENTER
        {
            float xo = width * 0.5f - lw * 0.5f;

            for (e_fontword &w : inFword) {
                for (dg = w.ptr[0]; dg < w.ptr[1]; dg++) {
                    dg->pos.x += xo;
                }
            }
        } else if (text_align_x == align::right) {  // RIGHT

            float xo = width - lw;

            for (e_fontword &w : inFword) {
                for (dg = w.ptr[0]; dg < w.ptr[1]; dg++) {
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
                float xo = 0;
                dg       = p_begin;

                while (dg->codepoint <= 32 && dg < p_end) {
                    dg->size.x = 0;
                    dg->pos.x  = xo;
                    dg++;
                }

                if (dg < p_end && dg->codepoint > 32) {
                    rr[0] = dg;
                    dg    = p_end;

                    while (dg->codepoint <= 32 && dg > p_begin) {
                        dg->size.x = 0;
                        dg->pos.x  = width;
                        --dg;
                    }

                    if (dg > p_begin && dg->codepoint > 32) {
                        rr[1] = dg;

                        float tx = 0;
                        int   nc = 0;

                        for (dg = rr[0]; dg <= rr[1]; dg++) {
                            if (dg->codepoint <= 32) {
                                ++nc;
                            }
                            if (dg->codepoint > 32) {
                                tx += dg->size.x;
                            }
                        }

                        if (nc > 0) {
                            auto dx = (width - tx) / static_cast<float>(nc);
                            auto x  = xo;

                            for (dg = rr[0]; dg <= rr[1]; dg++) {
                                dg->pos.x = x;
                                if (dg->codepoint <= 32) {
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
    auto l = static_cast<int>(u.length());
    if (l < 1) {
        return -1;
    }

    unsigned char u0 = u[0];
    if (u0 >= 0 && u0 <= 127) {
        return u0;
    }

    if (l < 2) {
        return -1;
    }

    unsigned char u1 = u[1];
    if (u0 >= 192 && u0 <= 223) {
        return (u0 - 192) * 64 + (u1 - 128);
    }

    if (static_cast<unsigned char>(u[0]) == 0xed && (u[1] & 0xa0) == 0xa0) {
        return -1;
    }  // code points, 0xd800 to 0xdfff

    if (l < 3) {
        return -1;
    }

    unsigned char u2 = u[2];
    if (u0 >= 224 && u0 <= 239) {
        return (u0 - 224) * 4096 + (u1 - 128) * 64 + (u2 - 128);
    }

    if (l < 4) {
        return -1;
    }

    unsigned char u3 = u[3];
    if (u0 >= 240 && u0 <= 247) {
        return (u0 - 240) * 262144 + (u1 - 128) * 4096 + (u2 - 128) * 64 + (u3 - 128);
    }
    return -1;
}

}