#include <UIElements/Text/Label.h>
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
    m_fontType = "regular";
#endif
    setTypeName<Label>();
    setName(getTypeName<Label>());
    setFocusAllowed(false);
}

Label::Label(const LabelPars &initData) {
#ifndef FORCE_INMEDIATEMODE_RENDERING
    m_drawImmediate = false;
#endif
    m_fontType = "regular";
    setFocusAllowed(false);
    setTypeName<Label>();
    setName(getTypeName<Label>());
    setPos(initData.pos.x, initData.pos.y);
    setSize(initData.size.x, initData.size.y);
    Div::setBackgroundColor(initData.bg_color);
    Div::setColor(initData.text_color);
    setText(initData.text);
    setTextAlign(initData.text_align_x, initData.text_align_y);
    setFontSize(initData.font_height);
    setAlignX(initData.align);
    setAlignY(initData.valign);
}

void Label::loadStyleDefaults() {
    UINode::loadStyleDefaults();

    m_setStyleFunc[state::none][styleInit::color]        = [this] { setColor(1.f, 1.f, 1.f, 1.f); };
    m_setStyleFunc[state::none][styleInit::text]         = [this] { m_text = ""; };
    m_setStyleFunc[state::none][styleInit::textAlign]    = [this] { m_tAlignX = align::center; };
    m_setStyleFunc[state::none][styleInit::textValign]   = [this] { m_tAlignY = valign::center; };
    m_setStyleFunc[state::none][styleInit::labelOptions] = [this] { m_tOpt = 0; };
}

void Label::updateStyleIt(ResNode* node, state st, const std::string& styleClass) {
    UINode::updateStyleIt(node, st, styleClass);

    updtStyleSingleValue<std::string>(node, styleInit::text, st, "", m_text);
    updtStyleSingleValue<uint32_t>(node, styleInit::labelOptions, st, 0, m_tOpt);
    updtStyleSingleValue<align>(node, styleInit::textAlign, st, align::center, m_tAlignX);
    updtStyleSingleValue<valign>(node, styleInit::textValign, st, valign::center, m_tAlignY);

    if (const auto color = node->findNode<AssetColor>("text-color")) {
        auto col                                 = color->getColorvec4();
        m_setStyleFunc[st][styleInit::textColor] = [col, this, st] { setColor(col.r, col.g, col.b, col.a, st); };
    }

    if (const auto f = node->findNode<AssetFont>("font")) {
        auto        size = f->value<int32_t>("size", 0);
        std::string font = f->getValue("font");

        m_setStyleFunc[st][styleInit::fontFontSize]   = [this, size] { setFontSize(size); };
        m_setStyleFunc[st][styleInit::fontFontFamily] = [this, font] { setFontType(font); };
    }
}

vec4 Label::calculateMask() const {
    return {m_offset.x, m_offset.y, m_offset.x + m_tContSize.x, m_offset.y + m_tContSize.y};
}

