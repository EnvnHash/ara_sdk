
//

#pragma once

#include <Res/FontGlyphVector.h>

namespace ara {

class Shaders;

class Font {
public:
    Font() = default;
    Font(std::string font_path, int size, float pixRatio);
    Font(std::vector<uint8_t> &vp, const std::string &font_path, int size, float pixRatio);

    virtual ~Font() = default;

    bool Create(const std::string &font_path, int size, float pixRatio);
    bool Create(std::vector<uint8_t> &vp, const std::string &font_path, int size, float pixRatio);

    int drawDGlyphs(FontGlyphVector &dgv, glm::mat4 *mvp, Shaders *shdr, GLuint vao, float *tcolor, float off_x,
                    float off_y, float mask_x, float mask_y, float mask_w, float mask_h) const;

    int write(glm::mat4 *mvp, Shaders *shdr, GLuint vao, float *tcolor, float x, float y, const std::string &str);
    int writeFormat(glm::mat4 *mvp, Shaders *shdr, GLuint vao, float *tcolor, float x, float y, char *f, ...);

    e_fontglyph *getGlyph(int cp) {
        return (cp >= m_CodePoint_Range[0] && cp <= m_CodePoint_Range[1]) ? &m_Glyphs.at(cp - m_CodePoint_Range[0])
                                                                          : nullptr;
    }

    void setFontType(std::string fonttype) { m_FontType = std::move(fonttype); }

    void setTexLayer(GLuint texId, GLuint layerId, GLuint layerSize) {
        m_layerTexId         = texId;
        m_layerTexLayerId    = layerId;
        m_layerTexSize       = layerSize;
        m_layerTexLayerIdRel = layerSize <= 1 ? 0.f : static_cast<float>(m_layerTexId) / static_cast<float>(layerSize - 1);
    }

    bool isFontType(const std::string &fonttype, int size, float pixRatio) const {
        return m_FontType == fonttype && m_FontSize == size && m_pixRatio == pixRatio;
    }

    bool                isOK() const { return !m_Glyphs.empty(); }
    int                 getGlyphTexSize() const { return m_GlyphTexSize[0]; }
    [[nodiscard]] float getPixHeight() const { return static_cast<float>(m_FontSize); }                 ///> return in virtual pixels
    float               getPixAscent() const { return static_cast<float>(m_FontVMetrics[0]) * m_FontScale / m_pixRatio; }   ///> return in virtual pixels
    float               getPixDescent() const { return static_cast<float>(m_FontVMetrics[1]) * m_FontScale / m_pixRatio; }  ///> return in virtual pixels
    float               getPixLineGap() const { return static_cast<float>(m_FontVMetrics[2]) * m_FontScale / m_pixRatio; }  ///> return in virtual pixels
    [[nodiscard]] float getScale() const { return m_FontScale / m_pixRatio; }              ///> return in virtual pixels
    float               getPixRatio() const { return m_pixRatio; }
    [[nodiscard]] float getPixHeightHwp() const { return m_hwFontSize; }                       ///> return in hw pixels
    float               getPixAscentHwp() const { return static_cast<float>(m_FontVMetrics[0]) * m_FontScale; }   ///> return in hw pixels
    float               getPixDescentHwp() const { return static_cast<float>(m_FontVMetrics[1]) * m_FontScale; }  ///> return in hw pixels
    float               getPixLineGapHwp() const { return static_cast<float>(m_FontVMetrics[2]) * m_FontScale; }  ///> return in hw pixels
    [[nodiscard]] float getScaleHwp() const { return m_FontScale; }                            ///> return in hw pixels
    GLuint              getTexId() const { return gl_TexID; }
    GLuint              getLayerTexId() const { return m_layerTexId; }
    GLuint              getLayerTexLayerId() const { return m_layerTexLayerId; }
    float               getLayerTexLayerIdRel() const { return m_layerTexLayerIdRel; }
    GLuint              getLayerTexSize() const { return m_layerTexSize; }

private:
    std::vector<e_fontglyph> m_Glyphs;
    Shaders                 *m_glyphShader        = nullptr;
    GLuint                   gl_TexID             = 0;
    float                    m_FontScale          = 0.f;        ///> in hw pixels
    float                    m_pixRatio           = 1.f;        ///> fontSize up or downscaling, display dpi dependent
    int                      m_FontVMetrics[3]    = {0, 0, 0};  ///> [0]:ascent,[1]:descent,[2]:lineGap, in hw pixels
    int                      m_CodePoint_Range[2] = {32, 255};
    int                      m_GlyphTexSize[2]    = {0, 0};
    int                      m_texLayerInd = 0;    ///> Layer index into the FontLists 3d texture containing this font
    int                      m_FontSize    = 0;    ///> font size, virtual pixels
    float                    m_hwFontSize  = 0.f;  ///> font size, hw pixels
    std::string              m_FontType;
    bool                     m_glCreated = false;
    static inline float      def_color[4]{1, 1, 1, 1};
    GLuint                   m_layerTexId         = 0;
    GLuint                   m_layerTexLayerId    = 0;
    GLuint                   m_layerTexSize       = 0;
    float                    m_layerTexLayerIdRel = 0;

    // temporary local variables, made member variables for performance reasons
    bool           ret  = false;
    int            rerr = 0;
    unsigned char *buff = nullptr;
};

}
