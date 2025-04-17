#include "ResGlFont.h"

#include <Shaders/ShaderCollector.h>
#include <Utils/Texture.h>

#include "RwBinFile.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>  // http://nothings.org/stb/stb_truetype.h

using namespace std;
using namespace glm;

/**
 * OpenGL Font rendering
 * Note: all pixel arguments are meant in "virtual pixels", that is dpi
 * independent pixel values. all class member pixel values are meant in hardware
 * pixels, that is 1:1 in display (unscaled) resolution
 */

namespace ara {

Font::Font(std::string font_path, int size, float pixRatio) {
    m_FontSize = size;
    m_pixRatio = pixRatio;
    setFontType(std::move(font_path));
}

Font::Font(vector<uint8_t> &vp, const std::string &font_path, int size, float pixRatio) {
    m_pixRatio = pixRatio;
    Create(vp, font_path, size, pixRatio);
}

bool Font::Create(const std::string &font_path, int font_size, float pixRatio) {
    vector<uint8_t> vp;

    if (ReadBinFile(vp, font_path) <= 0) {
        LOGE << "Create return false";
        return false;
    }

    return Create(vp, font_path, font_size, pixRatio);
}

bool Font::Create(vector<uint8_t> &vp, const std::string &font_path, int font_size, float pixRatio) {
    ret          = false;
    m_pixRatio   = pixRatio;
    m_hwFontSize = std::floor((float)font_size * pixRatio);
    m_FontSize   = font_size;

    if (font_size <= 0 || vp.empty()) {
        LOGE << "Font::Create return false";
        return false;
    }

    buff      = &vp[0];
    auto info = stbtt_fontinfo{};

    try {
        rerr = stbtt_InitFont(&info, buff, 0);
    } catch (...) {
        LOGE << "stbtt_InitFont failed";
    }

    if (rerr) {
        int   wh     = 0;
        int   ch_off = m_CodePoint_Range[0], ch_count = m_CodePoint_Range[1] - m_CodePoint_Range[0] + 1;
        auto *bchar = new stbtt_bakedchar[ch_count];
        int   tret, bi;
        float cdiv;

        for (bi = 7; bi <= 12 && !ret; bi++)  // try from 128x128 up to 4096x4096
        {
            wh   = 1 << bi;
            cdiv = 1.f / float(wh);

            auto *bmp = new unsigned char[wh * wh];

            memset(bchar, 0, ch_count * sizeof(stbtt_bakedchar));

            if ((tret = stbtt_BakeFontBitmap(buff, 0, m_hwFontSize, bmp, wh, wh, ch_off, ch_count, bchar)) >= 0) {
                for (int i = 0; i < ch_count; i++)
                    m_Glyphs.push_back(e_fontglyph{
                        i + ch_off,                                                                 // codepoint
                        bchar[i].x0,                                                                // ppos[2][2]
                        bchar[i].y0, bchar[i].x1, bchar[i].y1, vec2{bchar[i].xoff, bchar[i].yoff},  // off
                        bchar[i].yoff2,                                                             // yoff2
                        bchar[i].xadvance, vec2{float(bchar[i].x1 - bchar[i].x0), float(bchar[i].y1 - bchar[i].y0)},
                        vec2{(float)bchar[i].x0 * cdiv, (float)bchar[i].y0 * cdiv},
                        vec2{float(bchar[i].x1 - bchar[i].x0) * cdiv, float(bchar[i].y1 - bchar[i].y0) * cdiv}});

                glGenTextures(1, &gl_TexID);
                glBindTexture(GL_TEXTURE_2D, gl_TexID);
                glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, wh, wh);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, wh, wh, GL_RED, GL_UNSIGNED_BYTE, bmp);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glBindTexture(GL_TEXTURE_2D, 0);

                m_GlyphTexSize[0] = m_GlyphTexSize[1] = wh;
                m_FontType                            = "";
                m_FontScale                           = stbtt_ScaleForPixelHeight(&info, m_hwFontSize);
                stbtt_GetFontVMetrics(&info, &m_FontVMetrics[0], &m_FontVMetrics[1], &m_FontVMetrics[2]);
                m_glCreated = true;

                ret = true;

                setFontType(font_path);
            }
            delete[] bmp;
        }

        if (!ret) LOGE << "[ERROR] NO SUITABLE SIZE FOR FONT";
        delete[] bchar;
    }

    return ret;
}

