#include "Utils/Typo/TypoGlyphMap.h"
#include "GeoPrimitives/Quad.h"
#include "Shaders/ShaderCollector.h"
#include "Shaders/Shaders.h"

using namespace glm;
using namespace std;

namespace ara {

TypoGlyphMap::TypoGlyphMap(uint32_t screenWidth, uint32_t screenHeight)
    : m_screen_width(screenWidth),
    m_screen_height(screenHeight),
    m_tex_width(512),
    m_tex_height(512) {
    // orthographic projection matrix, origin top left, right bottom => +x +y
    m_othoProj = ortho(0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight), 0.0f);
    m_iTexSize = vec2(static_cast<float>(1.0 / m_tex_width), static_cast<float>(1.0 / m_tex_height));
}

void TypoGlyphMap::initShader() {
#ifndef __EMSCRIPTEN__
    std::string vert =
        STRINGIFY(layout(location = 0) in vec4 position;\n
            layout(location = 2) in vec2 texCoord;\n
            uniform mat4 m_pvm;\n
            uniform vec2 scale;\n
            uniform vec2 trans;\n
            out vec2 tex_coord;\n
            const vec2[4] vertices = vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0));\n
            void main() { \n
                tex_coord   = vec2(vertices[gl_VertexID].x, 1.0 - vertices[gl_VertexID].y); \n
                gl_Position = m_pvm * vec4(vertices[gl_VertexID] * scale + trans, 0.0, 1.0); \n
            });

    vert = m_shCol->getShaderHeader() + "// basic texture shader, vert\n" + vert;

    std::string frag = STRINGIFY(
        uniform sampler2D tex;\n
        uniform vec2 tex_scale;\n
        uniform vec2 tex_offs;\n
        uniform vec4 color;\n
        in vec2 tex_coord;\n
        layout(location = 0) out vec4 fragColor;\n
        void main() { \n
            vec2 t_tex_coord = tex_coord * tex_scale + tex_offs;
            float   c        = texture(tex, t_tex_coord).r;\n
            fragColor        = vec4(color.rgb, color.a * c); \n
        });

    frag = m_shCol->getShaderHeader() + "// basic texture shader, frag\n" + frag;

#else
    std::string vert = STRINGIFY(precision mediump float; attribute vec4 position; attribute vec2 texCoord;
                                 uniform mat4 m_pvm; varying vec2 tex_coord; void main() {
                                     tex_coord   = texCoord;
                                     gl_Position = m_pvm * position;
                                 });

    std::string frag =
        STRINGIFY(precision mediump float; varying vec2 tex_coord; uniform sampler2D tex; vec4 outCol; void main() {
            outCol = texture2D(tex, tex_coord);
            if (outCol.a < 0.01) {
                discard;
            } else {
                gl_FragColor = outCol;
            }
        });
#endif
    m_typoShader = m_shCol->add("TypoGlyphMap_draw", vert, frag);

    vert = STRINGIFY(
        uniform mat4 m_pvm;\n uniform vec2 pix_off = vec2(0, 0);\n uniform vec2 pix_size = vec2(0, 0);\n out vec2 tex_coord;\n out vec4 t_pos;\n void
            main() {
                \n const vec2[4] vertices = vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0));
                \n vec2  v                = vertices[gl_VertexID];
                \n       tex_coord        = v;
                \n       t_pos            = vec4(pix_off.x + v.x * pix_size.x, pix_off.y + v.y * pix_size.y, 0, 1);
                \n       gl_Position      = m_pvm * t_pos;
                \n
            });

    vert = "// TypoGlyphMap_m_draw, vert\n" + m_shCol->getShaderHeader() + vert;

    frag = STRINGIFY(
        uniform sampler2D tex;\n
        uniform vec2 tex_size;\n
        uniform vec2 tex_off;\n
        uniform vec4 color;\n
        uniform vec2 xrange = vec2(0, 0);\n
        in vec2 tex_coord;\n
        in vec4 t_pos;\n
        layout(location = 0) out vec4 glFragColor;\n
        void main() {\n
            vec2 t_tex_coord = tex_coord * tex_size + tex_off;
            if (t_pos.x < xrange[0] || t_pos.x > xrange[1]) discard;
            float c = texture(tex, t_tex_coord).r;\n
            glFragColor = vec4(color.rgb, color.a * c);\n
        });

    frag           = "// TypoGlyphMap_m_draw, frag\n" + m_shCol->getShaderHeader() + frag;
    m_typoShader_m = m_shCol->add("TypoGlyphMap_m_draw", vert, frag);

    m_shaderInited = true;
}

