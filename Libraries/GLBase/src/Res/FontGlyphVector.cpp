//
// Created by hahne on 22.04.2025.
//

#include "Res/FontGlyphVector.h"
#include "Res/Font.h"
#include "Res/UtfIterator.h"

using namespace glm;

namespace ara {

unsigned FontGlyphVector::calculateBoundingBoxHwp(glm::vec4 &bb) {
    memset(&bb[0], 0, 16);

    unsigned i = 0;
    for (e_fontline &fl : vline) {
        if (!i) {
            bb.x = fl.ptr[0]->opos.x;
            bb.z = fl.ptr[0]->opos.x + fl.width;
            bb.y = fl.yrange[0];
            bb.w = fl.yrange[1];
        } else {
            bb.x = std::min(bb.x, fl.ptr[0]->opos.x);
            bb.z = std::max(bb.z, fl.ptr[0]->opos.x + fl.width);
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

glm::vec4 FontGlyphVector::calculateBoundingBoxHwp() {
    glm::vec4 bb;
    calculateBoundingBoxHwp(bb);
    return bb;
}

glm::vec2 FontGlyphVector::getPixSize() {
    glm::vec4 bb = calculateBoundingBoxHwp();
    return {(bb.z - bb.x) / m_pixRatio, (bb.w - bb.y) / m_pixRatio};
}

glm::vec2 FontGlyphVector::getPixSizeHwp() {
    glm::vec4 bb = calculateBoundingBoxHwp();
    return {(bb.z - bb.x), (bb.w - bb.y)};
}

void FontGlyphVector::reset(const Font *font) {
    v.clear();
    vline.clear();

    if (font != nullptr) {
        m_PixLineHeight  = font->getPixHeightHwp();
        m_PixVMetrics[0] = font->getPixAscentHwp();
        m_PixVMetrics[1] = font->getPixDescentHwp();
        m_PixVMetrics[2] = font->getPixLineGapHwp();
    }
}

e_fontdglyph FontGlyphVector::findByCharIndex(int idx) {
    for (e_fontdglyph &gIt : v) {
        if (gIt.chidx == idx) {
            return gIt;
        }
    }

    return e_fontdglyph{};
}

int FontGlyphVector::getLineIndexByPixPos(float pix_x, float pix_y, float off_x, float off_y) {
    int i = 0;

    pix_y -= off_y;

    float hwPix_x = pix_x * m_pixRatio;
    float hwPix_y = pix_y * m_pixRatio;

    // convert to hw pixels
    if (hwPix_x < m_BB[1]) {
        return -1;
    }

    if (hwPix_y > m_BB[3]) {
        return -2;
    }

    for (e_fontline &l : vline) {
        if (hwPix_x >= l.yselrange[0] && hwPix_y < l.yselrange[1]) {
            return i;
        }
        i++;
    }

    return -1;
}

int FontGlyphVector::getLineIndexByCharIndex(int ch_index) {
    int i = 0;

    for (e_fontline &l : vline) {
        if (ch_index >= l.chidx[0] && ch_index <= l.chidx[1]) {
            return i;
        }

        i++;
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
            return static_cast<int>(v.size());
        }

        return 0;
    }

    pix_y -= off_y;
    pix_x -= off_x;

    float hwPix_x = pix_x * m_pixRatio;

    auto e = vline[lidx].ptr[0];

    if (e == nullptr || !vline[lidx].ptr[1]) {
        return -1;
    }

    if (hwPix_x <= m_BB[0]) {
        return vline[lidx].ptr[0]->chidx;
    }

    if (hwPix_x >= m_BB[2]) {
        return vline[lidx].ptr[1]->chidx;
    }

    if (hwPix_x < e[0].opos.x) {
        return e[0].chidx;
    }

    e = vline[lidx].ptr[0];

    while (e < vline[lidx].ptr[1]) {
        if (hwPix_x >= e[0].opos.x && hwPix_x < e[1].opos.x) {
            return e[0].chidx;
        }
        ++e;
    }

    if (e == vline[lidx].ptr[1]) {
        if (hwPix_x >= e[0].opos.x && hwPix_x <= e[0].opos.x + e[0].osize.x) {
            return e[0].chidx;
        }
    }

    return vline[lidx].ptr[1]->chidx;
}

glm::vec2 &FontGlyphVector::getCaretPos(glm::vec2 &pos, int caret_index) {
    auto nchars = static_cast<int>(size());
    memset(&pos[0], 0, sizeof(float) * 2);

    if (!nchars) {
        return pos;
    }

    if (nchars > 0) {
        e_fontdglyph pr = findByCharIndex(caret_index);

        if (caret_index < nchars) {
            if (pr.cp > 0) {
                pos.x = pr.opos.x;
                pos.y = (pr.opos.y + pr.osize.y);
            }
        } else {
            pos = v[nchars - 1].opos + v[nchars - 1].osize;
        }

        if (caret_index < 0) {
            pos.y = vline[0].y;
        } else if (caret_index >= static_cast<int>(v.size())) {
            pos.y = vline[vline.size() - 1].y;
        } else {
            auto lidx = getLineIndexByCharIndex(caret_index);
            if (lidx < 0) {
                pos.y = vline[0].y;
            } else {
                pos.y = vline[lidx].y;
            }
        }
    }

    pos /= m_pixRatio;  // convert to virtual pixels

    return pos;
}

int FontGlyphVector::jumpToLine(int caret_index, int line_delta) {
    int ci   = caret_index;
    int lidx = getLineIndexByCharIndex(ci) + line_delta;

    if (lidx < 0 || lidx >= static_cast<int>(vline.size())) {
        return ci;
    }

    getCaretPos(m_tCaretPos, ci);

    auto ve = vline[lidx].ptr[0];

    if (ve == nullptr || !vline[lidx].ptr[1]) {
        return ci;
    }

    if (m_tCaretPos.x <= m_BB.x) {
        return vline[lidx].ptr[0]->chidx;
    }

    if (m_tCaretPos.x >= m_BB.z) {
        return vline[lidx].ptr[1]->chidx;
    }

    if (m_tCaretPos.x < ve[0].opos.x) {
        return ve[0].chidx;
    }

    ci = ve[0].chidx;
    float dist, mdist = 1e10;

    while (ve < vline[lidx].ptr[1]) {
        if ((dist = fabsf(ve[0].opos.x - m_tCaretPos.x)) < mdist) {
            ci    = ve[0].chidx;
            mdist = dist;
        }
    }

    return ci;
}

int FontGlyphVector::jumpToBeginOfLine(int caret_index) {
    auto lidx = getLineIndexByCharIndex(caret_index);

    if (lidx < 0 || lidx >= static_cast<int>(vline.size())) {
        return caret_index;
    }

    if (vline[lidx].ptr[0] == nullptr) {
        return caret_index;
    }

    return vline[lidx].ptr[0]->chidx;
}

int FontGlyphVector::jumpToEndOfLine(int caret_index) {
    auto lidx = getLineIndexByCharIndex(caret_index);

    if (lidx < 0 || lidx >= static_cast<int>(vline.size())) {
        return caret_index;
    }

    if (vline[lidx].ptr[1] == nullptr) {
        return caret_index;
    }

    return vline[lidx].ptr[1]->chidx;
}

bool FontGlyphVector::Process(Font *font, const vec2 &size, const vec2 &sep, align text_align_x, const std::string &str, bool word_wrap) {
    if (!font || size.x == 0.f || size.y == 0.f) {
        return false;
    }

    reset(font);  // gets font metrics

    // convert from virtual pixels to hardware pixels
    auto l_sep  = sep * m_pixRatio;
    auto l_size = size * m_pixRatio;
    m_Sep  = l_sep;

    if (str.empty() || !font->isOK()) {
        return false;
    }

    for (int i = 0; i < 2; i++) {
        m_sc[i] = 1.f;
    }

    glm::vec2 p{0.f};
    e_fontdglyph* linep[2]{nullptr};
    e_fontdglyph* ws = nullptr;
    uint8_t lch = 0;
    uint8_t ch = 0;
    std::vector<e_fontword> fword;
    e_fontdglyph           *ve = nullptr;

    auto eorg = str;

    auto e = eorg.begin();

    float lineheight = m_PixLineHeight;
    float spheight   = m_PixVMetrics[0];

    // resize the glyph vector to the size of the input string
    v.resize(str.size() + 2);

    auto vorig = ve = &v[0];

    if (e == eorg.end()) {
        return false;
    }

    while (e != eorg.end()) {
        UtfIterator iter(e);
        ch = codepoint(*iter);

        if (ch == 9) {
            float d = m_TabSize > 0 ? m_TabSize - fmodf(p.x, m_TabSize) : 0;

            if (!ws) {
                ws = ve;
            }

            if (!linep[0]) {
                linep[0] = linep[1] = ve;
            }

            ve[0] = e_fontdglyph{ch,
                                 vec2{floor(p.x + 0.5f), floor((p.y - spheight) + 0.5f)},
                                 vec2{d, spheight},
                                 nullptr,
                                 static_cast<int>(e - eorg.begin()),
                                 m_pixRatio};
            ++ve;

            p[0] += d;
            ++e;
        } else if (ch == 13 || ch == 10) {
            if (!ws) {
                ws = ve;
            }
            if (!linep[0]) {
                linep[0] = linep[1] = ve;
            }

            ve[0] = e_fontdglyph{ch,
                                 vec2{floor(p[0] + 0.5f), floor((p[1] - spheight) + 0.5f)},
                                 vec2{0, spheight},
                                 nullptr,
                                 static_cast<int>(e - eorg.begin()),
                                 m_pixRatio};

            linep[1] = ve;
            ++ve;

            if (ws) {
                fword.push_back(e_fontword{ws, ve});
            }

            AddLine(linep[0], linep[1], p[1], fword, text_align_x, l_size.x, true);

            fword.clear();

            linep[0] = linep[1] = nullptr;
            ws                  = nullptr;

            p.x = 0;
            p.y += lineheight + l_sep.x;
            ++e;
        } else if (ch == 32) {
            if (!ws) ws = ve;
            if (!linep[0]) {
                linep[0] = linep[1] = ve;
            }

            e_fontglyph            *g  = nullptr;
            if ((g = font->getGlyph(ch)) != nullptr) {
                ve[0] = e_fontdglyph{ch,
                                     vec2{floor((p[0] + g->off[0]) + 0.5f), floor(p[1] - spheight + 0.5f)},
                                     vec2{g->xadv, spheight},
                                     g,
                                     static_cast<int>(e - eorg.begin()),
                                     m_pixRatio};
                ++ve;
                p.x += g->xadv + l_sep.x;
            }
            ++e;
        } else if (ch > 32 && ch < 255) {
            if (!linep[0]) {
                linep[0] = linep[1] = ve;
            }

            if (lch <= 32) {
                if (ws) {
                    fword.emplace_back(e_fontword{ws, ve});
                }
                ws = ve;
            }

            e_fontglyph            *g  = nullptr;
            if ((g = font->getGlyph(ch)) != nullptr) {
                ve[0]  = e_fontdglyph{ch,
                                     vec2{floor((p[0] + g->off[0]) + 0.5f), floor((p[1] + g->off[1]) + 0.5f)},
                                     vec2{g->outpixsize[0], g->outpixsize[1]},
                                     g,
                                     static_cast<int>(e - eorg.begin()),
                                     m_pixRatio};

                if (p[0] + ve[0].osize.x >= l_size.x && word_wrap) {
                    if (!fword.empty()) {
                        linep[1] = ws;

                        AddLine(linep[0], linep[1] - 1, p.y, fword, text_align_x, l_size.x, false);
                        fword.clear();

                        p.x = 0;
                        p.y += lineheight + l_sep.y;

                        if (ws) {
                            ve = ws;
                            e  = eorg.begin() + ws->chidx;
                        } else {
                            ++ve;
                            ++e;
                        }

                        linep[0] = linep[1] = nullptr;
                    } else {
                        linep[1] = ve;

                        fword.push_back(e_fontword{linep[0], linep[1]});
                        AddLine(linep[0], linep[1], p.y, fword, text_align_x, l_size.x, false);
                        fword.clear();

                        p.x = 0;
                        p.y += lineheight + l_sep.y;

                        linep[0] = linep[1] = nullptr;
                        ws                  = ve;
                        ++ve;
                        ++e;
                    }
                } else {
                    p.x += g->xadv + l_sep.x;
                    ++ve;
                    ++e;
                }
            } else {
                ++e;
            }
        } else {
            ++e;
        }

        lch = ch;
    }

    if (linep[0]) {
        ve[0] = e_fontdglyph{0,
                             vec2{floor((p[0]) + 0.5f), floor((p[1] - spheight) + 0.5f)},
                             vec2{0, spheight},
                             nullptr,
                             static_cast<int>(e - eorg.begin()),
                             m_pixRatio};

        linep[1] = ve;

        fword.push_back(e_fontword{ws, ve + 1});

        AddLine(linep[0], linep[1], p[1], fword, text_align_x, l_size.x, false);
        fword.clear();
    }

    v.resize(ve - vorig);
    calculateBoundingBoxHwp(m_BB);

    return true;
}

/**  y and width are in hw pixels */
e_fontline *FontGlyphVector::AddLine(e_fontdglyph *p_begin, e_fontdglyph *p_end, float y,
                                     std::vector<e_fontword> &inFword, align text_align_x, float width, bool eol) {
    e_fontline   *fline = nullptr;
    e_fontdglyph *rr[2]{};

    if (!p_begin || !p_end) {
        return nullptr;
    }

    if (p_begin == p_end) {
        vline.emplace_back(e_fontline{p_begin, p_end, y, 0, p_begin->chidx, p_end->chidx, y - m_PixVMetrics[0],
                                      y - m_PixVMetrics[1], y - m_PixVMetrics[0],
                                      y - m_PixVMetrics[0] + m_PixLineHeight + m_Sep[1], m_pixRatio});
        fline = &vline.back();
        return fline;
    }

    float lw = p_end->opos.x - p_begin->opos.x;

    vline.emplace_back(e_fontline{p_begin, p_end, y, lw, p_begin->chidx, p_end->chidx, y - m_PixVMetrics[0],
                                  y - m_PixVMetrics[1], y - m_PixVMetrics[0],
                                  y - m_PixVMetrics[0] + m_PixLineHeight + m_Sep[1], m_pixRatio});
    fline = &vline.back();

    if (text_align_x != align::left) {
        e_fontdglyph *dg;
        if (text_align_x == align::center)  // CENTER
        {
            float xo = width * 0.5f - lw * 0.5f;

            for (e_fontword &w : inFword) {
                for (dg = w.ptr[0]; dg < w.ptr[1]; dg++) {
                    dg->opos.x += xo;
                }
            }
        } else if (text_align_x == align::right) {  // RIGHT

            float xo = width - lw;

            for (e_fontword &w : inFword) {
                for (dg = w.ptr[0]; dg < w.ptr[1]; dg++) {
                    dg->opos.x += xo;
                }
            }
        } else if (text_align_x == align::justify ||
                   text_align_x == align::justify_ex) {  // JUSTIFIED (4 is whole, even EOL)

            bool doit = true;
            if (text_align_x == align::justify && eol) doit = false;

            if (doit) {
                float xo = 0;
                dg       = p_begin;

                while (dg->cp <= 32 && dg < p_end) {
                    dg->osize.x = 0;
                    dg->opos.x  = xo;
                    dg++;
                }

                if (dg < p_end && dg->cp > 32) {
                    rr[0] = dg;
                    dg    = p_end;

                    while (dg->cp <= 32 && dg > p_begin) {
                        dg->osize.x = 0;
                        dg->opos.x  = width;
                        dg--;
                    }

                    if (dg > p_begin && dg->cp > 32) {
                        rr[1] = dg;

                        float tx = 0;
                        int   nc = 0;

                        for (dg = rr[0]; dg <= rr[1]; dg++) {
                            if (dg->cp <= 32) {
                                ++nc;
                            }
                            if (dg->cp > 32) {
                                tx += dg->osize.x;
                            }
                        }

                        if (nc > 0) {
                            auto dx = (width - tx) / static_cast<float>(nc);
                            auto x  = xo;

                            for (dg = rr[0]; dg <= rr[1]; dg++) {
                                dg->opos.x = x;
                                if (dg->cp <= 32) {
                                    dg->osize.x = dx;
                                }
                                x += dg->osize.x;
                            }
                        }
                    }
                }
            }
        }
    }

    return fline;
}

int FontGlyphVector::codepoint(std::string u) {
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