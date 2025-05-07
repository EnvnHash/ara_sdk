//
// Created by user on 5/5/25.
//

#pragma once

#include <UIElements/UINodeBase/UINodeGeom.h>

namespace ara {

class ResNode;
class GLBase;

class UINodeStyle : public UINodeGeom {
public:
    void setStyleInitVal(const std::string& name, const std::string& val, state st = state::m_state) override;

    virtual void loadStyleDefaults();
    virtual void rebuildCustomStyle();
    virtual void updateStyleIt(ResNode* node, state st, const std::string& styleClass);
    virtual void updateStyle();
    virtual void addStyleClass(const std::string& styleClass);
    virtual void clearStyles();
    virtual void applyStyle();

    void setStyleInitCol(const std::string& propName, const glm::vec4& col, state st);
    virtual void setBorderColor(float r, float g, float b, float a, state st = state::m_state);
    virtual void setBorderColor(const glm::vec4& col, state st = state::m_state);
    virtual void setColor(float r, float g, float b, float a, state st = state::m_state);
    virtual void setColor(const glm::vec4& col, state st = state::m_state);
    virtual void setBackgroundColor(float r, float g, float b, float a, state st = state::m_state);
    virtual void setBackgroundColor(const glm::vec4& col, state st = state::m_state);

    virtual void setVisibility(bool val, state st = state::m_state);
    virtual void setSelected(bool val, bool forceStyleUpdt = false);
    virtual void setDisabled(bool val, bool forceStyleUpdt = false);
    virtual void setHighlighted(bool val, bool forceStyleUpdt = false);
    virtual void setDisabledHighlighted(bool val, bool forceStyleUpdt = false);
    virtual void setDisabledSelected(bool val, bool forceStyleUpdt = false);

    void setSelectedCb(std::function<void(bool)> f) { m_selectedCb = std::move(f); }
    void setFontType(std::string fontType) { m_fontType = std::string(std::move(fontType)); }
    void excludeFromStyles(bool val) { m_excludeFromStyles = val; }
    void setState(state st);
    void setGlBase(GLBase* glbase);
    void setAlpha(float val);

    static std::string& getCustomDefName() { return m_customDefName; }
    std::string&        getFontType() { return m_fontType; }
    bool                isSelected() { return m_state == state::selected; }
    state               getState() { return m_state; }
    state               getLastState() { return m_lastState; }
    virtual float       getAlpha() { return m_absoluteAlpha; }
    glm::vec4&          getColor() { return m_color; }
    glm::vec4&          getBackgroundColor() { return m_bgColor; }
    std::string&        getStyleClass() { return m_baseStyleClass; }
    bool                getStyleChanged() const { return m_styleChanged; }
    bool                getStyleClassInited() const { return m_styleClassInited; }

    ResNode* getStyleResNode();
    std::unordered_map<state, std::unordered_map<styleInit, std::function<void()>>> m_setStyleFunc;

protected:
    std::list<std::string> m_styleTree;

    GLBase* m_glbase = nullptr;

    bool m_styleChanged         = false;
    bool m_styleClassInited     = true;
    bool m_excludeFromStyles    = false;
    bool m_drawParamChanged     = false;
    bool m_visible              = true;

    float m_alpha           = 1.f; /// relative alpha of this node
    float m_absoluteAlpha   = 1.f; /// "flat" absolute alpha, top-down multiplied values

    glm::vec4 m_color{};
    glm::vec4 m_bgColor{};
    glm::vec4 m_borderColor{};
    glm::vec2 m_uvDiff{};

    std::string m_fontType;
    std::string m_baseStyleClass;
    std::string m_custDefStyleSheet = {0};

    std::unique_ptr<ResNode>                                                m_customStyleNode;
    std::unordered_map<state, std::unordered_map<std::string, std::string>> m_styleCustDefs;
    std::unordered_map<std::string, std::any>                               m_styleInitVals;

    std::function<void(bool)> m_selectedCb;

    static inline std::string m_customDefName = "__custom_Default";
};

}
