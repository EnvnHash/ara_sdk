#pragma once

#include <UIElements/Div.h>
#include <Utils/Typo/FontGlyphVector.h>
#include <Utils/VAO.h>
#include <UISharedRes.h>
#include <UIWindow.h>

namespace ara {

class Font;

struct LabelPars {
    glm::ivec2 pos{};
    glm::ivec2 size{};
    std::string style{};
    ara::align align = align::left; // gcc fails without the namespace specification
    ara::valign valign = valign::top;
    glm::vec4 color{ 1.f, 1.f, 1.f, 1.f };
    glm::vec4 bgColor{};
    uint32_t borderWidth{};
    uint32_t borderRadius{};
    glm::vec4 borderColor{};
    const std::string& text;
    ara::align textAlignX{};
    ara::valign textAlignY{};
    std::string fontType = "regular";
    int fontHeight=0;
};

class Label : public Div {
public:
    ARA_NODE_ADD_SERIALIZE_FUNCTIONS(Div, m_fontSize, m_tOpt,  m_tPos, m_tSize, m_tSep, m_tAlignX, m_tAlignY, m_offset, m_alignOffset, m_tabSize, m_adaptScaling, m_text)

    enum eopt {
        none         = 0x0000,
        single_line  = 0x0010,    // Single line, do not accept CR
        accept_tabs  = 0x0020,    // Accepting TABs
        manual_space = 0x0040,    // the edit pixel space is defined by the user, otherwise it will adapt to content size
        end_ellipsis = 0x0080,    // will crop text if exceeds area and add ellipsis at the end (...)
        front_ellipsis = 0x0100,  // will crop text if exceeds area and add ellipsis at the beginning (...)
        adaptive = 0x0120         // will adjust the fontsize to fit the content into the container
    };

    Label();
    explicit Label(const LabelPars& initData);
    ~Label() override = default;

    [[nodiscard]] unsigned long getOpt() const { return m_tOpt; }
    [[nodiscard]] bool          hasOpt(const unsigned long f) const { return m_tOpt & f; }

    unsigned long   setOpt(unsigned long f);
    unsigned long   removeOpt(unsigned long f);
    void            setSingleLine() { setOpt(single_line); }
    virtual Font*   updateDGV(bool *checkFontTexture);
    bool            draw(uint32_t& objId) override;
    bool            drawIndirect(uint32_t& objId) override;
    virtual bool    checkGlyphsPrepared(bool checkFontTex = false);
    void            updateMatrix() override;
    virtual void    updateFontGeo();
    void            updateDrawData() override;
    void            updateIndDrawData(bool checkFontTex = false);
    void            updateIndDrawDataGlyph(const Fontdglyph& g, std::vector<DivVaoData>::iterator& ld, glm::vec4& scLabelIndDraw);
    void            checkDrawLimits(const Fontdglyph& g, std::vector<DivVaoData>::iterator& ld, glm::vec4& scLabelIndDraw);
    void            pushVaoUpdtOffsets() override;
    virtual void    prepareVao(bool checkFontTex = false);
    virtual void    reqUpdtGlyphs(bool updateTree);
    void            clearDs() override;
    void            loadStyleDefaults() override;
    void            updateStyleIt(ResNode *node, state st, const std::string& styleClass) override;

    void setFont(const std::string& fontType, uint32_t fontSize, align ax, valign ay, glm::vec4 fontColor, state st = state::m_state);
    void setColor(float r, float g, float b, float a, state st = state::m_state) override;
    void setColor(const glm::vec4 &col, state st = state::m_state) override;
    void setTextAlign(align ax, valign ay, state st = state::m_state);
    void setTextAlignX(align ax, state st = state::m_state);
    void setTextAlignY(valign ay, state st = state::m_state);
    void setText(const std::string &val, state st = state::m_state);
    void setFontSize(int fontSize, state st = state::m_state);
    void setFontType(std::string fontType, state st = state::m_state);

    glm::vec2 &getTextBoundSize();
    const std::string& getText() { return m_text; }