Font* Label::updateDGV(bool* checkFontTexture) {
    if (!getSharedRes() || !getSharedRes()->res) {
        return nullptr;
    }

    const auto font = getSharedRes()->res->getGLFont(getSharedRes()->winHandle, m_fontType, m_fontSize, getPixRatio());
    if (!font) {
        LOGE << "[ERROR] UIEdit::UpdateDGV() / Cannot get font for " << m_fontType << "   size=" << m_fontSize;
        return nullptr;
    }

    m_riFont = font;

    m_riFont->setFontTexChangedCb(this, [this] {
        // this is called when the font related 3D texture got a different texId, num of Layers or layerid,
        // so the VAO draw data needs to be updated (updateIndDrawData)
        m_fontLayerTexChanged = true;

        // in case num of layers or texId has changed, the DrawManager needs to updated it's fontTex data
        m_updateDrawSetFontData = m_riFont->getLayerTexId() != m_glFontPar.texId || m_riFont->getLayerTexNrLayers() != m_glFontPar.nrLayers;
        getSharedRes()->reqRedraw();
    });

    if (!hasOpt(manual_space)) {
        m_tSize = m_tContSize;
    }

    m_fontDGV.setPixRatio(getPixRatio());
    m_fontDGV.setTabPixSize(m_tabSize);
    m_fontDGV.process(m_riFont, m_tSize, m_tSep, m_tAlignX, m_text, !hasOpt(single_line) && !hasOpt(adaptive));

    if (!m_text.empty()) {
        m_textBounds = m_fontDGV.getPixSize();

        if ((hasOpt(front_ellipsis) || hasOpt(end_ellipsis)) && hasOpt(single_line)) {
            bs = m_fontDGV.getPixSize();

            if (bs.x > m_tContSize.x) { // if the bounds of the renderer font are bigger than the content size
                // estimate the bounds of the rendered ellipsis in pixels at the actual font size
                faux.process(m_riFont, m_tSize, m_tSep, m_tAlignX, "...", false);

                bas = faux.getPixSize();

                const auto limit = m_tContSize.x - bas.x;  // get the available space (content size - ellipsis size)
                int i = 0;
                const auto rightLimit = m_fontDGV.getRightLimit();

                // sum up char until the max bounds is reached
                for (const auto& g : m_fontDGV.getGlyphs())
                    if (g.glyphPtr) {
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

                m_fontDGV.process(m_riFont, m_tSize, m_tSep, m_tAlignX,
                                  hasOpt(end_ellipsis) ? m_text.substr(0, i) + "..."
                                                       : "..." + m_text.substr(i, m_fontDGV.getGlyphs().size() - 1),
                                  false);

                m_textBounds = m_fontDGV.getPixSize();
            }
        } else if (hasOpt(adaptive)) {
            m_adaptScaling = std::min(m_tContSize.x / m_textBounds.x, m_tContSize.y / m_textBounds.y);
        }
    } else {
        memset(&m_textBounds[0], 0, 8);
    }

    return m_riFont;
}

void Label::setEditPixSpace(const float width, const float height, const bool set_flag) {
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

    if (m_drawMan) {
        m_lblDB.drawSet = &m_drawMan->push(m_lblDB, this);
    }

    return true;  // count up objId
}

bool Label::checkGlyphsPrepared(bool checkFontTex) {
    if (!m_glyphsPrepared || m_fontLayerTexChanged) {
        if (!m_sharedRes) {
            return false;
        }

        if (!m_glyphsPrepared) {
            updateDGV(&checkFontTex);
            updateFontGeo();

            if (!m_drawImmediate) {
                m_lblDB.vaoData.resize(m_fontDGV.getGlyphs().size() * 4);
                m_lblDB.indices.resize(m_fontDGV.getGlyphs().size() * 6);
            }
        }

        prepareVao(checkFontTex || m_fontLayerTexChanged);

        m_glyphsPrepared = true;
        return true;
    }
    return false;
}

void Label::updateFontGeo() {
    if (!m_riFont) {
        return;
    }

    memset(&m_alignOffset[0], 0, 8);

    if (m_tAlignY == valign::bottom) {
        bs              = m_fontDGV.getPixSize();
        bs.y            = std::max<float>(bs.y, m_riFont->getPixAscent());
        m_alignOffset.y = m_tContSize.y - bs.y;
    } else if (m_tAlignY == valign::center) {
        bs              = m_fontDGV.getPixSize();
        bs.y            = std::max<float>(bs.y, m_riFont->getPixAscent());
        m_alignOffset.y = m_tContSize.y * 0.5f - bs.y * 0.5f;
    }

    // take the matrix of the helper content Div, since this will use the Label's content transformation
    m_bo.x      = m_offset[0] + m_alignOffset.x;
    m_bo.y      = m_offset[1] + m_riFont->getPixAscent() + m_alignOffset.y;
    m_mask      = calculateMask();
    m_modMvp    = m_mvp;

    if (m_adaptScaling < 1.f) {
        // get in bounds offset
        const auto inBoundsOffs = m_textBounds * (1.f - m_adaptScaling);

        m_bo.x = m_tAlignX == align::center
                     ? m_bo.x + inBoundsOffs.x * 0.5f
                     : (m_tAlignX == align::right ? m_bo.x + inBoundsOffs.x : m_bo.x / m_adaptScaling);
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
    if ((!m_geoChanged || m_updating) && !m_fontLayerTexChanged) {
        return;
    }

    Div::updateMatrix();

    m_updating       = true;  // prevent updateMatrix feedback
    m_tContSize      = getContentSize();
    m_offset         = getContentOffset();
    m_glyphsPrepared = false;
    checkGlyphsPrepared();
    m_updating = false;
}

// all input values are in virtual pixels and must be converted to hw pixels
void Label::prepareVao(const bool checkFontTex) {
    if (m_drawImmediate) {
        dstSize = m_fontDGV.getGlyphs().size() * 4;

        if (!m_vao.isInited()) {
            m_vao.init("position:4f,texCoord:2f");
        }

        if (m_vao.getNrVertices() < dstSize) {
            m_vao.resize(static_cast<GLuint>(dstSize));
            m_positions.resize(dstSize);
            m_texCoord.resize(dstSize);
            m_indices.resize(m_fontDGV.getGlyphs().size() * 6);
        }

        size_t ind    = 0;
        size_t elmInd = 0;
        for (auto& g : m_fontDGV.getGlyphs()) {
            if (g.glyphPtr) {
                for (const auto& v : m_vtxPos) {
                    tuv                = glm::floor(m_bo + g.pos + v * g.size);
                    m_positions[ind].x = tuv.x;
                    m_positions[ind].y = tuv.y;
                    m_positions[ind].z = 0.f;
                    m_positions[ind].w = 1.f;
                    m_texCoord[ind]    = g.glyphPtr->srcpixpos + v * g.glyphPtr->srcpixsize;
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

void Label::updateIndDrawData(const bool checkFontTex) {
    if (!m_riFont || !m_riFont->isOK() || !m_sharedRes || !m_sharedRes->objSel || m_lblDB.vaoData.empty() ||
        !getWindow()) {
        return;
    }

    // check if the layerTexture containing this font was already collected, if not append it (must happen before Glyph
    // updating in order to have the correct texUnit value set to the vao data)
    if (m_riFont && (checkFontTex || m_updateDrawSetFontData)) {
        m_texUnitArrayIndex = m_drawMan->pushFont(m_riFont);
        m_glFontPar.texId = m_riFont->getLayerTexId();
        m_glFontPar.layerId = m_riFont->getLayerTexLayerId();
        m_glFontPar.nrLayers = m_riFont->getLayerTexNrLayers();

        m_fontLayerTexChanged = false;
        m_updateDrawSetFontData = false;
    }

    auto ld = m_lblDB.vaoData.begin();

    getWinPos();
    vec4 scLabelIndDraw{0.f};
    for (int i = 0; i < 2; i++) {
        scLabelIndDraw[i] = std::max(m_scIndDraw[i], m_winRelPos[i]);
        scLabelIndDraw[i + 2] = m_size[i] - std::max(m_winRelPos[i] + m_size[i] - (m_scIndDraw[i] + m_scIndDraw[i + 2]), 0.f);
    }

    for (auto& g : m_fontDGV.getGlyphs()) {
        if (!g.glyphPtr) {
            continue;
        }

        for (const auto& v : stdQuadVertices) {
            if (ld == m_lblDB.vaoData.end()) {
                break;
            }

            tuv = m_bo + g.pos + v * g.size;

            ld->aux1.x = tuv.x;
            ld->aux1.y = tuv.y;
            ld->aux1.z = 0.f;
            ld->aux1.w = 1.f;

            ld->pos         = m_modMvp * ld->aux1;
            ld->texCoord    = g.glyphPtr->srcpixpos + v * g.glyphPtr->srcpixsize;
            ld->color       = g.color ? *g.color : m_color;
            ld->aux2.x      = m_texUnitArrayIndex;
            ld->aux2.y      = static_cast<float>(m_glFontPar.layerId);
            ld->aux2.z      = m_excludeFromObjMap ? 0.f : static_cast<float>(m_objIdMin);
            ld->aux2.w      = m_zPos;
            ld->aux3.x      = 1.f;  // type indicator (1=Label)
            ld->aux3.w      = m_absoluteAlpha;

            ++ld;
        }

        ld -= 4;  // reset iterator to quad beginning
        auto uvSize      = (ld + 3)->texCoord - ld->texCoord;
        auto charSizePix = g.size / getWindow()->getPixelRatio();

        int i = 0;
        for (const auto& v : stdQuadVertices) {
            if (ld == m_lblDB.vaoData.end()) {
                break;
            }

            limitDrawVaoToBounds(ld, charSizePix, m_uvDiff, scLabelIndDraw, m_viewPort);  // scissoring, calculates m_uvDiff

            if (m_uvDiff.x != 0.f || m_uvDiff.y != 0.f) {
                limitTexCoordsToBounds(&ld->texCoord[0], i, uvSize, m_uvDiff);
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

void Label::reqUpdtGlyphs(const bool updateTree) {
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

unsigned long Label::setOpt(const unsigned long f) {
    m_tOpt |= f;
    m_glyphsPrepared = false;
    return m_tOpt;
}

unsigned long Label::removeOpt(const unsigned long f) {
    m_tOpt &= ~f;
    m_glyphsPrepared = false;
    return m_tOpt;
}

void Label::setFont(const std::string& fontType, const uint32_t fontSize, const align ax, const valign ay,
                    const vec4 fontColor, const state st) {
    setFontType(fontType, st);
    setFontSize(static_cast<int>(fontSize), st);
    setTextAlign(ax, ay, st);
    setColor(fontColor, st);
}

void Label::setColor(float r, float g, float b, float a, const state st)  {
    Label::setColor({r, g, b, a}, st);
}

void Label::setColor(const vec4 &col, const state st)  {
    UINode::setColor(col, st);
}

void Label::setTextAlign(const align ax, const valign ay, const state st) {
    setTextAlignX(ax, st);
    setTextAlignY(ay, st);
}

void Label::setTextAlignX(const align ax, const state st) {
    m_tAlignX       = ax;
    m_glyphsPrepared = false;
    setStyleInitVal("text-align", ax == align::justify_ex ? "justify-ex" : ax == align::justify ? "justify" : ax == align::center ? "center" : ax == align::left ? "left" : "right", st);
}

void Label::setTextAlignY(const valign ay, const state st) {
    m_tAlignY       = ay;
    m_glyphsPrepared = false;
    setStyleInitVal("text-valign", ay == valign::center ? "center" : (ay == valign::top ? "top" : "bottom"), st);
}

void Label::setText(const std::string &val, const state st) {
    bool updt = val.size() != m_text.size();
    m_text    = val;
    reqUpdtGlyphs(updt);
    setStyleInitVal("text", val, st);
}

void Label::setFontSize(const int fontSize, const state st) {
    if (st == state::m_state || st == m_state) {
        m_fontSize       = fontSize;
        m_glyphsPrepared = false;
    }
}

void Label::setFontType(std::string fontType, const state st) {
    if (st == state::m_state || st == m_state) {
        m_fontType       = std::move(fontType);
        m_glyphsPrepared = false;
    }
}

vec2& Label::getTextBoundSize() {
    if (m_textBounds.x == 0.f || m_textBounds.y == 0.f) {
        updateDGV(nullptr);
    }
    return m_textBounds;
}

}  // namespace ara
