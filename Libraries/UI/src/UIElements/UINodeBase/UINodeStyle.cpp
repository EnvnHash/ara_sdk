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
    m_setStyleFunc[state::none][styleInit::x]         = [this]() { setX((int)0); };
    m_setStyleFunc[state::none][styleInit::y]         = [this]() { setY((int)0); };
    m_setStyleFunc[state::none][styleInit::width]     = [this]() { setWidth(1.f); };
    m_setStyleFunc[state::none][styleInit::height]    = [this]() { setHeight(1.f); };
    m_setStyleFunc[state::none][styleInit::align]     = [this]() { setAlignX(align::left); };
    m_setStyleFunc[state::none][styleInit::valign]    = [this]() { setAlignY(valign::top); };
    m_setStyleFunc[state::none][styleInit::color]     = [this]() { setColor(0.f, 0.f, 0.f, 0.f); };
    m_setStyleFunc[state::none][styleInit::bkcolor]   = [this]() { setBackgroundColor(0.f, 0.f, 0.f, 0.f); };
    m_setStyleFunc[state::none][styleInit::brdColor]  = [this]() { setBorderColor(0.f, 0.f, 0.f, 0.f); };
    m_setStyleFunc[state::none][styleInit::brdWidth]  = [this]() { setBorderWidth(0); };
    m_setStyleFunc[state::none][styleInit::brdRadius] = [this]() { setBorderRadius(0); };
    m_setStyleFunc[state::none][styleInit::padding]   = [this]() { setPadding(0, 0, 0, 0); };
    m_setStyleFunc[state::none][styleInit::visible]   = [this]() { setVisibility(true); };
}