void TypoGlyphMap::loadFont(const char *_file, ShaderCollector *_shCol) {
    m_shCol = _shCol;

    if (filesystem::exists(filesystem::path(string(_file)))) {
        try {
            m_fontFile = ifstream(_file, ios_base::in | ios_base::binary);
            m_fontFile.seekg(0, std::ios::end);
            m_fontBuffer.resize(m_fontFile.tellg());
            m_fontFile.seekg(0);
            m_fontFile.read(&m_fontBuffer[0], m_fontBuffer.size());

            // prepare font
            if (!stbtt_InitFont(&m_info, reinterpret_cast<unsigned char *>(&m_fontBuffer[0]), 0))
                throw runtime_error("TypoGlyphMap::loadFont> failed");
        } catch (runtime_error &e) {
            LOGE << "could not load font: " << e.what() << endl;
        }
    } else
        LOGE << "TypoGlyphMap::loadFont> Couldn't open: (" << _file << ") - file does not exist";
}

void TypoGlyphMap::bakeFont(int fontSize) {
    // bake only if not already baked
    if (m_bakedFonts.contains(fontSize) || !m_info.data) {
        return;
    }

    m_bakedFonts[fontSize] = BakedFont();
    m_bakedFonts[fontSize].bitmap.resize(m_tex_width * m_tex_height);
    m_bakedFonts[fontSize].scale = stbtt_ScaleForPixelHeight(&m_info, static_cast<float>(fontSize));
    stbtt_GetFontVMetrics(&m_info,
        &m_bakedFonts[fontSize].ascent,
        &m_bakedFonts[fontSize].descent,
        &m_bakedFonts[fontSize].lineGap);

    // apply the scalefactor to these values to get screen coordinates
    float scaleFact                = stbtt_ScaleForPixelHeight(&m_info, static_cast<float>(fontSize));
    m_bakedFonts[fontSize].ascent  = static_cast<int>(scaleFact * static_cast<float>(m_bakedFonts[fontSize].ascent));
    m_bakedFonts[fontSize].descent = static_cast<int>(scaleFact * static_cast<float>(m_bakedFonts[fontSize].descent));
    m_bakedFonts[fontSize].lineGap = static_cast<int>(scaleFact * static_cast<float>(m_bakedFonts[fontSize].lineGap));

    // generate bitmap on CPU
    stbtt_BakeFontBitmap(reinterpret_cast<unsigned char *>(&m_fontBuffer[0]),
                         0,                // offset
                         static_cast<float>(fontSize),  // pixel_height, also defines the x-advance scaling
                         &m_bakedFonts[fontSize].bitmap[0], m_tex_width,
                         m_tex_height,  // pixel_width, pixel_height
                         32, 255 - 32,  // first Char, num_chars
                         &m_bakedFonts[fontSize].cdata[0]);

    // upload bitmap to GPU
    glGenTextures(1, &m_bakedFonts[fontSize].tex);
    glBindTexture(GL_TEXTURE_2D, m_bakedFonts[fontSize].tex);
#ifdef __EMSCRIPTEN__
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, m_tex_width, m_tex_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE,
                 m_bakedFonts[fontSize].bitmap);
#else
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R8, m_tex_width, m_tex_height);
    glTexSubImage2D(GL_TEXTURE_2D,  // target
                    0,              // mipmap level
                    0, 0,           // x and y offset
                    m_tex_width,    // width and height
                    m_tex_height, GL_RED, GL_UNSIGNED_BYTE, &m_bakedFonts[fontSize].bitmap[0]);
#endif

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// prints in opengl standard normalized coordinates (0,0 center, left/bottom:
// [-1,-1], right/top: [1,1] this point refers to the lower left corner of the
// rendered text
void TypoGlyphMap::print(float _x, float _y, const std::string &text, int fontSize, float *col) {
    print(static_cast<int>((_x * .5f + .5f) * static_cast<float>(m_screen_width)),
        static_cast<int>((_y * -.5f + .5f) * static_cast<float>(m_screen_height)),
        text,
        fontSize,
        col);
}

void TypoGlyphMap::print(float _x, float _y, const std::string &&text, int fontSize, float *col) {
    print(static_cast<int>((_x * .5f + .5f) * static_cast<float>(m_screen_width)),
        static_cast<int>((_y * -.5f + .5f) * static_cast<float>(m_screen_height)),
        text,
        fontSize,
        col);
}

