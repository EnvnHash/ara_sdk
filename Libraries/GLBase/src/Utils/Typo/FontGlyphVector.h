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

#include <GlbCommon/GlbCommon.h>

namespace ara {

struct AtlasPar {
    float ascent{}; // ascent is the coordinate above the baseline the font extends;
    float descent{}; // descent is the coordinate below the baseline the font extends (i.e. it is typically negative)
    float lineGap{}; // lineGap is the spacing between one row's descent and the next row's ascent...
};
struct Fontglyph {
    int                         charAsCodepoint = 0;
    std::array<glm::vec2, 2>    ppos{};    // font atlas bounding box coordinates in pixel for this glyph
    std::array<glm::vec2, 2>    off{};  // packed Font atlases: left, top, right, bottom baseline offset
                                        // for standard Font atlases left, top, [invalid], bottom baseline offset
    float          xadv  = 0.f;     // horizontal advance
    glm::vec2      outpixsize{0.f};
    glm::vec2      srcpixpos{0.f};
    glm::vec2      srcpixsize{0.f};

    std::array<glm::vec2, 2>    tuv{};
};

class Fontdglyph {
public:
    unsigned     charAsCodepoint = 0;
    glm::vec2    pos{0.f};
    glm::vec2    size{0.f};
    Fontglyph *glyphPtr   = nullptr;
    glm::vec4   *color      = nullptr;
    int32_t      characterIdx      = 0;  // character position in source string
    float        pixRatio = 0.f;

    [[nodiscard]] float getRightLimit() const { return (pos.x + size.y) / pixRatio; }  ///>  return in hw pixels
};

class Fontline {
public:
    std::array<Fontdglyph*, 2>    ptr{nullptr};  /// pointer to the first and last glyph in the line
    float                           y     = 0.f;
    float                           width = 0.f;
    std::array<int32_t, 2>          characterIdx{};
    std::array<float, 2>            yrange{};
    std::array<float, 2>            yselrange{};
    float                           pixRatio = 1.f;

    [[nodiscard]] float getYSelRange(int idx) const { return yselrange[idx] / pixRatio; }  ///>  return in hw pixels
};

struct Fontword {
    std::array<Fontdglyph*, 2>    ptr{nullptr};
    float                           width = 0.f;
    std::array<int32_t, 2>          characterIdx{};
};

struct procPar {
    glm::vec2                       pos{};
    Fontdglyph                    *ws = nullptr;
    Fontdglyph                    *ve = nullptr;
    std::array<Fontdglyph*, 2>    linep{nullptr, nullptr};
    uint8_t                         charAsCodepoint = 0;
    uint8_t                         lch = 0;
    float                           spheight = 0;
    std::string                     text;
    std::string::iterator           textIt;
    std::vector<Fontword>         fword;
    float                           lineheight = 0;
    align                           text_align_x{};
    glm::vec2                       sep{};
    glm::vec2                       l_sep{};
    glm::vec2                       l_size{};

    std::vector<std::pair<glm::ivec2, glm::vec4>>::iterator textColIt{};
};

class Font;

class FontGlyphVector {
public:
    virtual ~FontGlyphVector() = default;

    bool process(Font *font, const glm::vec2 &size, const glm::vec2 &sep, align text_align_x, const std::string &str,
                 bool word_wrap);  // text_align : see e_fontalign

    static void     glyphPtrCheck(procPar& par);
    Fontdglyph    addDGlyph(procPar &par, const glm::vec2& offs, const glm::vec2& size, Fontglyph* g=nullptr) const;
    glm::vec4*      getCharColor(procPar& par) const;
    void            procTab(procPar& p) const;
    void            procCrAndNl(procPar& par);
    void            procSpace(procPar& par, Font* font) const;
    void            procChar(procPar& par, Font* font, bool word_wrap);

    unsigned        calculateBoundingBoxHwp(glm::vec4 &bb) const;  // bb vec4 / [x1,y1,x2,y2]
    unsigned        calculateBoundingBox(glm::vec4 &bb) const;     // bb vec4 / [x1,y1,x2,y2]
    glm::vec4       calculateBoundingBoxHwp() const;
    glm::vec2       getPixSize() const;  ///> in virtual pixels
    glm::vec2       getPixSizeHwp() const;
    void            reset(const Font *font);
    Fontdglyph    findByCharIndex(int idx) const;
    int             getLineIndexByPixPos(float pix_x, float pix_y, float off_x, float off_y);      // returns -1 : before, -2 : beyond
    int             getLineIndexByCharIndex(int ch_index) const;  // returns -1 if not found
    int             getCharIndexByPixPos(float pix_x, float pix_y, float off_x, float off_y, int &off_bound);  // off_bound==-1 : before, off_bound=1 beyond
    glm::vec2       getCaretPos(int caret_index) const;

    int jumpToLine(int caret_index, int line_delta);// returns new caret position, on error returns caret_index
    int jumpToBeginOfLine(int caret_index) const;         // returns new caret position, on error returns caret_index
    int jumpToEndOfLine(int caret_index) const;           // returns new caret position, on error returns caret_index

    auto& getGlyphs() { return m_glyphs; }
    [[nodiscard]] size_t size() const { return m_glyphs.size(); }
    Fontdglyph &operator[](size_t index) { return m_glyphs[index]; }

    auto& getFontLines() { return m_vline; }
    auto& getFontLines(size_t idx) { return m_vline[idx]; }
    float getRightLimit() { return (m_glyphs.back().pos[0] + m_glyphs.back().size[0]) / m_pixRatio; }
    auto& getTextColors() { return m_textColors; }
    auto& getLastTextColor() { return m_textColors.back(); }
    void  setTabPixSize(float ts) { m_tabSize = ts * m_pixRatio; }
    void  setPixRatio(float pixRatio) { m_pixRatio = pixRatio; }
    void clearTextColors() { m_textColors.clear(); }
    void addTextColor(const glm::ivec2& range, const glm::vec4& color) { m_textColors.emplace_back(std::make_pair(range, color)); }

    static int codepoint(const std::string& u);

private:
    Fontline *addLine(const procPar& par, bool eol, int lineEndOffs=0);

    std::vector<Fontdglyph> m_glyphs;
    std::vector<Fontline>   m_vline;

    float     m_pixVMetrics[3] = {0, 0, 0};  // [0]:ascent,[1]:descent,[2]:lineGap
    float     m_pixLineHeight  = 0;
    float     m_tabSize        = 100.f;
    float     m_pixRatio       = 1.f;
    glm::vec4 m_bb{0.f};
    glm::vec2 m_tCaretPos{0.f};

    std::vector<std::pair<glm::ivec2, glm::vec4>> m_textColors{};
};

}