// all input values are in virtual pixels and must be converted to hw pixels
int Font::drawDGlyphs(FontGlyphVector &dgv, glm::mat4 *mvp, Shaders *shdr, GLuint vao, float *tcolor, float off_x,
                      float off_y, float mask_x, float mask_y, float mask_w, float mask_h) {
    if (!isOK()) return 0;
    if (tcolor == nullptr) tcolor = def_color;

    if (shdr) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gl_TexID);

        shdr->begin();
        shdr->setUniform1i("stex", 0);
        shdr->setUniformMatrix4fv("mvp", &((*mvp)[0][0]));
        shdr->setUniform4fv("tcolor", tcolor);
        shdr->setUniform2f("off", off_x * m_pixRatio, off_y * m_pixRatio);
        shdr->setUniform2f("mask_pos", mask_x * m_pixRatio, mask_y * m_pixRatio);
        shdr->setUniform2f("mask_size", mask_w * m_pixRatio, mask_h * m_pixRatio);
        shdr->setUniform1f("pixRatio", m_pixRatio);

        for (e_fontdglyph &g : dgv.v) {
            if (g.gptr) {
                shdr->setUniform2fv("opos", &g.opos[0]);
                shdr->setUniform2fv("osize", &g.osize[0]);
                shdr->setUniform2fv("tpos", &g.gptr->srcpixpos[0]);
                shdr->setUniform2fv("tsize", &g.gptr->srcpixsize[0]);

                glBindVertexArray(vao);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }
    }

    return 0;
}

/** input x and y in virtual pixels */
int Font::write(glm::mat4 *mvp, Shaders *shdr, GLuint vao, float *tcolor, float x, float y, const std::string &str) {
    if (!isOK()) return 0;

    FontGlyphVector dgv;
    glm::vec2       size{1e10, 1e10};
    glm::vec2       pos{0, 0};
    dgv.Process(this, size, pos, align::left, str, true);
    drawDGlyphs(dgv, mvp, shdr, vao, tcolor, x, y + getPixAscent(), 0, 0, 1e10, 1e10);

    return 0;
}

