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

Font::Font(const std::string& font_path, const int size, const float pixRatio) {
    m_fontSize = size;
    m_pixRatio = pixRatio;
    setFontType(font_path);
}

Font::Font(const vector<uint8_t> &vp, const std::string &font_path, const int size, const float pixRatio) {
    m_pixRatio = pixRatio;
    create(vp, font_path, size, pixRatio);
}

bool Font::createFromMem(const vector<uint8_t>& vp, const std::string &name, const int font_size,
                         const float pixRatio) {
    m_pixRatio = pixRatio;
    return create(vp, name, font_size, pixRatio);
}

bool Font::create(const std::string &font_path, const int size, const float pixRatio) {
    m_pixRatio = pixRatio;
    vector<uint8_t> vp;
    if (ReadBinFile(vp, font_path) <= 0) {
        LOGE << "Create return false";
        return false;
    }

    return create(vp, font_path, size, pixRatio);
}

bool Font::create(const vector<uint8_t> &vp, const std::string &font_path, const int size, const float pixRatio) {
    if (size <= 0 || vp.empty()) {
        LOGE << "Font::Create return false";
        return false;
    }

    const auto buff      = &vp[0];
    auto info = stbtt_fontinfo{};

    try {
        rerr = stbtt_InitFont(&info, buff, 0);
    } catch (...) {
        LOGE << "stbtt_InitFont failed";
        return ret;
    }

    ret          = false;
    m_pixRatio   = pixRatio;
    m_fontSize   = size;
    m_hwFontSize = static_cast<float>(size) * pixRatio;

    if (rerr) {
        const auto ch_off = m_codepointRange[0];
        const auto ch_count = m_codepointRange[1] - m_codepointRange[0] + 1;
        const auto wh     = getOptimalAtlasPixSize(ch_off, ch_count, buff, m_hwFontSize * m_overSampling).x;
        std::vector<uint8_t> bitmap(wh * wh);
        stbtt_pack_context pc; // Declare and initialize the packing context
        if (!stbtt_PackBegin(&pc, bitmap.data(), wh, wh, 0, 1, nullptr)) {
            LOGE << "Failed to initialize font packing.";
            return ret;
        }

        std::vector<stbtt_packedchar> chardata(ch_count);
        stbtt_PackSetOversampling(&pc, static_cast<uint32_t>(m_overSampling), static_cast<uint32_t>(m_overSampling));
        stbtt_PackFontRange(&pc, buff, 0, static_cast<float>(size), ch_off, ch_count, chardata.data());
        stbtt_PackEnd(&pc);

        auto cdiv = 1.f / static_cast<float>(wh);
        for (int i = 0; i < ch_count; i++) {
            m_glyphs.emplace_back(Fontglyph{
                .charAsCodepoint = i + ch_off,
                .ppos = { vec2{chardata[i].x0, chardata[i].y0}, vec2{chardata[i].x1, chardata[i].y1} },
                .off = { vec2{chardata[i].xoff, chardata[i].yoff}, vec2{chardata[i].xoff2, chardata[i].yoff2} },
                .xadv = chardata[i].xadvance,
                .srcpixpos = vec2{static_cast<float>(chardata[i].x0), static_cast<float>(chardata[i].y0)} * cdiv
            });

            m_glyphs.back().outpixsize = m_glyphs.back().off[1] - m_glyphs.back().off[0];
            m_glyphs.back().srcpixsize = m_glyphs.back().outpixsize * cdiv * m_overSampling;
        }

        pushGlyphAtlas(wh, bitmap);

        std::array<int32_t, 3> fontMetrics{};
        stbtt_GetFontVMetrics(&info, &fontMetrics[0], &fontMetrics[1], &fontMetrics[2]);
        m_fontScale = stbtt_ScaleForPixelHeight(&info, m_hwFontSize);
        m_fontVMetrics.ascent = static_cast<float>(fontMetrics[0]) * m_fontScale;
        m_fontVMetrics.descent = static_cast<float>(fontMetrics[1]) * m_fontScale;
        m_fontVMetrics.lineGap = static_cast<float>(fontMetrics[2]) * m_fontScale;

        setFontType(font_path);
        ret = true;
    }

    return ret;
}

