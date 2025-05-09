#include <UIElements/Label.h>
#include <Asset/AssetColor.h>
#include <Asset/AssetFont.h>
#include <Asset/AssetManager.h>

#include "UIWindow.h"

using namespace glm;
using namespace std;

namespace ara {

Label::Label() {
#ifndef FORCE_INMEDIATEMODE_RENDERING
    m_drawImmediate = false;
#endif
    setName(getTypeName<Label>());
    setFocusAllowed(false);
}

Label::Label(const std::string& styleClass) : Div(styleClass) {
#ifndef FORCE_INMEDIATEMODE_RENDERING
    m_drawImmediate = false;
#endif
    setName(getTypeName<Label>());
    setFocusAllowed(false);
}

Label::Label(const LabelInitData &initData) {
#ifndef FORCE_INMEDIATEMODE_RENDERING
    m_drawImmediate = false;
#endif
    setFocusAllowed(false);
    setName(getTypeName<Label>());
    setPos(initData.pos.x, initData.pos.y);
    setSize(initData.size.x, initData.size.y);
    Div::setBackgroundColor(initData.bg_color);
    Div::setColor(initData.text_color);
    setText(initData.text);
    setTextAlign(initData.ax, initData.ay);
    setFontSize(initData.font_height);
}

void Label::loadStyleDefaults() {
    UINode::loadStyleDefaults();

    m_setStyleFunc[state::none][styleInit::color]        = [this]() { setColor(1.f, 1.f, 1.f, 1.f); };
    m_setStyleFunc[state::none][styleInit::text]         = [this]() { m_Text = ""; };
    m_setStyleFunc[state::none][styleInit::textAlign]    = [this]() { m_tAlign_X = align::center; };
    m_setStyleFunc[state::none][styleInit::textValign]   = [this]() { m_tAlign_Y = valign::center; };
    m_setStyleFunc[state::none][styleInit::labelOptions] = [this]() { m_tOpt = 0; };
}

void Label::updateStyleIt(ResNode* node, state st, const std::string& styleClass) {
    UINode::updateStyleIt(node, st, styleClass);

    if (auto color = node->findNode<AssetColor>("text-color")) {
        vec4 col                                 = color->getColorvec4();
        m_setStyleFunc[st][styleInit::textColor] = [col, this, st]() { setColor(col.r, col.g, col.b, col.a, st); };
    }

    if (auto text = node->findNode("text")) {
        std::string tv                      = text->m_value;
        m_setStyleFunc[st][styleInit::text] = [tv, this]() { m_Text = tv; };
    }

    if (node->hasValue("text-opt")) {
        ParVec p = node->splitNodeValue("text-opt");
        unsigned opt = 0;

        for (std::string& par : p) {
            if (par == "single-line") {
                opt |= single_line;
            }
            if (par == "accept-tabs") {
                opt |= accept_tabs;
            }
            if (par == "manual-space") {
                opt |= manual_space;
            }
            if (par == "end-ellipsis") {
                opt |= end_ellipsis;
            }
            if (par == "front-ellipsis") {
                opt |= front_ellipsis;
            }
            if (par == "adaptive") {
                opt |= adaptive;
            }
        }

        m_tOpt                                      = opt;
        m_setStyleFunc[st][styleInit::labelOptions] = [this, opt]() { m_tOpt = opt; };
    }

    if (node->hasValue("text-align")) {
        ParVec p = node->splitNodeValue("text-align");
        align  aux = align::center;

        for (std::string& par : p) {
            if (par == "left") {
                aux = align::left;
            }
            if (par == "center") {
                aux = align::center;
            }
            if (par == "right") {
                aux = align::right;
            }
            if (par == "justify") {
                aux = align::justify;
            }
            if (par == "justify-ex") {
                aux = align::justify_ex;
            }
        }

        m_tAlign_X = aux;
        m_setStyleFunc[st][styleInit::textAlign] = [this, aux]() { m_tAlign_X = aux; };
    }

    if (node->hasValue("text-valign")) {
        ParVec p = node->splitNodeValue("text-valign");
        valign aux = valign::center;

        for (std::string& par : p) {
            if (par == "top") {
                aux = valign::top;
            }
            if (par == "vcenter" || par == "center") {
                aux = valign::center;
            }
            if (par == "bottom") {
                aux = valign::bottom;
            }
        }

        m_tAlign_Y = aux;
        m_setStyleFunc[st][styleInit::textValign] = [this, aux]() { m_tAlign_Y = aux; };
    }

    if (auto f = node->findNode<AssetFont>("font")) {
        int         size = f->value<int32_t>("size", 0);
        std::string font = f->getValue("font");

        m_setStyleFunc[st][styleInit::fontFontSize]   = [this, size]() { setFontSize(size); };
        m_setStyleFunc[st][styleInit::fontFontFamily] = [this, font]() { setFontType(font); };
    }
}

void Label::setProp(Property<std::string>* prop) {
    onChanged<std::string>(prop, [this](const std::any& val) {
        if (m_sharedRes)
            // is this really necessary as glcallback??? basically nothing
            // gl-ish is happening on setText
            static_cast<UIWindow *>(m_sharedRes->win)->addGlCb(this, "chgTxt", [this, val] {
                setText(std::any_cast<std::string>(val));
                return true;
            });
    });
    setText((*prop)());
}

void Label::setProp(Property<std::filesystem::path>* prop) {
    onChanged<std::filesystem::path>(prop, [this](const std::any& val) {
        if (m_sharedRes) {
            // is this really necessary as glcallback??? basically nothing
            // gl-ish  is happening on setText
            static_cast<UIWindow *>(m_sharedRes->win)->addGlCb(this, "chgTxt", [this, val] {
                setText(std::any_cast<std::filesystem::path>(val).string());
                return true;
            });
        }
    });
    setText((*prop)().string());
}

glm::vec4 Label::calculateMask() const {
    return {m_Offset.x, m_Offset.y, m_Offset.x + m_tContSize.x, m_Offset.y + m_tContSize.y};
}

Font* Label::UpdateDGV(bool* checkFontTex) {
    if (!m_sharedRes || !m_sharedRes->res) {
        return nullptr;
    }

    const auto font = getSharedRes()->res->getGLFont(m_fontType, m_fontSize, getPixRatio());
    if (!font) {
        LOGE << "[ERROR] UIEdit::UpdateDGV() / Cannot get font for " << m_fontType << "   size=" << m_fontSize;
        return nullptr;
    }

    m_riFont = font;

    if (!hasOpt(manual_space)) {
        m_tSize = m_tContSize;
    }

    m_FontDGV.setPixRatio(getPixRatio());
    m_FontDGV.setTabPixSize(m_TabSize);
    m_FontDGV.Process(m_riFont, m_tSize, m_tSep, m_tAlign_X, m_Text, !hasOpt(single_line) && !hasOpt(adaptive));

    if (!m_Text.empty()) {
        m_textBounds = m_FontDGV.getPixSize();

        if ((hasOpt(front_ellipsis) || hasOpt(end_ellipsis)) && hasOpt(single_line)) {
            bs = m_FontDGV.getPixSize();

            if (bs.x > m_tContSize.x) { // if the bounds of the renderer font are bigger than the content size
                // estimate the bounds of the rendered ellipsis in pixels at the actual font size
                faux.Process(m_riFont, m_tSize, m_tSep, m_tAlign_X, "...", false);

                bas = faux.getPixSize();

                const auto limit = m_tContSize.x - bas.x;  // get the available space (content size - ellipsis size)
                int i      = 0;
                rightLimit = m_FontDGV.getRightLimit();

                // sum up char until the max bounds is reached
                for (const auto& g : m_FontDGV.v)
                    if (g.gptr) {
                        if (hasOpt(end_ellipsis)) {
                            if (g.getRightLimit() > limit) {
                                break;
                            }
                            ++i;
                        } else {
                            ++i;
                            if (rightLimit - (g.getRightLimit()) <= limit) {
                                break;
                            }
                        }
                    }

                m_FontDGV.Process(m_riFont, m_tSize, m_tSep, m_tAlign_X,
                                  hasOpt(end_ellipsis) ? m_Text.substr(0, i) + "..."
                                                       : "..." + m_Text.substr(i, m_FontDGV.v.size() - 1),
                                  false);

                m_textBounds = m_FontDGV.getPixSize();
            }
        } else if (hasOpt(adaptive)) {
            m_adaptScaling = std::min(m_tContSize.x / m_textBounds.x, m_tContSize.y / m_textBounds.y);
        }
    } else {
        memset(&m_textBounds[0], 0, 8);
    }

    return m_riFont;
}

void Label::setEditPixSpace(float width, float height, bool set_flag) {
    m_tSize[0] = width;
    m_tSize[1] = height;

    if (set_flag) {
        setOpt(manual_space);
    }
}

bool Label::draw(uint32_t& objId) {
    Div::draw(objId);

    if (!m_glyphShader) {
        m_glyphShader = getSharedRes()->shCol->getStdGlyphShdr();
        m_uniBlockLbl.init(m_glyphShader->getProgram(), "nodeData");
        Label::updateDrawData();
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_riFont->getTexId());

    m_glyphShader->begin();
    m_glyphShader->setUniform1i("stex", 0);
    m_uniBlockLbl.bind();

    if (m_vao.isInited()) {
        m_vao.drawElements(GL_TRIANGLES);
    }

    return true;
}

bool Label::drawIndirect(uint32_t& objId) {
    Div::drawIndirect(objId);

    checkGlyphsPrepared(true);

    if (m_glyphsPrepared) {
        updateIndDrawData(true);
    }

    if (m_sharedRes && m_sharedRes->drawMan) {
        m_lblDB.drawSet = &m_sharedRes->drawMan->push(m_lblDB, this);
    }

    return true;  // count up objId
}

bool Label::checkGlyphsPrepared(bool checkFontTex) {
    if (!m_glyphsPrepared) {
        if (!m_sharedRes) {
            return false;
        }

        UpdateDGV(&checkFontTex);
        updateFontGeo();

        if (!m_drawImmediate) {
            m_lblDB.vaoData.resize(m_FontDGV.v.size() * 4);
            m_lblDB.indices.resize(m_FontDGV.v.size() * 6);
        }

        prepareVao(checkFontTex);

        m_glyphsPrepared = true;
        return true;
    } else {
        return false;
    }
}

void Label::updateFontGeo() {
    if (!m_riFont) return;

    memset(&m_alignOffset[0], 0, 8);

    if (m_tAlign_Y == valign::bottom) {
        bs              = m_FontDGV.getPixSize();
        bs.y            = std::max<float>(bs.y, m_riFont->getPixAscent());
        m_alignOffset.y = m_tContSize.y - bs.y;
    } else if (m_tAlign_Y == valign::center) {
        bs              = m_FontDGV.getPixSize();
        bs.y            = std::max<float>(bs.y, m_riFont->getPixAscent());
        m_alignOffset.y = m_tContSize.y * 0.5f - bs.y * 0.5f;
    }

    // take the matrix of the helper content Div, since this will use the Label's content transformation
    m_bo.x      = m_Offset[0] + m_alignOffset.x;
    m_bo.y      = m_Offset[1] + m_riFont->getPixAscent() + m_alignOffset.y;
    m_mask      = calculateMask();
    m_modMvp    = m_mvp;

    if (m_adaptScaling < 1.f) {
        // get in bounds offset
        inBoundsOffs = m_textBounds * (1.f - m_adaptScaling);

        m_bo.x = m_tAlign_X == align::center
                     ? m_bo.x + inBoundsOffs.x * 0.5f
                     : (m_tAlign_X == align::right ? m_bo.x + inBoundsOffs.x : m_bo.x / m_adaptScaling);
        m_bo.y = m_bo.y / m_adaptScaling - inBoundsOffs.y * 0.5f;

        m_adaptScaleMat = *m_orthoMat * *m_parentMat * m_nodePosMat * scale(vec3(m_adaptScaling, m_adaptScaling, 1.f));
        m_modMvp        = m_adaptScaleMat;
        m_adaptScaleMat = m_modMvp;

        m_mask.z = getContentSize().x / m_adaptScaling;
        m_mask.w = getContentSize().y / m_adaptScaling;
    }

    m_mask *= m_riFont->getPixRatio();
    m_bo *= m_riFont->getPixRatio();

    m_modMvp = m_modMvp * scale(vec3{1.f / getPixRatio(), 1.f / getPixRatio(), 1.f});
}

void Label::updateDrawData() {
    Div::updateDrawData();

    if (m_drawImmediate) {
        if (!m_uniBlockLbl.isInited()) {
            m_uniBlockLbl.addVarName("mvp", &m_modMvp[0][0], GL_FLOAT_MAT4);
            m_uniBlockLbl.addVarName("tcolor", &m_color[0], GL_FLOAT_VEC4);
            m_uniBlockLbl.addVarName("mask", &m_mask[0], GL_FLOAT_VEC4);
        } else {
            m_uniBlockLbl.update();
        }
    } else {
        updateIndDrawData();
    }
}

void Label::updateMatrix() {
    if (!m_geoChanged || m_updating) {
        return;
    }

    Div::updateMatrix();

    m_updating       = true;  // prevent updateMatrix feedback
    m_tContSize      = getContentSize();
    m_Offset         = getContentOffset();
    m_glyphsPrepared = false;
    checkGlyphsPrepared();
    m_updating = false;
}

// all input values are in virtual pixels and must be converted to hw pixels
void Label::prepareVao(bool checkFontTex) {
    if (m_drawImmediate) {
        dstSize = (size_t)(m_FontDGV.v.size() * 4);

        if (!m_vao.isInited()) {
            m_vao.init("position:4f,texCoord:2f");
        }

        if (m_vao.getNrVertices() < dstSize) {
            m_vao.resize(static_cast<GLuint>(dstSize));
            m_positions.resize(dstSize);
            m_texCoord.resize(dstSize);
            m_indices.resize(m_FontDGV.v.size() * 6);
        }

        size_t ind    = 0;
        size_t elmInd = 0;
        for (e_fontdglyph& g : m_FontDGV.v) {
            if (g.gptr) {
                for (const auto& v : m_vtxPos) {
                    tuv                = glm::floor(m_bo + g.opos + v * g.osize);
                    m_positions[ind].x = tuv.x;
                    m_positions[ind].y = tuv.y;
                    m_positions[ind].z = 0.f;
                    m_positions[ind].w = 1.f;
                    m_texCoord[ind]    = (g.gptr->srcpixpos + v * g.gptr->srcpixsize);
                    ind++;
                }

                for (size_t i = 0; i < m_elmInd.size(); i++) {
                    m_indices[elmInd * m_elmInd.size() + i] = static_cast<GLuint>(m_elmInd[i] + elmInd * m_vtxPos.size());
                }

                elmInd++;
            }
        }

        if (!m_positions.empty()) {
            m_vao.upload(CoordType::Position, &m_positions[0][0], static_cast<uint32_t>(dstSize));
        }

        if (!m_texCoord.empty()) {
            m_vao.upload(CoordType::TexCoord, &m_texCoord[0][0], static_cast<uint32_t>(dstSize));
        }

        if (!m_indices.empty()) {
            m_vao.setElemIndices(static_cast<uint32_t>(m_indices.size()), &m_indices[0]);
        }
    } else {
        updateIndDrawData(checkFontTex);
    }
}

void Label::updateIndDrawData(bool checkFontTex) {
    if (!m_riFont || !m_riFont->isOK() || !m_sharedRes || !m_sharedRes->objSel || m_lblDB.vaoData.empty() ||
        !getWindow())
        return;

    // check if the layerTexture containing this font was already collected, if not append it (must happen before Glyph
    // updating in order to have the correct texUnit value set to the vao data)
    if (m_riFont && checkFontTex) {
        m_fontTexUnit = m_sharedRes->drawMan->pushFont(m_riFont->getLayerTexId(), static_cast<float>(m_riFont->getLayerTexSize()));
    }

    auto ld = m_lblDB.vaoData.begin();

    getWinPos();
    for (int i = 0; i < 2; i++) {
        m_scLabelIndDraw[i] = std::max(m_scIndDraw[i], m_winRelPos[i]);
        m_scLabelIndDraw[i + 2] = m_size[i] - std::max((m_winRelPos[i] + m_size[i]) - (m_scIndDraw[i] + m_scIndDraw[i + 2]), 0.f);
    }

    for (e_fontdglyph& g : m_FontDGV.v) {
        if (!g.gptr) {
            continue;
        }

        for (const auto& v : stdQuadVertices) {
            if (ld == m_lblDB.vaoData.end()) {
                break;
            }

            tuv = glm::floor(m_bo + g.opos + v * g.osize);

            ld->aux1.x = tuv.x;
            ld->aux1.y = tuv.y;
            ld->aux1.z = 0.f;
            ld->aux1.w = 1.f;

            ld->pos = m_modMvp * ld->aux1;

            ld->texCoord = g.gptr->srcpixpos + v * g.gptr->srcpixsize;
            ld->color    = m_color;

            ld->aux2.x = m_fontTexUnit;                          // layerTex Id
            ld->aux2.y = static_cast<float>(m_riFont->getLayerTexLayerId());  // layer Id
            ld->aux2.z = m_excludeFromObjMap ? 0.f : static_cast<float>(m_objIdMin);
            ld->aux2.w = m_zPos;
            ld->aux3.x = 1.f;  // type indicator (1=Label)
            ld->aux3.w = m_absoluteAlpha;

            ++ld;
        }

        ld -= 4;  // reset iterator to quad beginning
        m_uvSize      = (ld + 3)->texCoord - ld->texCoord;
        m_charSizePix = g.osize / getWindow()->getPixelRatio();

        int i = 0;
        for (const auto& v : stdQuadVertices) {
            if (ld == m_lblDB.vaoData.end()) {
                break;
            }

            limitDrawVaoToBounds(ld, m_charSizePix, m_uvDiff, m_scLabelIndDraw, m_viewPort);  // scissoring, calculates m_uvDiff

            if (m_uvDiff.x != 0.f || m_uvDiff.y != 0.f) {
                limitTexCoordsToBounds(&ld->texCoord[0], i, m_uvSize, m_uvDiff);
            }

            ++ld;
            ++i;
        }
    }
}

void Label::pushVaoUpdtOffsets() {
    Div::pushVaoUpdtOffsets();

    if (m_lblDB.drawSet) {
        m_lblDB.drawSet->updtNodes.emplace_back(m_lblDB.getUpdtPair());
    }
}

void Label::reqUpdtGlyphs(bool updateTree) {
    m_glyphsPrepared = false;
    if (updateTree) {
        reqUpdtTree();
    } else {
        checkGlyphsPrepared(true);
        pushVaoUpdtOffsets();
    }
}

void Label::clearDs() {
    Div::clearDs();
    m_lblDB.drawSet = nullptr;
}

unsigned long Label::setOpt(unsigned long f) {
    m_tOpt |= f;
    m_glyphsPrepared = false;
    return m_tOpt;
}

unsigned long Label::removeOpt(unsigned long f) {
    m_tOpt &= ~f;
    m_glyphsPrepared = false;
    return m_tOpt;
}

void Label::setFont(const std::string& fontType, uint32_t fontSize, align ax, valign ay, glm::vec4 fontColor, state st) {
    setFontType(fontType, st);
    setFontSize(static_cast<int>(fontSize), st);
    setTextAlign(ax, ay, st);
    setColor(fontColor, st);
}

void Label::setColor(float r, float g, float b, float a, state st)  {
    Label::setColor({r, g, b, a}, st);
}

void Label::setColor(const glm::vec4 &col, state st)  {
    UINode::setColor(col, st);
}

void Label::setTextAlign(align ax, valign ay, state st) {
    setTextAlignX(ax, st);
    setTextAlignY(ay, st);
}

void Label::setTextAlignX(align ax, state st) {
    m_tAlign_X       = ax;
    m_glyphsPrepared = false;
    setStyleInitVal("text-align", ax == align::center ? "center" : (ax == align::left ? "left" : "right"), st);
}

void Label::setTextAlignY(valign ay, state st) {
    m_tAlign_Y       = ay;
    m_glyphsPrepared = false;
    setStyleInitVal("text-valign", ay == valign::center ? "center" : (ay == valign::top ? "top" : "bottom"), st);
}

void Label::setText(const std::string &val, state st) {
    bool updt = val.size() != m_Text.size();
    m_Text    = val;
    reqUpdtGlyphs(updt);
    setStyleInitVal("text", val, st);
}

void Label::setFontSize(int fontSize, state st) {
    if (st == state::m_state || st == m_state) {
        m_fontSize       = fontSize;
        m_glyphsPrepared = false;
    }
}

void Label::setFontType(std::string fontType, state st) {
    if (st == state::m_state || st == m_state) {
        m_fontType       = std::move(fontType);
        m_glyphsPrepared = false;
    }
}

vec2& Label::getTextBoundSize() {
    if (m_textBounds.x == 0.f || m_textBounds.y == 0.f) {
        UpdateDGV(nullptr);
    }
    return m_textBounds;
}

}  // namespace ara