int Font::writeFormat(glm::mat4 *mvp, Shaders *shdr, GLuint vao, float *tcolor, float x, float y, char *f, ...) {
    char    se[4096];
    va_list p;

    va_start(p, f);
    vsprintf(se, f, p);
    va_end(p);

    return write(mvp, shdr, vao, tcolor, x, y, se);
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// FontList
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Font *FontList::get(std::string font_path, int size, float pixRatio) {
    return add(std::move(font_path), size, pixRatio);
}

Font *FontList::find(std::string &font_path, int size, float pixRatio) {
    for (auto &f : m_FontList)
        if (f->isFontType(font_path, size, pixRatio)) return f.get();

    return nullptr;
}

Font *FontList::add(std::string font_path, int size, float pixRatio) {
    Font *font;

    if ((font = find(font_path, size, pixRatio)) != nullptr) return font;

    m_FontList.push_back(make_unique<Font>(font_path, size, pixRatio));
    update3DLayers();
    return m_FontList.back().get();
}

Font *FontList::addFromFilePath(std::string font_path, int size, float pixRatio) {
    Font *font;
    if ((font = find(font_path, size, pixRatio)) != nullptr) return font;

    std::vector<uint8_t> vp;
    if (!ReadBinFile(vp, font_path)) return nullptr;

    m_FontList.push_back(make_unique<Font>(vp, font_path, size, pixRatio));
    update3DLayers();
    return m_FontList.back().get();
}

Font *FontList::add(std::vector<uint8_t> &vp, std::string font_path, int size, float pixRatio) {
    Font *font;
    if ((font = find(font_path, size, pixRatio)) != nullptr) return font;

    m_FontList.emplace_back(make_unique<Font>(vp, font_path, size, pixRatio));
    update3DLayers();

    return m_FontList.back().get();
}

/** copy all glyph texture maps of the same size into one TEXTURE_3D for
 * DrawManager indirect drawing */
void FontList::update3DLayers() {
    m_layerCount.clear();

    // check how many glyph texture maps are there for each size
    for (auto &fnt : m_FontList) m_layerCount[fnt->getGlyphTexSize()].emplace_back(fnt.get());

    // check which of the layers changed and update those
    bool entrExists = false;
    int  layerCnt   = 0;
    int  sz         = 0;
    for (auto &lc : m_layerCount) {
        sz         = lc.first;
        entrExists = m_fontTexLayers.find(sz) != m_fontTexLayers.end();

        // 3d texture doesn't exist, or quantity changed
        if (!entrExists || (entrExists && m_fontTexLayers[sz]->getDepth() != (uint)lc.second.size())) {
            // unfortunately textures can't be resized, conserving the existing
            // content, so we have to delete them
            if (entrExists) m_fontTexLayers[sz]->releaseTexture();

            // allocate a new texture
            m_fontTexLayers[sz] = make_unique<Texture>(m_glbase);
            m_fontTexLayers[sz]->allocate3D(sz, sz, (uint32_t)m_layerCount[sz].size(), GL_R8, GL_RED, GL_TEXTURE_3D,
                                            GL_UNSIGNED_BYTE);

            layerCnt = 0;

#ifdef ARA_USE_GLES31
            std::vector<uint8_t> pixels(sz * sz);

            // on GLES the blitting trick doesn't work ... do this by copying to
            // the cpu and back copy the glyph textures into them, do this by
            for (auto &fnt : lc.second) {
                glesGetTexImage(fnt->getTexId(), GL_TEXTURE_2D, GL_RED, GL_UNSIGNED_BYTE, sz, sz, &pixels[0]);
                glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, layerCnt, sz, sz, 1, GL_RED, GL_UNSIGNED_BYTE, &pixels[0]);
                fnt->setTexLayer(m_fontTexLayers[sz]->getId(), layerCnt, (uint32_t)m_layerCount[sz].size());
                layerCnt++;
            }
#else
#ifdef __APPLE__
            if (!m_glbase->rendererIsIntel()) {
#endif
                m_fbo.setGlbase(m_glbase);
                m_fbo.getActStates();  // save last state
                m_fbo.genFbo();

                std::array<GLenum, 2> bufModes = {GL_NONE, GL_COLOR_ATTACHMENT1};
                glDrawBuffers(2, &bufModes[0]);

                // copy the glyph textures into them, do this by fbo blitting if
                // possible
                for (auto &fnt : lc.second) {
                    // attach the source texture to color attachment0
                    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fnt->getTexId(),
                                           0);

                    // attach the destination texture layer to color attachment1
                    glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, m_fontTexLayers[sz]->getId(),
                                              0, layerCnt);

                    m_fbo.checkFbo();

                    glBlitFramebuffer(0, 0, sz, sz, 0, 0, sz, sz, GL_COLOR_BUFFER_BIT, GL_NEAREST);

                    fnt->setTexLayer(m_fontTexLayers[sz]->getId(), layerCnt, (uint32_t)m_layerCount[sz].size());
                    layerCnt++;
                }

                m_fbo.deleteFbo();
                m_fbo.restoreStates();  // restore states
#ifdef __APPLE__
            } else {
                // special treatment for macOS with intel gpu which don't
                // supported mixed TEXTURE_2D and TEXTURE_3D attachments copy to
                // cpu and back
                std::vector<uint8_t> pixels(sz * sz);
                for (auto &fnt : lc.second) {
                    glBindTexture(GL_TEXTURE_2D, fnt->getTexId());
                    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, &pixels[0]);
                    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, layerCnt, sz, sz, 1, GL_RED, GL_UNSIGNED_BYTE, &pixels[0]);
                    fnt->setTexLayer(m_fontTexLayers[sz]->getId(), layerCnt, (uint32_t)m_layerCount[sz].size());
                    layerCnt++;
                }
            }
#endif
#endif
        }
    }
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// FontGlyphVector
// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------

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

        i++;
    }

    return i;
}

