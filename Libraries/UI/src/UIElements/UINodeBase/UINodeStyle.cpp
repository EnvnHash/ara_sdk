//
// Created by user on 5/5/25.
//

#include <Asset/AssetColor.h>
#include <Asset/AssetManager.h>
#include <Asset/ResSrcFile.h>
#include <UIElements/UINodeBase/UINodeStyle.h>
#include <UISharedRes.h>
#include <GLBase.h>

using namespace std;
using namespace glm;

namespace ara {

void UINodeStyle::loadStyleDefaults() {
    m_setStyleFunc[state::none][styleInit::x]         = [this] { setX(0); };
    m_setStyleFunc[state::none][styleInit::y]         = [this] { setY(0); };
    m_setStyleFunc[state::none][styleInit::width]     = [this] { setWidth(1.f); };
    m_setStyleFunc[state::none][styleInit::height]    = [this] { setHeight(1.f); };
    m_setStyleFunc[state::none][styleInit::align]     = [this] { setAlignX(align::left); };
    m_setStyleFunc[state::none][styleInit::valign]    = [this] { setAlignY(valign::top); };
    m_setStyleFunc[state::none][styleInit::color]     = [this] { setColor(0.f, 0.f, 0.f, 0.f); };
    m_setStyleFunc[state::none][styleInit::bgcolor]   = [this] { setBackgroundColor(0.f, 0.f, 0.f, 0.f); };
    m_setStyleFunc[state::none][styleInit::brdColor]  = [this] { setBorderColor(0.f, 0.f, 0.f, 0.f); };
    m_setStyleFunc[state::none][styleInit::brdWidth]  = [this] { setBorderWidth(0); };
    m_setStyleFunc[state::none][styleInit::brdRadius] = [this] { setBorderRadius(0); };
    m_setStyleFunc[state::none][styleInit::padding]   = [this] { setPadding(0.f); };
    m_setStyleFunc[state::none][styleInit::visible]   = [this] { setVisibility(true); };
}

void UINodeStyle::rebuildCustomStyle() {
    if (m_styleCustDefs.empty()) {
        return;
    }

    // rebuild the stylesheet
    m_custDefStyleSheet.clear();

    unordered_map<state, std::string> stateStringMap {
        { state::selected, "selected" },
        { state::highlighted, "highlighted" },
        { state::disabled, "disabled" },
        { state::disabledSelected, "disabledSelected" },
        { state::disabledHighlighted, "disabledHighlighted" }
    };

    for (const auto&[state, defs] : m_styleCustDefs) {
        if (stateStringMap.contains(state)) {
            m_custDefStyleSheet += stateStringMap[state]+" { \n";
        }
        m_styleChanged = true;

        if (!defs.empty()) {
            for (const auto&[name, val] : defs) {
                if (name == "text") {
                    m_custDefStyleSheet += "\t" + name + ":\"" + val + "\"\n";
                } else {
                    m_custDefStyleSheet += "\t" + name + ":" + val + "\n";
                }
            }
        }

        if (state == state::selected || state == state::highlighted ||
            state == state::disabled || state == state::disabledHighlighted ||
            state == state::disabledSelected) {
            m_custDefStyleSheet += "} \n";
        }
    }

    // convert the collected style definitions (strings) into a local ResNode
    if (!m_custDefStyleSheet.empty()) {
        SrcFile              s(m_glbase);
        std::vector<uint8_t> vp(m_custDefStyleSheet.begin(), m_custDefStyleSheet.end());
        auto         n = std::make_unique<ResNode>(getCustomDefName(), m_glbase);

        if (s.process(n.get(), vp)) {
            n->preprocess();
            n->process();

            if (n->errList.empty() && n->load() && n->errList.empty()) {
                m_customStyleNode = std::move(n);
            }
        }

        if (m_styleTree.empty() || !m_styleTree.empty() && m_styleTree.front() != getCustomDefName()) {
            m_styleTree.push_front(getCustomDefName());  // must be the first entry
        }

        m_styleChanged = true;
    }
}

void UINodeStyle::updateStyleColor(ResNode* node, const state st, const std::string& findNode, const styleInit si, const std::function<void(vec4)>& f) {
    if (const auto color = node->findNode<AssetColor>(findNode)) {
        vec4 col = color->getColorVec4();
        m_setStyleFunc[st][si] = [f, col] { f(col); };
    }
}

void UINodeStyle::updateStylePixel(ResNode* node, const state st, const std::string& findNode, const styleInit si, const std::function<void(int)>& f) {
    if (const auto resNode = node->findNumericNode(findNode); get<ResNode*>(resNode) && get<unitType>(resNode) == unitType::Pixels) {
        auto val = stoi(get<string>(resNode));
        m_setStyleFunc[st][si] = [val, f] { f(val); };
    }
}

void UINodeStyle::updateStyleIt(ResNode* node, state st, const std::string& styleClass) {
    updateStylePixAndPercent(node, st, "x", styleInit::x, [this, st]<typename T>(T val){ setX(val, st); });
    updateStylePixAndPercent(node, st, "y", styleInit::y, [this, st]<typename T>(T val){ setY(val, st); });
    updateStylePixAndPercent(node, st, "width", styleInit::width, [this, st]<typename T>(T val){ setWidth(val, st); });
    updateStylePixAndPercent(node, st, "height", styleInit::height, [this, st]<typename T>(T val){ setHeight(val, st); });

    updateStyleColor(node, st, "color", styleInit::color, [this, st](const vec4& col) { setColor(col.r, col.g, col.b, col.a, st); });
    updateStyleColor(node, st, "bgcolor", styleInit::bgcolor, [this, st](const vec4& col) { setBackgroundColor(col.r, col.g, col.b, col.a, st); });
    updateStyleColor(node, st, "border-color", styleInit::brdColor, [this, st](const vec4& col) { setBorderColor(col.r, col.g, col.b, col.a, st); });

    updateStylePixel(node, st, "border-width", styleInit::brdWidth, [this, st](const int32_t val){ setBorderWidth(val, st); });
    updateStylePixel(node, st, "border-radius", styleInit::brdRadius, [this, st](const int32_t val){ setBorderRadius(val, st); });

    if (const auto align = node->findNode("align")) {
        if (align->getRawValue() == "center") {
            m_setStyleFunc[st][styleInit::align] = [this, st] { setAlignX(align::center, st); };
        } else if (align->getRawValue() == "left") {
            m_setStyleFunc[st][styleInit::align] = [this, st] { setAlignX(align::left, st); };
        } else if (align->getRawValue() == "right") {
            m_setStyleFunc[st][styleInit::align] = [this, st] { setAlignX(align::right, st); };
        }
    }

    if (const auto v_align = node->findNode("v-align")) {
        if (v_align->getRawValue() == "center")
            m_setStyleFunc[st][styleInit::valign] = [this, st] { setAlignY(valign::center, st); };
        else if (v_align->getRawValue() == "top")
            m_setStyleFunc[st][styleInit::valign] = [this, st] { setAlignY(valign::top, st); };
        else if (v_align->getRawValue() == "bottom")
            m_setStyleFunc[st][styleInit::valign] = [this, st] { setAlignY(valign::bottom, st); };
    }

    if (node->has("padding")) {
        auto pv = node->splitNodeValue("padding");

        // in case this is a reference iterate again
        if (pv.size() == 1 && !is_number(pv[0])) {
            if (auto n = node->getRoot()->findNode(pv[0])) {
                pv = n->splitValue(',');
            }
        }

        vec4 pd{pv.f(0), pv.f(1, pv.f(0)), pv.f(2, pv.f(0)), pv.f(3, pv.f(0))};
        m_setStyleFunc[st][styleInit::padding] = [this, st, pd]() {
            setPadding(pd.x, pd.y, pd.z, pd.w, st);
        };  // this will avoid exceptions if padding value has less than 4 values,
        // default is first value, so one can make it padding:10 for example and will become 10,10,10,10
    }

    if (const auto vis = node->findNode("visible")) {
        bool val                               = vis->getRawValue() == "true";
        m_setStyleFunc[st][styleInit::visible] = [this, val, st] { setVisibility(val, st); };
    }
}

void UINodeStyle::updateStyle() {
    m_styleChanged     = false;
    m_styleClassInited = true;
    m_updateStyleScope = true;

    if (m_reqRebuildCustomStyle) {
        rebuildCustomStyle();
    }

    m_reqRebuildCustomStyle = false;

    if (m_excludeFromStyles){
        return;
    }

    // reset style functions
    for (auto& it : m_setStyleFunc | views::values) {
        it.clear();
    }

    loadStyleDefaults();

    for (auto& it : m_styleTree) {
        const auto resNode = getCustomDefName() == it ? m_customStyleNode.get() : m_sharedRes->res->findNode(it);
        if (!resNode) {
            continue;
        }

        // get styles for state::none
        updateStyleIt(resNode, state::none, it);

        // if there are subdefinitions and the corresponding flags are set, return those definitions
        if (!resNode->m_children.empty()) {
            ResNode* auxResNode = nullptr;
            for (unordered_map<string, state> stateList = { {"selected", state::selected}, {"highlighted", state::highlighted},
                {"disabled", state::disabled}, {"disabledSelected", state::disabledSelected},
                {"disabledHighlighted", state::disabledHighlighted}};
                 const auto& [key, st] : stateList) {
                if ((auxResNode = resNode->findNode(key))) {
                    updateStyleIt(auxResNode, st, it);
                }
            }
        }
    }

    // execute style functions for the none state to set default values
    const auto tmpState = m_state;

    // need to change m_state to none temporarily in order to allow setters take effect
    if (m_state == state::selected) {
        setSelected(false, true);  // call the method instead of setting the variable directly, in order to respect
    }
    // modifications of this method in derived classes, force a style update

    // set state to none and update styles
    m_state = m_lastState = state::none;
    for (const auto &val: m_setStyleFunc[state::none] | views::values) {
        val();
    }

    // change state back
    m_state = tmpState;

    // in case the actual state is different from none, call those functions to
    // update the styles
    if (m_state != state::none) {
        if (m_state == state::selected) {
            setSelected(true, true);
        } else {
            for (const auto &val: m_setStyleFunc[m_state] | views::values) {
                val();
            }
        }
    }

    // updateDrawData(); // updateStyle is only called in updateMatrIt which
    // causes m_drawParamChanged = true, which causes updateDrawData()

    setChanged(true);  // recursive true
    m_updateStyleScope = false;
}

void UINodeStyle::addStyleClass(const std::string& styleClass) {
    if (m_styleTree.empty() || (m_styleTree.size() == 1 && m_styleTree.front().find("__") != std::string::npos)) {
        m_baseStyleClass = styleClass;
    }

    m_styleClassInited = false;

    // in case there was a previous (not default) style definition, delete it
    if (!m_styleTree.empty()) {
        if (const auto it = ranges::find_if(m_styleTree, [&styleClass](const auto& st) {
                return st == styleClass;
            }); it == m_styleTree.end()) {
            m_styleTree.emplace_back(styleClass);
        }
    } else {
        m_styleTree.emplace_back(styleClass);
    }
}

void UINodeStyle::clearStyles() {
    // clear style tree (text definitions)
    m_styleTree.clear();

    // clear lambada functions
    for (int i = 0; i < static_cast<int>(state::count); i++) {
        m_setStyleFunc[static_cast<state>(i)].clear();
    }

    loadStyleDefaults();  // calls the derived classes loadStyleDefaults
    rebuildCustomStyle();

    m_styleClassInited = false;
}

void UINodeStyle::applyStyle() {
    if (!m_excludeFromStyles) {
        // call all style definitions for the highlighted state if there are any
        for (const auto &val: m_setStyleFunc[m_state] | views::values) {
            val();
        }

        m_drawParamChanged = true;

        if (!m_setStyleFunc[m_state].empty()) {
            m_sharedRes->requestRedraw = true;
        }
    }
}

ResNode* UINodeStyle::getStyleResNode() const {
    return m_sharedRes->res->findNode(m_baseStyleClass);
}

void UINodeStyle::setStyleInitVal(const std::string& name, const std::string& val, const state st) {
    m_styleCustDefs[st == state::m_state ? m_state : st][name] = val;
    m_reqRebuildCustomStyle = !m_updateStyleScope;
}

void UINodeStyle::setStyleInitCol(const std::string& propName, const vec4& col, const state st) {
    setStyleInitVal(propName,
                    "rgba(" + std::to_string(static_cast<int>(col.r * 255)) + ","
                    + std::to_string(static_cast<int>(col.g * 255)) + ","
                    + std::to_string(static_cast<int>(col.b * 255)) + ","
                    + std::to_string(static_cast<int>(col.a * 255)) + ")",
                    st);
}

void UINodeStyle::setBorderColor(float r, float g, float b, float a, const state st) {
    setBorderColor({r, g, b, a}, st);
}

void UINodeStyle::setBorderColor(const vec4& col, const state st) {
    if (st == state::m_state || st == m_state) {
        m_borderColor      = col;
        m_drawParamChanged = true;
    }
    setStyleInitCol("border-color", col, st);
}

void UINodeStyle::setColor(float r, float g, float b, float a, const state st) {
    setColor({r, g, b, a}, st);
}

void UINodeStyle::setColor(const vec4& col, const state st) {
    if (st == state::m_state || st == m_state) {
        m_color            = col;
        m_drawParamChanged = true;
    }
    setStyleInitCol("color", col, st);
}

void UINodeStyle::setBackgroundColor(float r, float g, float b, float a, const state st) {
    setBackgroundColor({r, g, b, a}, st);
}

void UINodeStyle::setBackgroundColor(const vec4& col, const state st) {
    if (st == state::m_state || st == m_state) {
        m_bgColor          = col;
        m_drawParamChanged = true;
    }
    setStyleInitCol("bgcolor", col, st);
}

void UINodeStyle::setAlpha(const float val) {
    m_alpha = val;
    setChanged(true);
}

void UINodeStyle::setSelected(const bool val, const bool forceStyleUpdt) {
    if (m_state == state::disabled || m_state == state::disabledSelected || m_state == state::disabledHighlighted) {
        return;
    }

    setState(val ? state::selected : state::none);

    if (forceStyleUpdt){
        applyStyle();
    }

    if (m_selectedCb) {
        m_selectedCb(val);
    }
}

void UINodeStyle::setDisabled(const bool val, const bool forceStyleUpdt) {
    setState(val ? state::disabled : state::none);

    // clear m_lastState -> otherwise will cause unwanted style changed on mouseout
    if (!val) {
        m_lastState = state::none;
    }

    if (forceStyleUpdt) {
        applyStyle();
    }

    if (m_selectedCb) {
        m_selectedCb(val);
    }
}

void UINodeStyle::setHighlighted(const bool val, const bool forceStyleUpdt) {
    setState(val ? state::highlighted : m_lastState);
    if (forceStyleUpdt) {
        applyStyle();
    }
}

void UINodeStyle::setDisabledHighlighted(const bool val, const bool forceStyleUpdt) {
    setState(val ? state::disabledHighlighted : m_lastState);
    if (forceStyleUpdt) {
        applyStyle();
    }
}

void UINodeStyle::setDisabledSelected(const bool val, const bool forceStyleUpdt) {
    setState(val ? state::disabledSelected : m_lastState);
    if (forceStyleUpdt) {
        applyStyle();
    }
}

void UINodeStyle::setVisibility(const bool val, const state st) {
    if (st == state::m_state || st == m_state) {
        m_visible = val;
    }

    setStyleInitVal("visible", val ? "true" : "false", st);
}

void  UINodeStyle::setState(const state st) {
    if (m_state != st) {
        m_lastState = m_state;
    }
    m_state = st;
}

void UINodeStyle::setGlBase(GLBase* glBase) {
    m_glbase = glBase;
}

}