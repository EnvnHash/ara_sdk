#pragma once

#include <GlbCommon/GlbCommon.h>

// in build settings preprocessor flags: STB_TRUETYPE_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC

#include <stb_truetype.h>  // http://nothings.org/stb/stb_truetype.h

namespace ara {

class Shaders;
class ShaderCollector;
class Quad;

class BakedFont {
public:
    GLuint                           tex = 0;
    float                            scale = 0.f;
    int                              ascent  = 0;
    int                              descent = 0;
    int                              lineGap = 0;
    std::vector<uint8_t>             bitmap;
    std::array<stbtt_bakedchar, 223> cdata{};
};

class TypoGlyphMap {
public:
    TypoGlyphMap(uint32_t screenWidth, uint32_t screenHeight);
    ~TypoGlyphMap() = default;

    void initShader();
    void loadFont(const char *_file, ShaderCollector *_shCol);
    void bakeFont(int fontSize);

    // prints in opengl standard normalized coordinates (0,0 center,
    // left/bottom: [-1,-1], right/top: [1,1]
    void print(float x, float y, const std::string &&text, int fontSize, float *col);
    void print(float x, float y, const std::string &text, int fontSize, float *col);

    // prints in pixel coordinates, origin is left/top
    void      print(int _x, int _y, const std::string &text, int fontSize, float *col);
    void      print(const glm::vec2 &p, const std::string &text, int fontSize, float *col) {
        print(p.x, p.y, text, fontSize, col);
    }
    glm::vec2 getTextWidth(const char *text, int fontSize);
    glm::ivec2 getPixTextWidth(const char *text, int fontSize,
                               int max_count = -1);  // Text width for both x,y in pixels, maxcount==-1 is entire string
    int        m_print(glm::vec2 offset, const char *text, int font_size, float *col, float *pvm, float x_range_lo,
                       float x_range_hi);
    glm::ivec2 m_getPixExtent(const char *text, int font_size, int max_count = -1, glm::mat2 *box = nullptr,
                              std::vector<float> *ppos = nullptr);  // Text width for both x,y in pixels, maxcount==-1 is entire string
    void         setScreenSize(uint32_t screenWidth, uint32_t screenHeight);
    unsigned int getScreenWidth() const { return m_screen_width; }
    unsigned int getScreenHeight() const { return m_screen_height; }
    float        getRelativeLineHeight() const { return static_cast<float>(m_heightPix) / static_cast<float>(m_screen_height); };
    int32_t      getDescent(int fontSize) { return m_bakedFonts[fontSize].descent; }

    std::vector<glm::vec2>           m_charOffs;
    std::vector<glm::vec2>::iterator m_charOffsIt;

private:
    stbtt_fontinfo     m_info = {nullptr};
    stbtt_aligned_quad q{};
    std::ifstream      m_fontFile;

    std::unordered_map<int, BakedFont> m_bakedFonts;
    std::vector<char>                  m_fontBuffer;

    bool m_shaderInited = false;

    int m_heightPix = 0;

    unsigned int m_screen_width  = 0;
    unsigned int m_screen_height = 0;
    unsigned int m_tex_width     = 0;
    unsigned int m_tex_height    = 0;

    glm::vec2 m_iTexSize{};  // 1/texture size

    float m_scale = 0.f;

    std::unique_ptr<Quad> m_quad;

    Shaders         *m_stdCol         = nullptr;
    Shaders         *m_typoShader     = nullptr;
    Shaders         *m_typoShader_m   = nullptr;
    Shaders         *m_typoInstShader = nullptr;
    ShaderCollector *m_shCol          = nullptr;

    glm::mat4 m_othoProj = glm::mat4(1.f);
    glm::mat4 m_scaleMat = glm::mat4(1.f);

    glm::vec2 m_tex_offs{0.f};
    glm::vec2 m_tex_scale{0.f};

    GLuint m_nullVao = 0;
};

}  // namespace ara