unsigned FontGlyphVector::calculateBoundingBox(glm::vec4 &bb) {
    unsigned res = calculateBoundingBoxHwp(bb);
    for (int i = 0; i < 4; i++) bb[i] /= m_pixRatio;
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

void FontGlyphVector::reset(Font *font) {
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
    for (e_fontdglyph &gIt : v)
        if (gIt.chidx == idx) return gIt;

    return e_fontdglyph{};
}

int FontGlyphVector::getLineIndexByPixPos(float pix_x, float pix_y, float off_x, float off_y) {
    int i = 0;

    pix_y -= off_y;

    float hwPix_x = pix_x * m_pixRatio;
    float hwPix_y = pix_y * m_pixRatio;

    // convert to hw pixels
    if (hwPix_x < m_BB[1]) return -1;
    if (hwPix_y > m_BB[3]) return -2;

    for (e_fontline &l : vline) {
        if (hwPix_x >= l.yselrange[0] && hwPix_y < l.yselrange[1]) return i;
        i++;
    }

    return -1;
}

int FontGlyphVector::getLineIndexByCharIndex(int ch_index) {
    int i = 0;

    for (e_fontline &l : vline) {
        if (ch_index >= l.chidx[0] && ch_index <= l.chidx[1]) return i;

        i++;
    }

    return -1;
}

int FontGlyphVector::getCharIndexByPixPos(float pix_x, float pix_y, float off_x, float off_y, int &off_bound) {
    lidx = getLineIndexByPixPos(pix_x, pix_y, off_x, off_y);

    off_bound = 0;

    if (lidx < 0) {
        if (lidx == -1) return 0;

        if (lidx == -2) return (int)v.size();

        return 0;
    }

    pix_y -= off_y;
    pix_x -= off_x;

    float hwPix_x = pix_x * m_pixRatio;
    float hwPix_y = pix_y * m_pixRatio;

    e_fontdglyph *e = vline[lidx].ptr[0];

    if (e == nullptr || !vline[lidx].ptr[1]) return -1;

    if (hwPix_x <= m_BB[0]) return vline[lidx].ptr[0]->chidx;

    if (hwPix_x >= m_BB[2]) return vline[lidx].ptr[1]->chidx;

    if (hwPix_x < e[0].opos.x) return e[0].chidx;

    e = vline[lidx].ptr[0];

    while (e < vline[lidx].ptr[1])  // bug fix, selection between last two
                                    // characters not possible
    {
        if (hwPix_x >= e[0].opos.x && hwPix_x < e[1].opos.x) return e[0].chidx;

        e++;
    }

    if (e == vline[lidx].ptr[1])
        if (hwPix_x >= e[0].opos.x && hwPix_x <= e[0].opos.x + e[0].osize.x) return e[0].chidx;

    return vline[lidx].ptr[1]->chidx;
}

glm::vec2 &FontGlyphVector::getCaretPos(glm::vec2 &pos, int caret_index) {
    int nchars = (int)size();

    memset(&pos[0], 0, sizeof(float) * 2);

    if (!nchars) return pos;

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
        } else if (caret_index >= (int)v.size()) {
            pos.y = vline[vline.size() - 1].y;
        } else {
            lidx = getLineIndexByCharIndex(caret_index);
            if (lidx < 0)
                pos.y = vline[0].y;
            else
                pos.y = vline[lidx].y;
        }
    }

    pos /= m_pixRatio;  // convert to virtual pixels

    return pos;
}

int FontGlyphVector::jumpToLine(int caret_index, int line_delta) {
    ci   = caret_index;
    lidx = getLineIndexByCharIndex(ci) + line_delta;

    if (lidx < 0 || lidx >= (int)vline.size()) return ci;

    getCaretPos(m_tCaretPos, ci);

    ve = vline[lidx].ptr[0];

    if (ve == nullptr || !vline[lidx].ptr[1]) return ci;

    if (m_tCaretPos.x <= m_BB.x) return vline[lidx].ptr[0]->chidx;
    if (m_tCaretPos.x >= m_BB.z) return vline[lidx].ptr[1]->chidx;

    if (m_tCaretPos.x < ve[0].opos.x) return ve[0].chidx;

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
    lidx = getLineIndexByCharIndex(caret_index);

    if (lidx < 0 || lidx >= (int)vline.size()) return caret_index;

    if (vline[lidx].ptr[0] == nullptr) return caret_index;

    return vline[lidx].ptr[0]->chidx;
}

int FontGlyphVector::jumpToEndOfLine(int caret_index) {
    lidx = getLineIndexByCharIndex(caret_index);

    if (lidx < 0 || lidx >= (int)vline.size()) return caret_index;

    if (vline[lidx].ptr[1] == nullptr) return caret_index;

    return vline[lidx].ptr[1]->chidx;
}