void UINodeStyle::rebuildCustomStyle() {
    if (m_styleCustDefs.empty()) return;

    // rebuild the stylesheet
    m_custDefStyleSheet.clear();
    for (const auto& stateDef : m_styleCustDefs) {
        switch (stateDef.first) {
            case state::selected:
                m_custDefStyleSheet += "selected { \n";
                m_styleChanged = true;
                break;
            case state::highlighted:
                m_custDefStyleSheet += "highlighted { \n";
                m_styleChanged = true;
                break;
            case state::disabled:
                m_custDefStyleSheet += "disabled { \n";
                m_styleChanged = true;
                break;
            case state::disabledSelected:
                m_custDefStyleSheet += "disabledSelected { \n";
                m_styleChanged = true;
                break;
            case state::disabledHighlighted:
                m_custDefStyleSheet += "disabledHighlighted { \n";
                m_styleChanged = true;
                break;
            default: break;
        }

        if (!stateDef.second.empty())
            for (const auto& it : stateDef.second) {
                if (it.first == "text") {
                    m_custDefStyleSheet += "\t" + it.first + ":\"" + it.second + "\"\n";
                } else {
                    m_custDefStyleSheet += "\t" + it.first + ":" + it.second + "\n";
                }
            }

        if (stateDef.first == state::selected || stateDef.first == state::highlighted ||
            stateDef.first == state::disabled || stateDef.first == state::disabledHighlighted ||
            stateDef.first == state::disabledSelected) {
            m_custDefStyleSheet += "} \n";
        }
    }

    // convert the collected style definitions (strings) into a local ResNode
    if (!m_custDefStyleSheet.empty()) {
        SrcFile              s(m_glbase);
        std::vector<uint8_t> vp(m_custDefStyleSheet.begin(), m_custDefStyleSheet.end());
        ResNode::Ptr         n = std::make_unique<ResNode>(getCustomDefName(), m_glbase);

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

void UINodeStyle::updateStyleIt(ResNode* node, state st, const std::string& styleClass) {
    AssetColor* color;

    auto x = node->findNumericNode("x");
    if (get<ResNode*>(x)) {
        if (get<unitType>(x) == unitType::Percent) {
            float val                        = stof(get<string>(x)) * 0.01f;
            m_setStyleFunc[st][styleInit::x] = [this, val, st]() { setX(val, st); };
        } else {
            int val                          = stoi(get<string>(x));
            m_setStyleFunc[st][styleInit::x] = [this, val, st]() { setX(val, st); };
        }
    }

    auto y = node->findNumericNode("y");
    if (get<ResNode*>(y)) {
        if (get<unitType>(y) == unitType::Percent) {
            float val                        = stof(get<string>(y)) * 0.01f;
            m_setStyleFunc[st][styleInit::y] = [this, val, st]() { setY(val, st); };
        } else {
            int val                          = stoi(get<string>(y));
            m_setStyleFunc[st][styleInit::y] = [this, val, st]() { setY(val, st); };
        }
    }

    auto width = node->findNumericNode("width");
    if (get<ResNode*>(width)) {
        if (get<unitType>(width) == unitType::Percent) {
            float val                            = stof(get<string>(width)) * 0.01f;
            m_setStyleFunc[st][styleInit::width] = [this, val, st]() { setWidth(val, st); };
        } else {
            int val                              = stoi(get<string>(width));
            m_setStyleFunc[st][styleInit::width] = [this, val, st]() { setWidth(val, st); };
        }
    }

    auto height = node->findNumericNode("height");
    if (get<ResNode*>(height)) {
        if (get<unitType>(height) == unitType::Percent) {
            float val                             = stof(get<string>(height)) * 0.01f;
            m_setStyleFunc[st][styleInit::height] = [this, val, st]() { setHeight(val, st); };
        } else {
            int val                               = stoi(get<string>(height));
            m_setStyleFunc[st][styleInit::height] = [this, val, st]() { setHeight(val, st); };
        }
    }

    if (auto align = node->findNode("align")) {
        if (align->m_value == "center") {
            m_setStyleFunc[st][styleInit::align] = [this, st]() { setAlignX(align::center, st); };
        } else if (align->m_value == "left") {
            m_setStyleFunc[st][styleInit::align] = [this, st]() { setAlignX(align::left, st); };
        } else if (align->m_value == "right") {
            m_setStyleFunc[st][styleInit::align] = [this, st]() { setAlignX(align::right, st); };
        }
    }

    if (auto v_align = node->findNode("v-align")) {
        if (v_align->m_value == "center")
            m_setStyleFunc[st][styleInit::valign] = [this, st]() { setAlignY(valign::center, st); };
        else if (v_align->m_value == "top")
            m_setStyleFunc[st][styleInit::valign] = [this, st]() { setAlignY(valign::top, st); };
        else if (v_align->m_value == "bottom")
            m_setStyleFunc[st][styleInit::valign] = [this, st]() { setAlignY(valign::bottom, st); };
    }

    if ((color = node->findNode<AssetColor>("color")) != nullptr) {
        vec4 col                             = color->getColorvec4();
        m_setStyleFunc[st][styleInit::color] = [this, st, col]() { setColor(col.r, col.g, col.b, col.a, st); };
    }

    if ((color = node->findNode<AssetColor>("bkcolor")) != nullptr) {
        vec4 col                               = color->getColorvec4();
        m_setStyleFunc[st][styleInit::bkcolor] = [this, st, col]() {
            setBackgroundColor(col.r, col.g, col.b, col.a, st);
        };
    }

    if ((color = node->findNode<AssetColor>("border-color")) != nullptr) {
        vec4 col                                = color->getColorvec4();
        m_setStyleFunc[st][styleInit::brdColor] = [this, st, col]() { setBorderColor(col.r, col.g, col.b, col.a, st); };
    }

    auto borderWidth = node->findNumericNode("border-width");
    if (get<ResNode*>(borderWidth) && get<unitType>(borderWidth) == unitType::Pixels) {
        int val                                 = stoi(get<string>(borderWidth));
        m_setStyleFunc[st][styleInit::brdWidth] = [this, val, st]() { setBorderWidth(val, st); };
    }

    auto borderRad = node->findNumericNode("border-radius");
    if (get<ResNode*>(borderRad) && get<unitType>(borderRad) == unitType::Pixels) {
        int val                                  = stoi(get<string>(borderRad));
        m_setStyleFunc[st][styleInit::brdRadius] = [this, val, st]() { setBorderRadius(val, st); };
    }

    if (node->has("padding")) {
        ParVec pv = node->splitNodeValue("padding");

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

    if (auto vis = node->findNode("visible")) {
        bool val                               = vis->m_value == "true";
        m_setStyleFunc[st][styleInit::visible] = [this, val, st]() { setVisibility(val, st); };
    }
}

void UINodeStyle::updateStyle() {
    m_styleChanged     = false;
    m_styleClassInited = true;

    if (m_excludeFromStyles){
        return;
    }

    // reset style functions
    for (auto& it : m_setStyleFunc | views::values) {
        it.clear();
    }

    loadStyleDefaults();

    for (auto& it : m_styleTree) {
        auto resNode = (getCustomDefName() == it) ? m_customStyleNode.get() : m_sharedRes->res->findNode(it);
        if (!resNode) {
            continue;
        }

        // get styles for state::none
        updateStyleIt(resNode, state::none, it);

        // if there are subdefinitions and the corresponding flags are set,
        // return those definitions
        if (!resNode->m_node.empty()) {
            ResNode* auxResNode = nullptr;
            if ((auxResNode = resNode->findNode("selected"))) {
                updateStyleIt(auxResNode, state::selected, it);
            }
            if ((auxResNode = resNode->findNode("highlighted"))) {
                updateStyleIt(auxResNode, state::highlighted, it);
            }
            if ((auxResNode = resNode->findNode("disabled"))) {
                updateStyleIt(auxResNode, state::disabled, it);
            }
            if ((auxResNode = resNode->findNode("disabledSelected"))) {
                updateStyleIt(auxResNode, state::disabledSelected, it);
            }
            if ((auxResNode = resNode->findNode("disabledHighlighted"))) {
                updateStyleIt(auxResNode, state::disabledHighlighted, it);
            }
        }
    }

    // execute style functions for the none state to set default values
    auto tmpState = m_state;

    // need to change m_state to none temporarily in order to allow setters take effect
    if (m_state == state::selected) {
        setSelected(false, true);  // call the method instead of setting the variable directly, in order to respect
    }
    // modifications of this method in derived classes, force a style update

    // set state to none and update styles
    m_state = m_lastState = state::none;
    for (const auto& it : m_setStyleFunc[state::none]) {
        it.second();
    }

    // change state back
    m_state = tmpState;

    // in case the actual state is different from none, call those functions to
    // update the styles
    if (m_state != state::none) {
        if (m_state == state::selected) {
            setSelected(true, true);
        } else {
            for (const auto& it : m_setStyleFunc[m_state]) {
                it.second();
            }
        }
    }

    // updateDrawData(); // updateStyle is only called in updateMatrIt which
    // causes m_drawParamChanged = true, which causes updateDrawData()

    setChanged(true);  // recursive true
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
        for (const auto& it : m_setStyleFunc[m_state]) {
            it.second();
        }

        m_drawParamChanged = true;

        if (!m_setStyleFunc[m_state].empty()) {
            m_sharedRes->requestRedraw = true;
        }
    }
}

ResNode* UINodeStyle::getStyleResNode() {
    return m_sharedRes->res->findNode(m_baseStyleClass);
}

void UINodeStyle::setStyleInitVal(const std::string& name, const std::string& val, state st) {
    m_styleCustDefs[st == state::m_state ? m_state : st][name] = val;
}

void UINodeStyle::setStyleInitCol(const std::string& propName, const glm::vec4& col, state st) {
    setStyleInitVal(propName,
                    "rgba(" + std::to_string(static_cast<int>(col.r * 255)) + ","
                    + std::to_string(static_cast<int>(col.g * 255)) + ","
                    + std::to_string(static_cast<int>(col.b * 255)) + ","
                    + std::to_string(static_cast<int>(col.a * 255)) + ")",
                    st);
}

void UINodeStyle::setBorderColor(float r, float g, float b, float a, state st) {
    setBorderColor({r, g, b, a}, st);
}

void UINodeStyle::setBorderColor(const glm::vec4& col, state st) {
    if (st == state::m_state || st == m_state) {
        m_borderColor      = col;
        m_drawParamChanged = true;
    }
    setStyleInitCol("border-color", col, st);
}

void UINodeStyle::setColor(float r, float g, float b, float a, state st) {
    setColor({r, g, b, a}, st);
}

void UINodeStyle::setColor(const glm::vec4& col, state st) {
    if (st == state::m_state || st == m_state) {
        m_color            = col;
        m_drawParamChanged = true;
    }
    setStyleInitCol("color", col, st);
}

void UINodeStyle::setBackgroundColor(float r, float g, float b, float a, state st) {
    setBackgroundColor({r, g, b, a}, st);
}

void UINodeStyle::setBackgroundColor(const glm::vec4& col, state st) {
    if (st == state::m_state || st == m_state) {
        m_bgColor          = col;
        m_drawParamChanged = true;
    }
    setStyleInitCol("bkcolor", col, st);
}

void UINodeStyle::setAlpha(float val) {
    m_alpha = val;
    setChanged(true);
}

void UINodeStyle::setSelected(bool val, bool forceStyleUpdt) {
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

void UINodeStyle::setDisabled(bool val, bool forceStyleUpdt) {
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

void UINodeStyle::setHighlighted(bool val, bool forceStyleUpdt) {
    setState(val ? state::highlighted : m_lastState);
    if (forceStyleUpdt) {
        applyStyle();
    }
}

void UINodeStyle::setDisabledHighlighted(bool val, bool forceStyleUpdt) {
    setState(val ? state::disabledHighlighted : m_lastState);
    if (forceStyleUpdt) {
        applyStyle();
    }
}

void UINodeStyle::setDisabledSelected(bool val, bool forceStyleUpdt) {
    setState(val ? state::disabledSelected : m_lastState);
    if (forceStyleUpdt) {
        applyStyle();
    }
}

void UINodeStyle::setVisibility(bool val, state st) {
    if (st == state::m_state || st == m_state) {
        m_visible = val;
    }

    setStyleInitVal("visible", val ? "true" : "false", st);
}

void  UINodeStyle::setState(state st) {
    if (m_state != st) {
        m_lastState = m_state;
    }
    m_state = st;
}

void UINodeStyle::setGlBase(GLBase* glbase) {
    m_glbase = glbase;
}

}