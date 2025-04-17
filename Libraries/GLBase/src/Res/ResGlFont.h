#pragma once

#include <GLBase.h>
#include <Utils/FBO.h>

namespace ara {

class Font;
class Shaders;

struct e_fontglyph {
    int            codepoint = 0;
    unsigned short ppos[2][2]{0};
    glm::vec2      off{0.f};
    float          yoff2 = 0.f;
    float          xadv  = 0.f;
    glm::vec2      outpixsize{0.f};
    glm::vec2      srcpixpos{0.f};
    glm::vec2      srcpixsize{0.f};
};

class e_fontdglyph {
public:
    unsigned     cp = 0;
    glm::vec2    opos{0.f};
    glm::vec2    osize{0.f};
    e_fontglyph *gptr       = nullptr;
    int          chidx      = 0;  // character position in source string
    float        m_pixRatio = 0.f;

    float getRightLimit() const { return (opos.x + osize.y) / m_pixRatio; }  ///>  return in hw pixels
};

class e_fontline {
public:
    e_fontdglyph *ptr[2]{nullptr};  /// pointer to the first and last glyph in the line
    float         y     = 0.f;
    float         width = 0.f;
    int           chidx[2]{0};
    float         yrange[2]{0.f};
    float         yselrange[2]{0.f};
    float         m_pixRatio = 1.f;

    float getYSelRange(int idx) { return yselrange[idx] / m_pixRatio; }  ///>  return in hw pixels
};

struct e_fontword {
    e_fontdglyph *ptr[2]{nullptr};
    float         width = 0.f;
    int           chidx[2]{0};
};

enum fontalign { center = 0, left, right, justify, justify_ex };
enum fontvalign { vcenter = 0, top, bottom };

class FontGlyphVector {
public:
    virtual ~FontGlyphVector() = default;

    bool Process(Font *font, glm::vec2 &size, glm::vec2 &sep, align text_align_x, const std::string &str,
                 bool word_wrap);  // text_align : see e_fontalign

    unsigned  calculateBoundingBoxHwp(glm::vec4 &bb);  // bb vec4 / [x1,y1,x2,y2]
    unsigned  calculateBoundingBox(glm::vec4 &bb);     // bb vec4 / [x1,y1,x2,y2]
    glm::vec4 calculateBoundingBoxHwp();

    glm::vec2 getPixSize();  ///> in virtual pixels
    glm::vec2 getPixSizeHwp();

    void         reset(Font *font);
    e_fontdglyph findByCharIndex(int idx);

    int        getLineIndexByPixPos(float pix_x, float pix_y, float off_x,
                                    float off_y);      // returns -1 : before, -2 : beyond
    int        getLineIndexByCharIndex(int ch_index);  // returns -1 if not found
    int        getCharIndexByPixPos(float pix_x, float pix_y, float off_x, float off_y,
                                    int &off_bound);  // off_bound==-1 : before, off_bound=1 beyond
    glm::vec2 &getCaretPos(glm::vec2 &pos, int caret_index);

    int jumpToLine(int caret_index,
                   int line_delta);          // returns new caret position, on error
                                             // returns caret_index
    int jumpToBeginOfLine(int caret_index);  // returns new caret position, on
                                             // error returns caret_index
    int jumpToEndOfLine(int caret_index);    // returns new caret position, on
                                             // error returns caret_index

    [[nodiscard]] size_t size() const { return v.size(); }
    e_fontdglyph        &operator[](size_t index) { return v[index]; }

    std::vector<e_fontdglyph> v;
    std::vector<e_fontline>   vline;

    void  setTabPixSize(float ts) { m_TabSize = ts * m_pixRatio; }
    void  setPixRatio(float pixRatio) { m_pixRatio = pixRatio; }
    float getRightLimit() { return (v.back().opos[0] + v.back().osize[0]) / m_pixRatio; }

    static int codepoint(std::string u) {
        int l = (int)u.length();
        if (l < 1) {
            return -1;
        }
        unsigned char u0 = u[0];
        if (u0 >= 0 && u0 <= 127) return u0;
        if (l < 2) {
            return -1;
        }
        unsigned char u1 = u[1];
        if (u0 >= 192 && u0 <= 223) return (u0 - 192) * 64 + (u1 - 128);
        if ((unsigned char)u[0] == 0xed && (u[1] & 0xa0) == 0xa0) {
            return -1;
        }  // code points, 0xd800 to 0xdfff
        if (l < 3) {
            return -1;
        }
        unsigned char u2 = u[2];
        if (u0 >= 224 && u0 <= 239) return (u0 - 224) * 4096 + (u1 - 128) * 64 + (u2 - 128);
        if (l < 4) {
            return -1;
        }
        unsigned char u3 = u[3];
        if (u0 >= 240 && u0 <= 247) return (u0 - 240) * 262144 + (u1 - 128) * 4096 + (u2 - 128) * 64 + (u3 - 128);
        return -1;
    }

private:
    float     m_PixVMetrics[3] = {0, 0, 0};  // [0]:ascent,[1]:descent,[2]:lineGap
    float     m_PixLineHeight  = 0;
    float     m_TabSize        = 100.f;
    float     m_pixRatio       = 1.f;
    glm::vec4 m_BB{0.f};
    glm::vec2 m_Sep{0.f};
    glm::vec2 m_sc{1.f, 1.f};
    glm::vec2 m_tCaretPos{0.f};