// prints in pixel coordinates, origin is left/top
void TypoGlyphMap::print(int _x, int _y, const std::string &text, int fontSize, float *col) {
    auto x = static_cast<float>(_x);
    auto y = static_cast<float>(_y);

    if (text.empty()) {
        return;
    }

    // bake font for this fontSize if necessary
    bakeFont(fontSize);

    if (text.size() + 1 != m_charOffs.size()) {
        m_charOffs.resize(text.size() + 1);
    }

    m_charOffsIt = m_charOffs.begin();

    if (!m_shaderInited) {
        glGenVertexArrays(1, &m_nullVao);
        initShader();
        m_shaderInited = true;
    }

    m_typoShader->begin();
    m_typoShader->setUniform1i("tex", 0);
    m_typoShader->setUniform4fv("color", col);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_bakedFonts[fontSize].tex);

    // does not start at x=0, ...first q.x0 > 0 - why the hack?
    int idx    = 0;
    int firstX = 0;

    for (auto ch : text) {
        if (static_cast<uint8_t>(ch) >= 32 && static_cast<uint8_t>(ch) < 255) {
            stbtt_GetBakedQuad(&m_bakedFonts[fontSize].cdata[0], m_tex_width, m_tex_height, static_cast<uint8_t>(ch) - 32, &x, &y,
                               &q, 1);

            // does not start at x=0, ...first q.x0 > 0 - why the hack?
            if (!idx) {
                firstX = static_cast<int>(q.x0 - _x);
            }

            (*m_charOffsIt++) = vec2(q.x0, q.y1);

            m_tex_offs.x  = q.s0;
            m_tex_offs.y  = q.t0;
            m_tex_scale.x = q.s1 - q.s0;
            m_tex_scale.y = q.t1 - q.t0;

            m_typoShader->setUniformMatrix4fv("m_pvm", &m_othoProj[0][0]);
            m_typoShader->setUniform2f("trans", q.x0 - firstX, q.y1);
            m_typoShader->setUniform2f("scale", q.x1 - q.x0, -(q.y1 - q.y0));
            m_typoShader->setUniform2fv("tex_offs", &m_tex_offs[0]);
            m_typoShader->setUniform2fv("tex_scale", &m_tex_scale[0]);

            glBindVertexArray(m_nullVao);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(0);
        }
        ++idx;
    }

    // add the last offset which will be right beside the last character
    *m_charOffsIt = *(m_charOffsIt - 1);
}

vec2 TypoGlyphMap::getTextWidth(const char *text, int fontSize) {
    // bake font for this fontSize if necessary
    bakeFont(fontSize);
    ivec2 pixSize = getPixTextWidth(text, fontSize);
    return {pixSize.x / m_screen_width * 2.f, pixSize.y / m_screen_height * 2.f};
}

glm::ivec2 TypoGlyphMap::getPixTextWidth(const char *text, int fontSize, int max_count) {
    glm::vec2 s{0, 0};
    int       minMaxY[2] = {std::numeric_limits<int>::max(), std::numeric_limits<int>::min()};
    int       width      = 0;
    int       maxIdx     = static_cast<int>(std::string(text).length() - 1);
    int       firstX     = 0;

    // bake font for this fontSize if necessary
    bakeFont(fontSize);

    // sh: better use stbtt_GetBakedQuad like in the draw function to be sure
    // the size is exatesla the same
    int idx = 0;
    while (*text) {
        if (max_count >= 0 && idx >= max_count) {
            break;
        }

        if (static_cast<uint8_t>(text[0]) >= 32 && static_cast<uint8_t>(text[0]) < 255) {
            stbtt_GetBakedQuad(&m_bakedFonts[fontSize].cdata[0], m_tex_width, m_tex_height, static_cast<uint8_t>(text[0]) - 32, &s.x,
                               &s.y, &q, 1);

            // does not start at x=0, ...first q.x0 > 0 - why the hack?
            if (!idx) {
                firstX = static_cast<int>(q.x0);
            }
            width = static_cast<int>(q.x1);

            minMaxY[0] = std::min(minMaxY[0], static_cast<int>(q.y0));
            minMaxY[0] = std::min(minMaxY[0], static_cast<int>(q.y1));

            minMaxY[1] = std::max(minMaxY[1], static_cast<int>(q.y0));
            minMaxY[1] = std::max(minMaxY[1], static_cast<int>(q.y1));
        }
        ++text;
        ++idx;
    }

    return {width - firstX, std::abs(minMaxY[1] - minMaxY[0])};
}

void TypoGlyphMap::setScreenSize(uint32_t screenWidth, uint32_t screenHeight) {
    m_screen_width  = screenWidth;
    m_screen_height = screenHeight;
    m_othoProj      = glm::ortho(0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight), 0.0f);
}

// - - - - - - - - - - - - - - - - - - -

