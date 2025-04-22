#pragma once

#include "Div.h"
#include "Utils/TypoGlyphMap.h"

namespace ara {

struct LabelInitData {
    int x=0;
    int y=0;
    int w=0;
    int h=0;
    glm::vec4 text_color{};
    glm::vec4 bg_color{};
    const std::string& text;
    align ax{};
    valign ay{};
    const std::string& font_type;
    int font_height=0;
};

class Label : public Div {
public:
    enum eopt {
        none         = 0x0000,
        single_line  = 0x0010,    // Single line, do not accept CR
        accept_tabs  = 0x0020,    // Accepting TABs
        manual_space = 0x0040,    // the edit pixel space is defined by the user,
                                  // otherwise it will adapt to content size
        end_ellipsis = 0x0080,    // will crop text if exceeds area and add
                                  // ellipsis at the end (...)
        front_ellipsis = 0x0100,  // will crop text if exceeds area and add
                                  // ellipsis at the beginning (...)
        adaptive = 0x0120         // will adjust the fontsize to fit the content into
                                  // the container
    };

    Label();
    Label(std::string &&styleClass);
    Label(LabelInitData initData);
    ~Label() override = default;

    unsigned long getOpt() const { return m_tOpt; }

    unsigned long setOpt(unsigned long f) {
        m_tOpt |= f;
        m_glyphsPrepared = false;
        return m_tOpt;
    }

    unsigned long removeOpt(unsigned long f) {
        m_tOpt &= ~f;
        m_glyphsPrepared = false;
        return m_tOpt;
    }

    bool          hasOpt(unsigned long f) const { return m_tOpt & f; }
    void          setSingleLine() { setOpt(single_line); }
    virtual Font *UpdateDGV(bool *checkFontTexture);
    bool          draw(uint32_t *objId) override;
    bool          drawIndirect(uint32_t *objId) override;
    virtual bool  checkGlyphsPrepared(bool checkFontTex = false);
    void          updateMatrix() override;
    virtual void  updateFontGeo();
    void          updateDrawData() override;
    void          updateIndDrawData(bool checkFontTex = false);
    void          pushVaoUpdtOffsets() override;
    virtual void  prepareVao(bool checkFontTex = false);
    virtual void  reqUpdtGlyphs(bool updateTree);
    void          clearDs() override;
    void          loadStyleDefaults() override;
    void          updateStyleIt(ResNode *node, state st, std::string &styleClass) override;
    virtual void  setProp(Property<std::string> *prop);
    virtual void  setProp(Property<std::filesystem::path> *prop);

    void setFont(std::string fontType, uint32_t fontSize, align ax, valign ay, glm::vec4 fontColor,
                 state st = state::m_state) {
        setFontType(std::move(fontType), st);
        setFontSize((int)fontSize, st);
        setTextAlign(ax, ay, st);
        setColor(fontColor, st);
    }

    void setColor(float r, float g, float b, float a, state st = state::m_state) override {
        UINode::setColor(r, g, b, a, st);
    }

    void setColor(glm::vec4 &col, state st = state::m_state) override { UINode::setColor(col, st); }

    void setTextAlign(align ax, valign ay, state st = state::m_state) {
        setTextAlignX(ax, st);
        setTextAlignY(ay, st);
    }

    void setTextAlignX(align ax, state st = state::m_state) {
        m_tAlign_X       = ax;
        m_glyphsPrepared = false;
        setStyleInitVal("text-align", ax == align::center ? "center" : (ax == align::left ? "left" : "right"), st);
    }

    void setTextAlignY(valign ay, state st = state::m_state) {
        m_tAlign_Y       = ay;
        m_glyphsPrepared = false;
        setStyleInitVal("text-valign", ay == valign::center ? "center" : (ay == valign::top ? "top" : "bottom"), st);
    }

    void setText(const std::string &val, state st = state::m_state) {
        bool updt = val.size() != m_Text.size();
        m_Text    = val;
        reqUpdtGlyphs(updt);
        setStyleInitVal("text", val, st);
    }

    void setFontSize(int fontSize, state st = state::m_state) {
        if (st == state::m_state || st == m_state) {
            m_fontSize       = fontSize;
            m_glyphsPrepared = false;
        }
    }

    void setFontType(std::string fontType, state st = state::m_state) {
        if (st == state::m_state || st == m_state) {
            m_fontType       = std::move(fontType);
            m_glyphsPrepared = false;
        }
    }

    glm::vec2 &getTextBoundSize() {
        if (m_textBounds.x == 0.f || m_textBounds.y == 0.f) {
            UpdateDGV(nullptr);
        }
        return m_textBounds;
    }

protected:
    void setEditPixSpace(float width, float height, bool set_flag = true);

    glm::vec4 calculateMask();

    int         m_fontSize = 17;
    std::string m_fontType = "regular";

    FontGlyphVector m_FontDGV;
    Font           *m_riFont      = nullptr;
    Shaders        *m_glyphShader = nullptr;
    VAO             m_vao;
    UniformBlock    m_uniBlockLbl;
    IndDrawBlock    m_lblDB;

    unsigned  m_tOpt = 0;                 // options
    glm::vec2 m_tPos{0.f}, m_tSize{0.f};  // pixel space, offset and size
    glm::vec2 m_tSep{0.f};                // font pixel character separation (default=0,0)
    align     m_tAlign_X = align::center;
    valign    m_tAlign_Y = valign::center;
    glm::vec2 m_Offset{0.f};
    glm::vec2 m_alignOffset{0.f};
    float     m_TabSize      = 50.f;  // Tab size in pixels
    float     m_adaptScaling = 1.f;   // matrix scaling when using adaptive flag

    bool m_glyphsPrepared = false;

    std::string m_Text;

    glm::mat4 m_adaptScaleMat = glm::mat4(1.f);
    glm::mat4 m_modMvp        = glm::mat4(1.f);
    glm::vec4 m_mask{0.f};
    glm::vec2 m_textBounds{0.f};
    glm::vec2 m_bo{0.f};

    std::vector<glm::vec4> m_positions;
    std::vector<glm::vec2> m_texCoord;
    std::vector<GLuint>    m_indices;

    static inline std::array<glm::vec2, 4> m_vtxPos{glm::vec2{0.f}, glm::vec2{1.f, 0.f}, glm::vec2{0.f, 1.f},
                                                    glm::vec2{1.f}};
    static inline std::array<GLuint, 6>    m_elmInd{0, 1, 3, 3, 2, 0};

    // only locally used variables
    glm::vec2       m_tContSize{0.f};
    glm::vec2       bs{0.f};
    glm::vec2       bas{0.f};
    glm::vec2       inBoundsOffs{0.f};
    glm::vec2       tuv{0.f};
    glm::vec2       m_charSizePix{0.f};
    glm::vec4       m_scLabelIndDraw{0.f};
    FontGlyphVector faux;
    float           rightLimit    = 0.f;
    float           m_fontTexUnit = -1.f;
    size_t          dstSize       = 0;
};

}  // namespace ara
