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

#pragma once

#include <Utils/Typo/FontGlyphVector.h>

namespace ara {

class Shaders;

class Font {
public:
    Font() = default;

    Font(const std::string& font_path, int size, float pixRatio);
    Font(const std::vector<uint8_t> &vp, const std::string &font_path, int size, float pixRatio);

    virtual ~Font() = default;

    bool createFromMem(const std::vector<uint8_t>& vp, const std::string &name, int font_size, float pixRatio);
    bool create(const std::string &font_path, int size, float pixRatio);
    bool create(const std::vector<uint8_t> &vp, const std::string &font_path, int size, float pixRatio);

    static glm::ivec2 getOptimalAtlasPixSize(const int32_t& ch_off, const int32_t& ch_count, const uint8_t* buff, float fontSize);
    void pushGlyphAtlas(uint32_t wh, const std::vector<uint8_t>& bmp);
    int drawDGlyphs(FontGlyphVector &dgv, glm::mat4 *mvp, Shaders *shdr, GLuint vao, float *tcolor, glm::vec2 off,
                    glm::vec2 maskPos, glm::vec2 maskSize) const;

    int write(glm::mat4 *mvp, Shaders *shdr, GLuint vao, float *tcolor, float x, float y, const std::string &str);
    int writeFormat(glm::mat4 *mvp, Shaders *shdr, GLuint vao, float *tcolor, float x, float y, char *f, ...);

    void callChangedCb() {
        for (auto &cb : m_changedCb | std::views::values) {
            cb();
        }
    }

    void setOversampling(int32_t val) { m_overSampling = static_cast<float>(val); }
    void setFontType(std::string fonttype) { m_fontType = std::move(fonttype); }
    void setTexLayer(GLuint texId, GLuint layerId, GLuint layerSize);
    void setLayertTexChangedCb(void* id, const std::function<void()>& cb) { m_changedCb[id] = cb; }

    [[nodiscard]] bool      isFontType(const std::string &fonttype, int size, float pixRatio) const;
    [[nodiscard]] bool      isOK() const { return !m_glyphs.empty(); }
    [[nodiscard]] auto&     getGlyphTexSize() const { return m_glyphTexSize; }
    [[nodiscard]] float     getPixHeight() const { return static_cast<float>(m_fontSize); }                 ///> return in virtual pixels
    [[nodiscard]] float     getPixAscent() const { return m_fontVMetrics.ascent / m_pixRatio; }   ///> return in virtual pixels
    [[nodiscard]] float     getPixDescent() const { return m_fontVMetrics.descent / m_pixRatio; }  ///> return in virtual pixels
    [[nodiscard]] float     getPixLineGap() const { return m_fontVMetrics.lineGap / m_pixRatio; }  ///> return in virtual pixels
    [[nodiscard]] float     getScale() const { return m_fontScale / m_pixRatio; }              ///> return in virtual pixels
    [[nodiscard]] float     getPixRatio() const { return m_pixRatio; }
    [[nodiscard]] float     getPixHeightHwp() const { return m_hwFontSize; }                       ///> return in hw pixels
    [[nodiscard]] float     getPixAscentHwp() const { return m_fontVMetrics.ascent; }   ///> return in hw pixels
    [[nodiscard]] float     getPixDescentHwp() const { return m_fontVMetrics.descent; }  ///> return in hw pixels
    [[nodiscard]] float     getPixLineGapHwp() const { return m_fontVMetrics.lineGap; }  ///> return in hw pixels
    [[nodiscard]] float     getScaleHwp() const { return m_fontScale; }                            ///> return in hw pixels
    [[nodiscard]] GLuint    getTexId() const { return m_glTexId; }
    [[nodiscard]] GLuint    getLayerTexId() const { return m_layerTexId; }
    [[nodiscard]] GLuint    getLayerTexLayerId() const { return m_layerTexLayerId; }
    [[nodiscard]] float     getLayerTexLayerIdRel() const { return m_layerTexLayerIdRel; }
    [[nodiscard]] GLuint    getLayerTexSize() const { return m_layerTexSize; }
    [[nodiscard]] float     getOverSampling() const { return m_overSampling; }
    Fontglyph*              getGlyph(int cp);

private:
    std::vector<Fontglyph> m_glyphs;
    Shaders                 *m_glyphShader        = nullptr;
    GLuint                   m_glTexId             = 0;
    float                    m_fontScale          = 0.f;        ///> in hw pixels
    float                    m_pixRatio           = 1.f;        ///> fontSize up or downscaling, display dpi dependent
    //int                      m_fontVMetrics[3]    = {0, 0, 0};  ///> [0]:ascent,[1]:descent,[2]:lineGap, in hw pixels
    AtlasPar                 m_fontVMetrics{};
    int                      m_codepointRange[2] = {32, 255};
    glm::uvec2               m_glyphTexSize{};
    int                      m_texLayerInd = 0;    ///> Layer index into the FontLists 3d texture containing this font
    int                      m_fontSize    = 0;    ///> font size, virtual pixels
    float                    m_hwFontSize  = 0.f;  ///> font size, hw pixels
    std::string              m_fontType;
    bool                     m_glCreated = false;
    static inline float      def_color[4]{1, 1, 1, 1};
    GLuint                   m_layerTexId         = 0;
    GLuint                   m_layerTexLayerId    = 0;
    GLuint                   m_layerTexSize       = 0;
    float                    m_layerTexLayerIdRel = 0;
    float                    m_overSampling = 2.f;

    std::unordered_map<void*, std::function<void()>>    m_changedCb;

    // temporary local variables, made member variables for performance reasons
    bool           ret  = false;
    int            rerr = 0;
};

}
