
//

#pragma once

#include <GlbCommon/GlbCommon.h>

namespace ara {

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

    [[nodiscard]] float getRightLimit() const { return (opos.x + osize.y) / m_pixRatio; }  ///>  return in hw pixels
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

    [[nodiscard]] float getYSelRange(int idx) const { return yselrange[idx] / m_pixRatio; }  ///>  return in hw pixels
};

struct e_fontword {
    e_fontdglyph *ptr[2]{nullptr};
    float         width = 0.f;
    int           chidx[2]{0};
};

class Font;

class FontGlyphVector {
public:
    virtual ~FontGlyphVector() = default;

    bool Process(Font *font, const glm::vec2 &size, const glm::vec2 &sep, align text_align_x, const std::string &str,
                 bool word_wrap);  // text_align : see e_fontalign

    unsigned        calculateBoundingBoxHwp(glm::vec4 &bb);  // bb vec4 / [x1,y1,x2,y2]
    unsigned        calculateBoundingBox(glm::vec4 &bb);     // bb vec4 / [x1,y1,x2,y2]
    glm::vec4       calculateBoundingBoxHwp();
    glm::vec2       getPixSize();  ///> in virtual pixels
    glm::vec2       getPixSizeHwp();
    void            reset(const Font *font);
    e_fontdglyph    findByCharIndex(int idx);
    int             getLineIndexByPixPos(float pix_x, float pix_y, float off_x, float off_y);      // returns -1 : before, -2 : beyond
    int             getLineIndexByCharIndex(int ch_index);  // returns -1 if not found
    int             getCharIndexByPixPos(float pix_x, float pix_y, float off_x, float off_y, int &off_bound);  // off_bound==-1 : before, off_bound=1 beyond
    glm::vec2      &getCaretPos(glm::vec2 &pos, int caret_index);

    int jumpToLine(int caret_index, int line_delta);// returns new caret position, on error returns caret_index
    int jumpToBeginOfLine(int caret_index);         // returns new caret position, on error returns caret_index
    int jumpToEndOfLine(int caret_index);           // returns new caret position, on error returns caret_index

    [[nodiscard]] size_t size() const { return v.size(); }
    e_fontdglyph &operator[](size_t index) { return v[index]; }

    std::vector<e_fontdglyph> v;
    std::vector<e_fontline>   vline;

    void  setTabPixSize(float ts) { m_TabSize = ts * m_pixRatio; }
    void  setPixRatio(float pixRatio) { m_pixRatio = pixRatio; }
    float getRightLimit() { return (v.back().opos[0] + v.back().osize[0]) / m_pixRatio; }

    static int codepoint(const std::string& u);

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
};

}