    template<typename T>
    requires std::is_same_v<T, std::string> || std::is_same_v<T, std::filesystem::path>
    void setProp(Property<T> &prop) {
        onChanged<T>(prop, [this](const std::any& val) {
            if (getSharedRes()) {
                // is this really necessary as glcallback??? basically nothing gl-ish  is happening on setText
                static_cast<UIWindow *>(getSharedRes()->win)->addGlCb(this, "chgTxt", [this, val] {
                    setText(std::any_cast<T>(val).string());
                    return true;
                });
            }
        });
        setText(typeid(T) == typeid(std::string) ? prop() : prop().string());
    }

    template <typename T>
    void updtStyleSingleValue(ResNode* node, const styleInit& si, const state st, const T& defVal, T& dest) {
        if (const auto& key = m_styleInitToString[si]; node->hasValue(key)) {
            auto val = defVal;

            if constexpr (std::is_same_v<T, std::string>) {
                val = node->findNode(key)->getRawValue();
            } else {
                const auto p = node->splitNodeValue(key);
                if constexpr (std::is_same_v<T, uint32_t>) {
                    for (auto& par : p) val |= m_textOptMap[par];
                } else if constexpr (std::is_same_v<T, align>) {
                    for (auto& par : p) val = m_textAlignMap[par];
                } else if constexpr (std::is_same_v<T, valign>) {
                    for (auto& par : p) val = m_textVAlignMap[par];
                }
            }

            dest                    = val;
            m_setStyleFunc[st][si]  = [this, &dest, val] { dest = val; };
        }
    }

protected:
    void setEditPixSpace(float width, float height, bool set_flag = true);

    [[nodiscard]] glm::vec4 calculateMask() const;
    Font* checkAndGetFont();

    int32_t m_fontSize = 17;

    FontGlyphVector m_fontDGV;
    Font           *m_riFont      = nullptr;
    Shaders        *m_glyphShader = nullptr;
    VAO             m_vao;
    UniformBlock    m_uniBlockLbl;
    IndDrawBlock    m_lblDB;
    GlFontPar       m_glFontPar{};
    GLuint          m_texUnitArrayIndex = 0;

    unsigned  m_tOpt = 0;                 // options
    unsigned  m_initOptions = 0;                 // options for in syleDefaults
    glm::vec2 m_tPos{0.f}, m_tSize{0.f};  // pixel space, offset and size
    glm::vec2 m_tSep{0.f};                // additional font pixel character separation (default=0,0)
    align     m_tAlignX = align::center;
    valign    m_tAlignY = valign::center;
    glm::vec2 m_offset{0.f};
    glm::vec2 m_alignOffset{0.f};
    float     m_tabSize      = 50.f;  // Tab size in pixels
    float     m_adaptScaling = 1.f;   // matrix scaling when using adaptive flag

    std::pair<int32_t, float> m_lineOverflowOffset{};   // when single_line opt is activated and the text overflows, defines how much to shift, to move visible range

    bool      m_glyphsPrepared = false;
    bool      m_fontLayerTexChanged = false;
    bool      m_updateDrawSetFontData = false;

    std::string m_text;

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

    glm::vec2       m_tContSize{0.f};
    glm::vec2       bs{0.f};
    glm::vec2       bas{0.f};
    glm::vec2       tuv{0.f};
    FontGlyphVector faux;
    size_t          dstSize         = 0;

    static inline std::unordered_map<std::string, unsigned> m_textOptMap {
        { "single-line", single_line },
        { "accept-tabs", accept_tabs },
        { "manual-space", manual_space },
        { "end-ellipsis", end_ellipsis },
        { "front-ellipsis", front_ellipsis },
        { "adaptive", adaptive}
    };

    static inline std::unordered_map<std::string, align> m_textAlignMap {
        { "left", align::left },
        { "center", align::center },
        { "right", align::right },
        { "justify", align::justify },
        { "justify-ex", align::justify_ex }
    };

    static inline std::unordered_map<std::string, valign> m_textVAlignMap {
        { "top", valign::top },
        { "center", valign::center },
        { "vcenter", valign::center },
        { "bottom", valign::bottom },
    };

    static inline std::unordered_map<styleInit, std::string> m_styleInitToString {
        {styleInit::text, "text"},
        {styleInit::labelOptions, "text-opt"},
        {styleInit::textAlign, "text-align"},
        {styleInit::textValign, "text-valign"}
    };
};

}  // namespace ara
