#pragma once

#include "UIElements/Div.h"

namespace ara {

class Image;
class AssetImageBase;

class ImageButton : public Div {
public:
    enum class imgType { Normal = 0, On, Size };

    ImageButton();
    explicit ImageButton(const std::string& file);

    void init() override;
    void loadStyleDefaults() override;
    void updateStyleIt(ResNode* node, state st, const std::string& styleClass) override;

    void mouseMove(hidData& data) override;
    void mouseUp(hidData& data) override;
    void mouseIn(hidData& data) override;
    void mouseOut(hidData& data) override;

    virtual void setProp(Property<bool>& prop);
    virtual void setStateImg(const std::string& file, imgType tp, int mipMapLevel = 8);
    virtual void setToggleState(uint32_t st);
    virtual void setObjUsesTexAlpha(bool val);

    void setSelected(bool val, bool forceStyleUpdt = false) override;
    void setDisabled(bool val, bool forceStyleUpdt = false) override;
    void setDisabledHighlighted(bool val, bool forceStyleUpdt = false) override;
    void setDisabledSelected(bool val, bool forceStyleUpdt = false) override;
    void setVisibility(bool val);
    void setIsToggle(bool val, bool multiToggle = false);
    void setColor(float r, float g, float b, float a, state st = state::m_state, bool rebuildStyle = true) const;
    void setColor(glm::vec4 col, state st = state::m_state, bool rebuildStyle = true) const;
    void applyToggleState();
    void setImgByStyle(const std::string& style);
    void setImgAlign(align ax, valign ay);
    void setImg(const std::string& file, int mipMapLevel = 8);
    void setSelectedDoAction(bool val);
    void setLod(float val);
    void setSectionSize(const glm::ivec2& sz);
    void setSectionSep(const glm::ivec2& sp);
    void setSectionPos(const glm::ivec2& pos);

    void setAltText(const char* alt_text) { m_alt_text = std::string(alt_text); }
    void setToggleCb(std::function<void(bool)> cbFunc) { m_toggleCbFunc = std::move(cbFunc); }
    void setClickedCb(std::function<void()> cbFunc) { m_clickedFunc = std::move(cbFunc); }
    void setOnStateImg(const std::string& file, int mipMapLevel) { setStateImg(file, imgType::On, mipMapLevel); }
    void setOnStateBackImg(const std::string& file, int mipMapLevel = 8) const;

    [[nodiscard]] Image* getImg() const;

    std::string&            getImgFile() { return m_imgFile; }
    [[nodiscard]] uint32_t  getToggleState() const { return m_toggleState; }
    [[nodiscard]] bool      getToggle() const { return m_isToggle; }
    auto                    getValidTex() { return m_tex | std::views::filter([](auto it) { return it != nullptr; }); };

protected:
    bool m_mouseIsIn       = false;
    bool m_isToggle        = false;
    bool m_isMultiToggle   = false;
    bool m_show_alt_text   = false;
    bool m_objUsesTexAlpha = false;
    bool m_procIbl         = false;

    float m_lod = 0.f;

    int      m_fontSize    = 0;
    uint32_t m_toggleState = 0;

    std::string m_alt_text;
    glm::ivec2  m_secPos{0};
    glm::ivec2  m_secSize{0};
    glm::ivec2  m_secSep{0};
    glm::vec4   m_typoColor{0.f, 0.f, 0.f, 1.f};

    std::function<void(bool)> m_toggleCbFunc;
    std::function<void()>     m_clickedFunc = nullptr;
    std::string               m_imgFile;

    std::vector<Image*> m_tex;
    Image*              m_onstate_back_tex = nullptr;

    // temporary local variables made member variables for performance reasons
    std::list<std::pair<int, AssetImageBase*>> m_ibl;
};
}  // namespace ara
