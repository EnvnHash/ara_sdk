//
// Created by user on 5/5/25.
//

#pragma once

#include <UIElements/UINodeBase/UINodeGeom.h>
#include <Asset/ResNode.h>

namespace ara {

class GLBase;

class UINodeStyle : public UINodeGeom {
public:
    ARA_NODE_ADD_SERIALIZE_FUNCTIONS(UINodeGeom, m_excludeFromStyles, m_visible, m_alpha, m_color, m_bgColor, m_borderColor, m_fontType)

    void setStyleInitVal(const std::string& name, const std::string& val, state st = state::m_state) override;

    virtual void loadStyleDefaults();
    virtual void rebuildCustomStyle();

    template <typename Callable>
    void updateStylePixAndPercent(ResNode* node, const state st, const std::string& findNode, const styleInit si, const Callable& f) {
        if (const auto numNode = node->findNumericNode(findNode); get<ResNode*>(numNode)) {
            if (get<unitType>(numNode) == unitType::Percent) {
                float val = std::stof(get<std::string>(numNode)) * 0.01f;
                m_setStyleFunc[st][si] = [val, f] { f(val); };
            } else {
                int val = std::stoi(get<std::string>(numNode));
                m_setStyleFunc[st][si] = [val, f] { f(val); };
            }
        }
    }

    void updateStyleColor(ResNode* node, state st, const std::string& findNode, styleInit si, const std::function<void(glm::vec4)>& f);
    void updateStylePixel(ResNode* node, state st, const std::string& findNode, styleInit si, const std::function<void(int)>& f);

    virtual void updateStyleIt(ResNode* node, state st, const std::string& styleClass);
    virtual void updateStyle();
    virtual void addStyleClass(const std::string& styleClass);
    virtual void clearStyles();
    virtual void applyStyle();

    void         setStyleInitCol(const std::string& propName, const glm::vec4& col, state st);
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
    void excludeFromStyles(const bool val) { m_excludeFromStyles = val; }
    void setState(state st);
    void setGlBase(GLBase* glBase);
    void setAlpha(float val);

    static std::string&     getCustomDefName() { return m_customDefName; }
    std::string&            getFontType() { return m_fontType; }
    [[nodiscard]] bool      isSelected() const { return m_state == state::selected; }
    [[nodiscard]] state     getState() const { return m_state; }
    [[nodiscard]] state     getLastState() const { return m_lastState; }
    virtual float           getAlpha() { return m_absoluteAlpha; }
    glm::vec4&              getColor() { return m_color; }
    glm::vec4&              getBackgroundColor() { return m_bgColor; }
    std::string&            getStyleClass() { return m_baseStyleClass; }
    [[nodiscard]] bool      getStyleChanged() const { return m_styleChanged; }
    [[nodiscard]] bool      getStyleClassInited() const { return m_styleClassInited; }
    [[nodiscard]] ResNode*  getStyleResNode() const;

    std::unordered_map<state, std::unordered_map<styleInit, std::function<void()>>> m_setStyleFunc;

protected:
    std::list<std::string> m_styleTree;

    GLBase* m_glbase = nullptr;

    bool m_styleChanged             = false;
    bool m_styleClassInited         = true;
    bool m_excludeFromStyles        = false;
    bool m_drawParamChanged         = false;
    bool m_visible                  = true;
    bool m_reqRebuildCustomStyle    = false;
    bool m_updateStyleScope         = false;

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