ivec2 Font::getOptimalAtlasPixSize(const int32_t& ch_off, const int32_t& ch_count, const uint8_t* buff,
                                   const float fontSize) {
    int wh = 0;
    std::vector<stbtt_bakedchar> bchar(ch_count);

    for (int bi = 7; bi <= 12; bi++) { // try from 128x128 up to 4096x4096
        wh   = 1 << bi;
        std::vector<uint8_t> bmp(wh * wh);

        if (stbtt_BakeFontBitmap(buff, 0, fontSize, bmp.data(), wh, wh, ch_off, ch_count, bchar.data()) >= 0) {
            return {wh, wh};
        }
    }
    return {};
}

void Font::pushGlyphAtlas(const uint32_t wh, const std::vector<uint8_t>& bmp) {
    glGenTextures(1, &m_glTexId);
    glBindTexture(GL_TEXTURE_2D, m_glTexId);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, static_cast<GLsizei>(wh), static_cast<GLsizei>(wh));
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(wh), static_cast<GLsizei>(wh), GL_RED, GL_UNSIGNED_BYTE, bmp.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_glyphTexSize.x = m_glyphTexSize.y = wh;
    m_fontType                            = "";
    m_glCreated                           = true;
}

// all input values are in virtual pixels and must be converted to hw pixels
int Font::drawDGlyphs(FontGlyphVector &dgv, mat4 *mvp, Shaders *shader, const GLuint vao, float *tcolor, const vec2 off,
                      const vec2 maskPos, const vec2 maskSize) const {
    if (!isOK()) {
        return -1;
    }

    if (tcolor == nullptr) {
        tcolor = def_color;
    }

    if (shader) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_glTexId);

        shader->begin();
        shader->setUniform1i("stex", 0);
        shader->setUniformMatrix4fv("mvp", &(*mvp)[0][0]);
        shader->setUniform4fv("tcolor", tcolor);
        shader->setUniform2f("off", off.x * m_pixRatio, off.y * m_pixRatio);
        shader->setUniform2f("mask_pos", maskPos.x * m_pixRatio, maskPos.y * m_pixRatio);
        shader->setUniform2f("mask_size", maskSize.x * m_pixRatio, maskSize.y * m_pixRatio);
        shader->setUniform1f("pixRatio", m_pixRatio);

        for (auto &g : dgv.getGlyphs()) {
            if (g.glyphPtr) {
                shader->setUniform2fv("opos", &g.pos[0]);
                shader->setUniform2fv("osize", &g.size[0]);
                shader->setUniform2fv("tpos", &g.glyphPtr->srcpixpos[0]);
                shader->setUniform2fv("tsize", &g.glyphPtr->srcpixsize[0]);

                glBindVertexArray(vao);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }
        }
    }

    return 0;
}

/** input x and y in virtual pixels */
int Font::write(mat4 *mvp, Shaders *shader, const GLuint vao, float *tcolor, float x, float y, const std::string &str) {
    if (!isOK()) {
        return -1;
    }

    FontGlyphVector dgv;
    constexpr vec2 size{1e10, 1e10};
    constexpr vec2 pos{0, 0};
    dgv.process(this, size, pos, align::left, str, true);
    drawDGlyphs(dgv, mvp, shader, vao, tcolor, {x, y + getPixAscent()}, {0.f, 0.f}, {1e10, 1e10});

    return 0;
}

void Font::setTexLayer(const GlFontPar& par) {
    if (par.texId != m_glFontPar.texId || par.nrLayers != m_glFontPar.nrLayers || par.layerId != m_glFontPar.layerId) {
        const bool isInitialCall = m_glFontPar.texId == 0 && m_glFontPar.nrLayers == 0;
        m_glFontPar = par;
        m_layerTexLayerIdRel = par.nrLayers <= 1 ? 0.f : static_cast<float>(par.texId) / static_cast<float>(par.nrLayers - 1);

        if (!isInitialCall) {
            for (auto &it : m_layerTexChangedCb | views::values) {
                it();
            }
        }
    }
}

Fontglyph *Font::getGlyph(const int cp) {
    return cp >= m_codepointRange[0] && cp <= m_codepointRange[1] ? &m_glyphs.at(cp - m_codepointRange[0]) : nullptr;
}

bool Font::isFontType(const std::string &fontType, const int size, const float pixRatio) const {
    return m_fontType == fontType && m_fontSize == size && m_pixRatio == pixRatio;
}

}