    e_fontline *AddLine(e_fontdglyph *p_begin, e_fontdglyph *p_end, float y, std::vector<e_fontword> &fword,
                        align text_align_x, float width, bool eol);

    // temporary local variables
    int                     ci   = 0;
    int                     lidx = 0;
    std::string             eorg;
    uint8_t                 ch, lch = 0;
    glm::vec2               p{0.f};
    glm::vec2               l_sep{0.f};
    glm::vec2               l_size{0.f};
    e_fontglyph            *g  = nullptr;
    e_fontdglyph           *ve = nullptr, *vb = nullptr, *lastve = nullptr;
    e_fontdglyph           *ws = nullptr;
    e_fontdglyph           *linep[2]{nullptr};
    e_fontdglyph           *vorig = nullptr;
    std::vector<e_fontword> fword;
};

class Font {
public:
    Font() = default;

    Font(std::string font_path, int size, float pixRatio);

    Font(std::vector<uint8_t> &vp, const std::string &font_path, int size, float pixRatio);

    virtual ~Font() = default;

    bool Create(const std::string &font_path, int size, float pixRatio);
    bool Create(std::vector<uint8_t> &vp, const std::string &font_path, int size, float pixRatio);

    int drawDGlyphs(FontGlyphVector &dgv, glm::mat4 *mvp, Shaders *shdr, GLuint vao, float *tcolor, float off_x,
                    float off_y, float mask_x, float mask_y, float mask_w, float mask_h);

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
        m_layerTexLayerIdRel = layerSize <= 1 ? 0.f : (float)m_layerTexId / (float)(layerSize - 1);
    }

    bool isFontType(std::string &fonttype, int size, float pixRatio) {
        return m_FontType == fonttype && m_FontSize == size && m_pixRatio == pixRatio;
    }

    bool                isOK() { return !m_Glyphs.empty(); }
    int                 getGlyphTexSize() { return m_GlyphTexSize[0]; }
    [[nodiscard]] float getPixHeight() const { return (float)m_FontSize; }                 ///> return in virtual pixels
    float getPixAscent() { return (float)m_FontVMetrics[0] * m_FontScale / m_pixRatio; }   ///> return in virtual pixels
    float getPixDescent() { return (float)m_FontVMetrics[1] * m_FontScale / m_pixRatio; }  ///> return in virtual pixels
    float getPixLineGap() { return (float)m_FontVMetrics[2] * m_FontScale / m_pixRatio; }  ///> return in virtual pixels
    [[nodiscard]] float getScale() const { return m_FontScale / m_pixRatio; }              ///> return in virtual pixels
    float               getPixRatio() const { return m_pixRatio; }
    [[nodiscard]] float getPixHeightHwp() const { return m_hwFontSize; }                       ///> return in hw pixels
    float               getPixAscentHwp() { return (float)m_FontVMetrics[0] * m_FontScale; }   ///> return in hw pixels
    float               getPixDescentHwp() { return (float)m_FontVMetrics[1] * m_FontScale; }  ///> return in hw pixels
    float               getPixLineGapHwp() { return (float)m_FontVMetrics[2] * m_FontScale; }  ///> return in hw pixels
    [[nodiscard]] float getScaleHwp() const { return m_FontScale; }                            ///> return in hw pixels
    GLuint              getTexId() { return gl_TexID; }
    GLuint              getLayerTexId() { return m_layerTexId; }
    GLuint              getLayerTexLayerId() { return m_layerTexLayerId; }
    float               getLayerTexLayerIdRel() { return m_layerTexLayerIdRel; }
    GLuint              getLayerTexSize() { return m_layerTexSize; }

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

class FontList {
public:
    Font *get(std::string font_path, int size, float pixRatio);
    Font *find(std::string &font_path, int size, float pixRatio);
    Font *add(std::string font_path, int size, float pixRatio);
    Font *addFromFilePath(std::string font_path, int size, float pixRatio);
    Font *add(std::vector<uint8_t> &vp, std::string font_path, int size, float pixRatio);
    void  update3DLayers();

    void  setGlbase(GLBase *glbase) { m_glbase = glbase; }
    int   getCount() { return static_cast<int>(m_FontList.size()); }
    Font *get(int index) { return (index < 0 || index >= getCount()) ? nullptr : m_FontList[index].get(); }
    void  clear() { m_FontList.clear(); }

private:
    std::vector<std::unique_ptr<Font>>                m_FontList;
    std::unordered_map<int, std::unique_ptr<Texture>> m_fontTexLayers;
    std::unordered_map<int, std::list<Font *>>        m_layerCount;
    FBO                                               m_fbo;
    GLBase                                           *m_glbase = nullptr;
};

class UtfIterator {
public:
    std::string::const_iterator str_iter;
    size_t                      cplen;

    UtfIterator(const std::string::const_iterator str_iter) : str_iter(str_iter) { find_cplen(); }
    std::string operator*() const { return std::string(str_iter, str_iter + cplen); }

    UtfIterator &operator++() {
        str_iter += cplen;
        find_cplen();
        return *this;
    }

    bool operator!=(const UtfIterator &o) const { return this->str_iter != o.str_iter; }

private:
    void find_cplen() {
        cplen = 1;
        if ((*str_iter & 0xf8) == 0xf0)
            cplen = 4;
        else if ((*str_iter & 0xf0) == 0xe0)
            cplen = 3;
        else if ((*str_iter & 0xe0) == 0xc0)
            cplen = 2;
        // if(iter + cplen > text.length()) cplen = 1;
    }
};

}  // namespace ara