bool FontGlyphVector::Process(Font *font, vec2 &size, vec2 &sep, align text_align_x, const std::string &str,
                              bool word_wrap) {
    if (!font || size.x == 0.f || size.y == 0.f) return false;

    reset(font);  // gets font metrics

    // convert from virtual pixels to hardware pixels
    l_sep  = sep * m_pixRatio;
    l_size = size * m_pixRatio;
    m_Sep  = l_sep;

    if (str.empty() || !font->isOK()) return false;

    // reset local values
    for (int i = 0; i < 2; i++) {
        m_sc[i] = 1.f;
        p[i]    = 0;
    }

    for (auto &i : linep) i = nullptr;
    ve = vb = lastve = nullptr;
    ws               = nullptr;
    ch = lch = 0;
    fword.clear();

    eorg = str;

    auto e = eorg.begin();

    float lineheight = m_PixLineHeight;
    float spheight   = m_PixVMetrics[0];

    // resize the glyph vector to the size of the input string
    v.resize(str.size() + 2);

    vorig = ve = vb = &v[0];

    if (e == eorg.end()) return false;

    while (e != eorg.end()) {
        UtfIterator iter(e);
        ch = codepoint(*iter);

        if (ch == 9) {
            float d = m_TabSize > 0 ? m_TabSize - fmodf(p.x, m_TabSize) : 0;

            if (!ws) ws = ve;
            if (!linep[0]) {
                linep[0] = linep[1] = ve;
            }

            ve[0] = e_fontdglyph{ch,
                                 vec2{floor(p.x + 0.5f), floor((p.y - spheight) + 0.5f)},
                                 vec2{d, spheight},
                                 nullptr,
                                 (int)(e - eorg.begin()),
                                 m_pixRatio};
            ve++;

            p[0] += d;
            e++;
        } else if (ch == 13 || ch == 10) {
            if (!ws) ws = ve;
            if (!linep[0]) {
                linep[0] = linep[1] = ve;
            }

            ve[0] = e_fontdglyph{ch,
                                 vec2{floor(p[0] + 0.5f), floor((p[1] - spheight) + 0.5f)},
                                 vec2{0, spheight},
                                 nullptr,
                                 (int)(e - eorg.begin()),
                                 m_pixRatio};

            linep[1] = ve;
            ve++;

            if (ws) fword.push_back(e_fontword{ws, ve});

            AddLine(linep[0], linep[1], p[1], fword, text_align_x, l_size.x, true);

            fword.clear();

            linep[0] = linep[1] = nullptr;
            ws                  = nullptr;

            p.x = 0;
            p.y += lineheight + l_sep.x;
            e++;
        } else if (ch == 32) {
            if (!ws) ws = ve;
            if (!linep[0]) {
                linep[0] = linep[1] = ve;
            }

            if ((g = font->getGlyph(ch)) != nullptr) {
                ve[0] = e_fontdglyph{ch,
                                     vec2{floor((p[0] + g->off[0]) + 0.5f), floor(p[1] - spheight + 0.5f)},
                                     vec2{g->xadv, spheight},
                                     g,
                                     int(e - eorg.begin()),
                                     m_pixRatio};
                ve++;
                p.x += g->xadv + l_sep.x;
            }
            e++;
        } else if (ch > 32 && ch < 255) {
            if (!linep[0]) {
                linep[0] = linep[1] = ve;
            }

            if (lch <= 32) {
                if (ws) fword.emplace_back(e_fontword{ws, ve});
                ws = ve;
            }

            if ((g = font->getGlyph(ch)) != nullptr) {
                ve[0]  = e_fontdglyph{ch,
                                     vec2{floor((p[0] + g->off[0]) + 0.5f), floor((p[1] + g->off[1]) + 0.5f)},
                                     vec2{g->outpixsize[0], g->outpixsize[1]},
                                     g,
                                     int(e - eorg.begin()),
                                     m_pixRatio};
                lastve = ve;

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
                            ve++;
                            e++;
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
                        ve++;
                        e++;
                    }
                } else {
                    p.x += g->xadv + l_sep.x;
                    ve++;
                    e++;
                }
            } else {
                e++;
            }
        } else {
            e++;
        }

        lch = ch;
    }

    if (linep[0]) {
        ve[0] = e_fontdglyph{0,
                             vec2{floor((p[0]) + 0.5f), floor((p[1] - spheight) + 0.5f)},
                             vec2{0, spheight},
                             nullptr,
                             (int)(e - eorg.begin()),
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
    e_fontdglyph *dg, *rr[2]{};

    if (!p_begin || !p_end) return nullptr;

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
        if (text_align_x == align::center)  // CENTER
        {
            float xo = width * 0.5f - lw * 0.5f;

            for (e_fontword &w : inFword) {
                for (dg = w.ptr[0]; dg < w.ptr[1]; dg++) dg->opos.x += xo;
            }
        } else if (text_align_x == align::right) {  // RIGHT

            float xo = width - lw;

            for (e_fontword &w : inFword) {
                for (dg = w.ptr[0]; dg < w.ptr[1]; dg++) dg->opos.x += xo;
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

                        float cx = rr[1]->opos[0] - rr[0]->opos[0];
                        float dx, tx = 0;
                        int   nc = 0;

                        for (dg = rr[0]; dg <= rr[1]; dg++) {
                            if (dg->cp <= 32) nc++;
                            if (dg->cp > 32) tx += dg->osize.x;
                        }

                        if (nc > 0) {
                            dx      = (width - tx) / (float)nc;
                            float x = xo;

                            for (dg = rr[0]; dg <= rr[1]; dg++) {
                                dg->opos.x = x;

                                if (dg->cp <= 32) dg->osize.x = dx;

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

}  // namespace ara