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

#include <Utils/Typo/Font.h>
#include <Shaders/Shaders.h>
#include <string_utils.h>
#include <RwBinFile.h>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <stb_truetype.h>  // http://nothings.org/stb/stb_truetype.h

using namespace glm;
using namespace std;

namespace ara {

Font::Font(std::string font_path, int size, float pixRatio) {
    m_FontSize = size;
    m_pixRatio = pixRatio;
    setFontType(std::move(font_path));
}

Font::Font(vector<uint8_t> &vp, const std::string &font_path, int size, float pixRatio) {
    m_pixRatio = pixRatio;
    create(vp, font_path, size, pixRatio);
}

bool Font::createFromMem(const vector<uint8_t>& vp, const std::string &name, int font_size, float pixRatio) {
    m_pixRatio = pixRatio;
    return create(vp, name, font_size, pixRatio);
}

bool Font::create(const std::string &font_path, int font_size, float pixRatio) {
    m_pixRatio = pixRatio;

    vector<uint8_t> vp;
    if (ReadBinFile(vp, font_path) <= 0) {
        LOGE << "Create return false";
        return false;
    }

    return create(vp, font_path, font_size, pixRatio);
}

bool Font::create(const vector<uint8_t> &vp, const std::string &font_path, int font_size, float pixRatio) {
    ret          = false;
    m_pixRatio   = pixRatio;
    m_hwFontSize = std::floor(static_cast<float>(font_size) * pixRatio);
    m_FontSize   = font_size;

    if (font_size <= 0 || vp.empty()) {
        LOGE << "Font::Create return false";
        return false;
    }

    const auto buff      = &vp[0];
    auto info = stbtt_fontinfo{};

    try {
        rerr = stbtt_InitFont(&info, buff, 0);
    } catch (...) {
        LOGE << "stbtt_InitFont failed";
    }

    if (rerr) {
        int wh     = 0;
        int ch_off = m_CodePoint_Range[0], ch_count = m_CodePoint_Range[1] - m_CodePoint_Range[0] + 1;
        int tret;
        std::vector<stbtt_bakedchar> bchar(ch_count);

        for (int bi = 7; bi <= 12 && !ret; bi++) { // try from 128x128 up to 4096x4096
            wh   = 1 << bi;
            auto cdiv = 1.f / static_cast<float>(wh);
            std::vector<uint8_t> bmp(wh * wh);
            std::fill_n(bchar.begin(), ch_count, stbtt_bakedchar{});

            if ((tret = stbtt_BakeFontBitmap(buff, 0, m_hwFontSize, bmp.data(), wh, wh, ch_off, ch_count, bchar.data())) >= 0) {
                for (int i = 0; i < ch_count; i++) {
                    m_Glyphs.emplace_back(e_fontglyph{
                        i + ch_off,                                                                 // codepoint
                        bchar[i].x0,                                                                // ppos[2][2]
                        bchar[i].y0, bchar[i].x1, bchar[i].y1, vec2{bchar[i].xoff, bchar[i].yoff},  // off
                        bchar[i].yoff2,                                                             // yoff2
                        bchar[i].xadvance, vec2{static_cast<float>(bchar[i].x1 - bchar[i].x0), static_cast<float>(bchar[i].y1 - bchar[i].y0)},
                        vec2{static_cast<float>(bchar[i].x0) * cdiv, static_cast<float>(bchar[i].y0) * cdiv},
                        vec2{static_cast<float>(bchar[i].x1 - bchar[i].x0) * cdiv, static_cast<float>(bchar[i].y1 - bchar[i].y0) * cdiv}});
                }

                pushGlyph(ch_count, ch_off, wh, bmp);

                m_FontScale                           = stbtt_ScaleForPixelHeight(&info, m_hwFontSize);
                stbtt_GetFontVMetrics(&info, &m_FontVMetrics[0], &m_FontVMetrics[1], &m_FontVMetrics[2]);
                setFontType(font_path);
                ret = true;
            }
        }

        if (!ret) {
            LOGE << "[ERROR] NO SUITABLE SIZE FOR FONT";
        }
    }

    return ret;
}

void Font::pushGlyph(int ch_count, int ch_off, int wh, std::vector<uint8_t>& bmp) {
    glGenTextures(1, &gl_TexID);
    glBindTexture(GL_TEXTURE_2D, gl_TexID);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, wh, wh);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, wh, wh, GL_RED, GL_UNSIGNED_BYTE, bmp.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_GlyphTexSize[0] = m_GlyphTexSize[1] = wh;
    m_FontType                            = "";
    m_glCreated                           = true;
}

// all input values are in virtual pixels and must be converted to hw pixels
int Font::drawDGlyphs(FontGlyphVector &dgv, glm::mat4 *mvp, Shaders *shdr, GLuint vao, float *tcolor, float off_x,
                      float off_y, float mask_x, float mask_y, float mask_w, float mask_h) const {
    if (!isOK()) {
        return 0;
    }

    if (tcolor == nullptr) {
        tcolor = def_color;
    }

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
    if (!isOK()) {
        return 0;
    }

    FontGlyphVector dgv;
    glm::vec2       size{1e10, 1e10};
    glm::vec2       pos{0, 0};
    dgv.Process(this, size, pos, align::left, str, true);
    drawDGlyphs(dgv, mvp, shdr, vao, tcolor, x, y + getPixAscent(), 0, 0, 1e10, 1e10);

    return 0;
}

int Font::writeFormat(glm::mat4 *mvp, Shaders *shdr, GLuint vao, float *tcolor, float x, float y, char *f, ...) {
    va_list p;
    va_start(p, f);
    auto se = string_format(f, p);
    va_end(p);

    return write(mvp, shdr, vao, tcolor, x, y, se);
}

void Font::setTexLayer(GLuint texId, GLuint layerId, GLuint layerSize) {
    m_layerTexId         = texId;
    m_layerTexLayerId    = layerId;
    m_layerTexSize       = layerSize;
    m_layerTexLayerIdRel = layerSize <= 1 ? 0.f : static_cast<float>(m_layerTexId) / static_cast<float>(layerSize - 1);
}

e_fontglyph *Font::getGlyph(int cp) {
    return (cp >= m_CodePoint_Range[0] && cp <= m_CodePoint_Range[1]) ? &m_Glyphs.at(cp - m_CodePoint_Range[0])
                                                                      : nullptr;
}

bool Font::isFontType(const std::string &fonttype, int size, float pixRatio) const {
    return m_FontType == fonttype && m_FontSize == size && m_pixRatio == pixRatio;
}

}
