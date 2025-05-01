#include "ImageButton.h"
#include "UIWindow.h"
#include <Asset/AssetColor.h>

using namespace glm;
using namespace std;

namespace ara {

ImageButton::ImageButton() : Div(), m_fontSize(22) {
    setName(getTypeName<ImageButton>());
    setFocusAllowed(false);
    setScissorChildren(true);
    m_onstate_back_tex = addChild<Image>();
    m_tex.resize(1);
    m_tex[0] = addChild<Image>();
}

ImageButton::ImageButton(const std::string& file) : Div() {
    setName(getTypeName<ImageButton>());
    setFocusAllowed(false);
    setScissorChildren(true);
    m_onstate_back_tex = addChild<Image>();
    m_tex.resize(1);
    m_tex[0] = addChild<Image>();
    m_tex[0]->setImg(file, 8);
}

void ImageButton::init() {
    if (!m_tex.empty() && m_tex[0]) {
        m_tex[0]->setName("ImgTex");
        m_tex[0]->setVisibility((m_tex.size() > 1 && m_tex[1] && !m_tex[1]->isLoaded()) || !m_isToggle ||
                                m_state != state::selected);
        m_tex[0]->excludeFromObjMap(true);
        m_tex[0]->setLod(m_lod);
    }

    if (m_tex.size() > 1 && m_tex[1]) {
        m_tex[1]->setVisibility((m_tex.size() > 1 && m_tex[1] && m_tex[1]->isLoaded()) && m_isToggle &&
                                m_state == state::selected);
        m_tex[1]->setName("OnStateTex");
        m_tex[1]->excludeFromObjMap(true);
        m_tex[1]->setLod(m_lod);
    }

    if (m_onstate_back_tex) {
        m_onstate_back_tex->setVisibility(m_isToggle && m_state == state::selected);
        m_onstate_back_tex->setName("OnStateBackgrTex");
        m_onstate_back_tex->excludeFromObjMap(true);
        m_onstate_back_tex->setLod(m_lod);
    }

    setObjUsesTexAlpha(m_objUsesTexAlpha);  // update in case it has been set before init
}

void ImageButton::loadStyleDefaults() {
    UINode::loadStyleDefaults();

    setStyleInitVal("img-align", "center,vcenter");

    m_setStyleFunc[state::none][styleInit::color] = [this] {
        for (auto &it : m_tex) {
            if (it) {
                it->setColor(1.f, 1.f, 1.f, 1.f);
            }
        }
    };

    m_setStyleFunc[state::none][styleInit::imgFlag] = [this] {
        for (auto &it : m_tex) {
            if (it) {
                it->m_imgFlags = 0;
            }
        }
    };

    m_setStyleFunc[state::none][styleInit::imgScale] = [this] {
        for (auto &it : m_tex) {
            if (it) {
                it->m_imgScale = 1.f;
            }
        }
    };

    // m_setStyleFunc[state::none][styleInit::imgAlign] = [this](){
    // m_ImgAlign[0] = m_ImgAlign[1] = 1; };
}

void ImageButton::updateStyleIt(ResNode *node, state st, const std::string &styleClass) {
    UINode::updateStyleIt(node, st, styleClass);

    auto *inode = node->findNode<ResNode>("image");
    if (inode) {
        std::string name                     = inode->m_value;
        m_setStyleFunc[st][styleInit::image] = [name, this]() {
            if (!m_tex.empty() && m_tex[0]) {
                m_tex[0]->setImgBase(m_sharedRes->res->img(name));
            }
        };
    }

    if (node->hasValue("imagePadding")) {
        float imgPadding                            = node->value1f("imagePadding", 1.f);
        m_setStyleFunc[st][styleInit::imagePadding] = [imgPadding, this]() {
            if (!m_tex.empty() && m_tex[0]) {
                m_tex[0]->setPadding(imgPadding, state::none);
                m_tex[0]->setPadding(imgPadding, state::selected);
                m_tex[0]->setPadding(imgPadding, state::disabled);
                m_tex[0]->setPadding(imgPadding, state::disabledHighlighted);
            }
        };
    }

    inode = node->findNode<ResNode>("imageOnState");
    if (inode) {
        std::string name                            = inode->m_value;
        m_setStyleFunc[st][styleInit::imageOnState] = [name, this]() {
            if (m_tex.size() < 2) {
                m_tex.emplace_back(addChild<Image>());
                m_tex.back()->excludeFromObjMap(true);
                m_tex.back()->setVisibility(false);
            }

            if (m_tex.size() > 1 && m_tex[1]) {
                m_tex[1]->setImgBase(m_sharedRes->res->img(name));
            }
        };
    }

    for (size_t i = 0; i < 10; i++) {
        inode = node->findNode<ResNode>("image_state" + std::to_string(i));
        if (inode) {
            if (!m_procIbl) {
                m_ibl.clear();
                m_procIbl = true;
            }
            while ((m_tex.size() - 1) < i) {
                m_tex.emplace_back(addChild<Image>());
                m_tex.back()->excludeFromObjMap(true);
                m_tex.back()->setVisibility(false);
                m_tex.back()->setLod(m_lod);
            }

            m_ibl.emplace_back(make_pair((int)i, m_sharedRes->res->img(inode->m_value)));
        }
    }

    inode = node->findNode<ResNode>("imageOnStateBack");
    if (inode) {
        std::string name                                = inode->m_value;
        m_setStyleFunc[st][styleInit::imageOnStateBack] = [name, this]() {
            if (m_onstate_back_tex) {
                m_onstate_back_tex->setImgBase(m_sharedRes->res->img(name));
            }
        };
    }

    if (m_procIbl) {
        m_setStyleFunc[st][styleInit::imageStates] = [this]() {
            for (auto &it : m_ibl) {
                if ((int)m_tex.size() > it.first && m_tex[it.first]) {
                    m_tex[it.first]->setImgBase(it.second);
                }
            }
        };

        m_procIbl = false;
    }

    // sh: changed to methods in order to be called also from Image()
    // (temporarily)
    if (node->hasValue("img-flags")) {
        ParVec   p      = node->splitNodeValue("img-flags");
        unsigned iflags = 0;

        for (std::string &par : p) {
            if (par == "fill") iflags |= 1;
            if (par == "scale") iflags |= 2;
            if (par == "hflip") iflags |= 4;
            if (par == "vflip") iflags |= 8;
            if (par == "int") iflags |= 16;
            if (par == "no-aspect") iflags |= 32;
        }

        m_setStyleFunc[st][styleInit::imgFlag] = [iflags, this]() {
            for (auto &it : m_tex) {
                if (it) {
                    it->m_imgFlags = iflags;
                }
            }
        };
    }

    if (node->hasValue("img-align")) {
        unsigned a[2] = {1, 1};

        ParVec p = node->splitNodeValue("img-align");

        for (std::string &par : p) {
            if (par == "left") a[0] = 0;
            if (par == "center") a[0] = 1;
            if (par == "right") a[0] = 2;

            if (par == "top") a[1] = 0;
            if (par == "vcenter") a[1] = 1;
            if (par == "bottom") a[1] = 2;
        }

        m_setStyleFunc[st][styleInit::imgAlign] = [a, this]() {
            for (auto &it : m_tex) {
                if (it) {
                    it->m_imgAlign[0] = a[0];
                    it->m_imgAlign[1] = a[1];
                }
            }
        };
    }

    if (node->hasValue("img-scale")) {
        float scale                             = node->value1f("img-scale", 1.f);
        m_setStyleFunc[st][styleInit::imgScale] = [scale, this]() {
            for (auto &it : m_tex) {
                if (it) {
                    it->m_imgScale = scale;
                }
            }
        };
    }

    AssetColor *color;
    if ((color = node->findNode<AssetColor>("color")) != nullptr) {
        vec4 col                             = color->getColorvec4();
        m_setStyleFunc[st][styleInit::color] = [this, col]() {
            for (auto &it : m_tex) {
                if (it) {
                    it->setColor(col.r, col.g, col.b, col.a);
                }
            }
        };
    }
}

void ImageButton::mouseMove(hidData *data) {
    data->actIcon = ui_MouseIcon;

    // if (!m_mouseIsIn && !m_show_alt_text)
    //     m_mouseInTime = std::chrono::system_clock::now();

    for (auto &it : m_mouseHidCb[hidEvent::MouseMove]) {
        it.first(data);
    }
    m_mouseIsIn = true;
    data->consumed = true;
}

void ImageButton::mouseIn(hidData *data) {
    m_mouseIsIn     = true;
    m_show_alt_text = false;

    for (auto &it : m_tex) {
        if (it) {
            it->mouseIn(data);
        }
    }

    if (m_onstate_back_tex) {
        m_onstate_back_tex->mouseIn(data);
    }

    UINode::mouseIn(data);
}

void ImageButton::mouseOut(hidData *data) {
    m_mouseIsIn     = false;
    m_show_alt_text = false;

    for (auto &it : m_tex) {
        if (it) {
            it->mouseOut(data);
        }
    }

    if (m_onstate_back_tex) {
        m_onstate_back_tex->mouseOut(data);
    }

    UINode::mouseOut(data);
}

void ImageButton::mouseUp(hidData *data) {
    if (data->hit) {
        if (m_state != state::disabled && m_state != state::disabledSelected && m_state != state::disabledHighlighted &&
            m_isToggle) {
            auto sw_state = (m_state == state::selected) ? m_state = state::none : m_state = state::selected;
            setSelectedDoAction(m_isMultiToggle || sw_state == state::selected);
            setDrawFlag();
        }

        if (m_state != state::disabled && m_clickedFunc) {
            m_clickedFunc();
        }

        // force a mouseOut
        if (getWindow() && getWindow()->getLastHoverFound()) {
            getWindow()->getLastHoverFound()->mouseOut(data);
            getWindow()->setLastHoverFound(nullptr);
        }

        data->consumed = true;
    }
}

void ImageButton::setProp(Property<bool> *prop) {
    onChanged<bool>(prop, [this](std::any val) {
        bool v = std::any_cast<bool>(val);
        if (isSelected() != v) {
            setSelected(v);
        }
    });

    setToggleCb([prop](bool val) { *prop = val; });
    setSelected((*prop)());
}

void ImageButton::setIsToggle(bool val, bool multiToggle) {
    m_isToggle      = val;
    m_isMultiToggle = multiToggle;
    applyToggleState();
}

void ImageButton::setSelected(bool val, bool forceStyleUpdt) {
    if (m_state == state::disabled || m_state == state::disabledSelected || m_state == state::disabledHighlighted){
        return;
    }

    if (!m_isToggle) {
        m_toggleState = -1;
    } else if ((m_isToggle && !m_isMultiToggle) && !m_tex.empty()) {
        m_toggleState = (int)val;
    } else if (m_isMultiToggle) {
        m_toggleState = (val ? (++m_toggleState % (int)m_tex.size()) : 0);
    }

    m_state = (m_isToggle && m_toggleState > 0) ? state::selected : state::none;

    applyToggleState();

    if (m_tex.size() <= 1) {
        UINode::setSelected(m_state == state::selected, forceStyleUpdt);
    }
}

void ImageButton::setDisabled(bool val, bool forceStyleUpdt) {
    UINode::setDisabled(val, forceStyleUpdt);

    for (auto it : m_tex) {
        if (it) {
            it->setDisabled(val, forceStyleUpdt);
        }
    }

    if (m_onstate_back_tex) {
        m_onstate_back_tex->setDisabled(val, forceStyleUpdt);
    }
}

void ImageButton::setDisabledHighlighted(bool val, bool forceStyleUpdt) {
    UINode::setDisabledHighlighted(val, forceStyleUpdt);
    for (auto &it : m_tex) {
        if (it) {
            it->setDisabledHighlighted(val, forceStyleUpdt);
        }
    }

    if (m_onstate_back_tex) {
        m_onstate_back_tex->setDisabledHighlighted(val, forceStyleUpdt);
    }
}

void ImageButton::setDisabledSelected(bool val, bool forceStyleUpdt) {
    UINode::setDisabledSelected(val, forceStyleUpdt);
    for (auto &it : m_tex) {
        if (it) {
            it->setDisabledSelected(val, forceStyleUpdt);
        }
    }

    if (m_onstate_back_tex) {
        m_onstate_back_tex->setDisabledSelected(val, forceStyleUpdt);
    }
}

void ImageButton::setToggleState(uint32_t st) {
    m_toggleState = st;
    m_state       = (m_isToggle && m_toggleState > 0) ? state::selected : state::none;

    applyToggleState();

    if (m_tex.size() <= 1) {
        UINode::setSelected(m_state == state::selected, true);
    }
}

void ImageButton::setVisibility(bool val) {
    UINode::setVisibility(val);

    if (!val) {
        for (auto &it : m_tex) {
            if (it) {
                it->setVisibility(false);
            }
        }

        if (m_onstate_back_tex) {
            m_onstate_back_tex->setVisibility(false);
        }
    } else {
        applyToggleState();
    }
}

void ImageButton::applyToggleState() {
    for (auto &it : m_tex) {
        if (it) {
            it->setVisibility(false);
        }
    }

    if (!m_tex.empty() && m_tex[0] && (!m_isToggle || (m_isToggle && m_tex.size() == 1))) {
        m_tex[0]->setVisibility(true);
    }

    if (m_isToggle && m_tex.size() > 1 && m_tex.size() > m_toggleState && m_tex[m_toggleState]) {
        m_tex[m_toggleState]->setVisibility(true);
    }

    if (m_onstate_back_tex) {
        m_onstate_back_tex->setVisibility(m_isToggle && m_state == state::selected);
    }
}

void ImageButton::setColor(float r, float g, float b, float a, state st, bool rebuildStyle) {
    for (auto &it : m_tex)
        if (it) {
            it->setColor(r, g, b, a, st);
            if (rebuildStyle) {
                it->rebuildCustomStyle();
            }
        }

    if (m_onstate_back_tex) {
        m_onstate_back_tex->setColor(r, g, b, a, st);
        if (rebuildStyle) {
            m_onstate_back_tex->rebuildCustomStyle();
        }
    }
}

void ImageButton::setColor(glm::vec4 col, state st, bool rebuildStyle) {
    for (auto &it : m_tex)
        if (it) {
            it->setColor(col, st);
            if (rebuildStyle) {
                it->rebuildCustomStyle();
            }
        }

    if (m_onstate_back_tex) {
        m_onstate_back_tex->setColor(col, st);
        if (rebuildStyle) {
            m_onstate_back_tex->rebuildCustomStyle();
        }
    }
}

void ImageButton::setStateImg(const std::string& file, imgType tp, int mipMapLevel) {
    while (m_tex.size() <= static_cast<size_t>(tp)) {
        m_tex.emplace_back(addChild<Image>());
    }

    m_tex[static_cast<size_t>(tp)]->setImg(file, mipMapLevel);

    if (m_secSize.x != 0 && m_secSize.y != 0) {
        m_tex[static_cast<size_t>(tp)]->setSectionSize(m_secSize);
    }

    if (m_secSep.x != 0 ||  m_secSep.y != 0) {
        m_tex[static_cast<size_t>(tp)]->setSectionSep(m_secSep);
    }

    if (m_secPos.x != 0 || m_secPos.y != 0) {
        m_tex[static_cast<size_t>(tp)]->setSectionPos(m_secPos);
    }
}

void ImageButton::setOnStateBackImg(const std::string& file, int mipMapLevel) {
    m_onstate_back_tex->setImg(file, mipMapLevel);
}

void ImageButton::setObjUsesTexAlpha(bool val) {
    m_objUsesTexAlpha = val;
    excludeFromObjMap(val);

    if (!m_tex.empty() && m_tex[0]) {
        m_tex[0]->excludeFromObjMap(!val);
        m_tex[0]->setObjUsesTexAlpha(val);
    }

    if (m_tex.size() > 1 && m_tex[1]) {
        m_tex[1]->excludeFromObjMap(!val);
        m_tex[1]->setObjUsesTexAlpha(val);
    }
}

void ImageButton::setImg(const std::string& file, int mipMapLevel) {
    if (m_tex.empty()) {
        m_tex.resize(1);
        m_tex[0] = addChild<Image>();
    }

    if (m_tex[0]) {
        m_tex[0]->setImg(file, mipMapLevel);
    }

    m_imgFile = file;
}

void ImageButton::setImgByStyle(std::string&& style) {
    if (m_tex.empty()) {
        m_tex.resize(1);
        m_tex[0] = addChild<Image>();
    }

    if (m_tex[0]) {
        m_tex[0]->addStyleClass(std::move(style));
    }
}

void ImageButton::setImgAlign(align ax, valign ay) {
    if (m_tex.empty()) {
        m_tex.resize(1);
        m_tex[0] = addChild<Image>();
    }

    if (m_tex[0]) {
        m_tex[0]->setAlign(ax, ay);
    }

    m_onstate_back_tex->setAlign(ax, ay);
}

void ImageButton::setSelectedDoAction(bool val) {
    setSelected(val, true);
    if (m_toggleCbFunc) {
        m_toggleCbFunc(m_state == state::selected);
    }
}

void ImageButton::setLod(float val) {
    for (const auto& it : m_tex) {
        it->setLod(val);
    }

    if (m_onstate_back_tex) {
        m_onstate_back_tex->setLod(val);
    }

    m_lod = val;
}

Image* ImageButton::getImg() {
    if (!m_tex.empty() && m_tex[0]) {
        return m_tex[0];
    } else {
        return nullptr;
    }
}

void ImageButton::setSectionSize(const glm::ivec2& sz) {
    m_secSize = sz;
    for (auto it : m_tex) {
        it->setSectionSize(sz);
    }
}

void ImageButton::setSectionSep(const glm::ivec2& sp) {
    m_secSep = sp;
    for (auto it : m_tex) {
        it->setSectionSep(sp);
    }
}

void ImageButton::setSectionPos(const glm::ivec2& pos) {
    m_secPos = pos;
    for (auto it : m_tex) {
        it->setSectionPos(pos);
    }
}


}  // namespace ara