int TypoGlyphMap::m_print(glm::vec2 offset, const char *text, int font_size, float *col, float *pvm, float x_range_lo,
                          float x_range_hi) {
    if (text == nullptr || !text[0]) return 0;

    // bake font for this fontSize if necessary
    bakeFont(font_size);

    if (!m_shaderInited) {
        initShader();
    }

    Shaders *shdr = m_typoShader_m;

    shdr->begin();
    shdr->setUniform1i("tex", 0);
    shdr->setUniform4fv("color", col);
    shdr->setUniformMatrix4fv("m_pvm", pvm);
    shdr->setUniform2f("xrange", x_range_lo, x_range_hi);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_bakedFonts[font_size].tex);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int                idx = 0;
    int                ro[2];

    while (text[0]) {
        if (static_cast<uint8_t>(text[0]) >= 32 && static_cast<uint8_t>(text[0]) < 255) {
            stbtt_aligned_quad qa;
            float d3d_bias = 0;
            const auto b = &m_bakedFonts[font_size].cdata[static_cast<uint8_t>(text[0]) - 32];

            ro[0] = STBTT_ifloor((offset.x + b->xoff) + 0.5f);
            ro[1] = STBTT_ifloor((offset.y + b->yoff) + 0.5f);

            qa.x0 = static_cast<float>(ro[0]) + d3d_bias;
            qa.y0 = static_cast<float>(ro[1]) + d3d_bias;
            qa.x1 = static_cast<float>(ro[0] + b->x1 - b->x0) + d3d_bias;
            qa.y1 = static_cast<float>(ro[1] + b->y1 - b->y0) + d3d_bias;

            qa.s0 = static_cast<float>(b->x0) * m_iTexSize.x;
            qa.t0 = static_cast<float>(b->y0) * m_iTexSize.y;
            qa.s1 = static_cast<float>(b->x1) * m_iTexSize.x;
            qa.t1 = static_cast<float>(b->y1) * m_iTexSize.y;

            shdr->setUniform2f("pix_off", qa.x0, qa.y0);
            shdr->setUniform2f("pix_size", qa.x1 - qa.x0, qa.y1 - qa.y0);
            shdr->setUniform2f("tex_off", qa.s0, qa.t0);
            shdr->setUniform2f("tex_size", qa.s1 - qa.s0, qa.t1 - qa.t0);

            glBindVertexArray(m_nullVao);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(0);

            offset.x += b->xadvance;
            ++idx;
        }
        ++text;
    }

    Shaders::end();
    return idx;
}

glm::ivec2 TypoGlyphMap::m_getPixExtent(const char *text, int font_size, int max_count, glm::mat2 *box,
                                        std::vector<float> *ppos) {
    glm::mat2 M(0.f);
    glm::vec2 offset(0, 0);

    if (ppos != nullptr) {
        ppos->clear();
    }

    if (box != nullptr) {
        *box = M;
    }

    if (text == nullptr || !text[0]) {
        return {0, 0};
    }

    bakeFont(font_size);

    int                idx = 0, i = 0;
    stbtt_aligned_quad qa       = {};
    int                ro[2];

    while (text[0]) {
        if (max_count >= 0 && i >= max_count) {
            break;
        }

        if (static_cast<uint8_t>(text[0]) >= 32 && static_cast<uint8_t>(text[0]) < 255) {
            float d3d_bias = 0;
            const auto b = &m_bakedFonts[font_size].cdata[static_cast<uint8_t>(text[0]) - 32];

            ro[0] = STBTT_ifloor((offset.x + b->xoff) + 0.5f);
            ro[1] = STBTT_ifloor((offset.y + b->yoff) + 0.5f);

            qa.x0 = static_cast<float>(ro[0]) + d3d_bias;
            qa.y0 = static_cast<float>(ro[1]) + d3d_bias;
            qa.x1 = static_cast<float>(ro[0] + b->x1 - b->x0) + d3d_bias;
            qa.y1 = static_cast<float>(ro[1] + b->y1 - b->y0) + d3d_bias;

            qa.s0 = static_cast<float>(b->x0) * m_iTexSize.x;
            qa.t0 = static_cast<float>(b->y0) * m_iTexSize.y;
            qa.s1 = static_cast<float>(b->x1) * m_iTexSize.x;
            qa.t1 = static_cast<float>(b->y1) * m_iTexSize.y;

            if (ppos != nullptr) {
                ppos->push_back(qa.x0);
            }

            if (idx) {
                M[0][0] = std::min(M[0][0], qa.x0);
                M[0][1] = std::min(M[0][1], qa.y0);
                M[1][0] = std::max(M[1][0], qa.x1);
                M[1][1] = std::max(M[1][1], qa.y1);
            } else {
                M[0] = glm::vec2(qa.x0, qa.y0);
                M[1] = glm::vec2(qa.x1, qa.y1);
            }

            offset.x += b->xadvance;
            ++idx;
        }

        ++text;
        ++i;
    }

    if (ppos != nullptr) {
        ppos->push_back(qa.x1);
    }

    if (box != nullptr) {
        *box = M;
    }

    return {static_cast<int>(M[1][0] - M[0][0]), static_cast<int>(M[1][1] - M[0][1])};
}

}  // namespace ara
