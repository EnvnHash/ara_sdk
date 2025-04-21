//
// Created by user on 13.01.2021.
//

#include "Image.h"

using namespace std;
using namespace glm;

namespace ara {

Image::Image() : Div() {
    setName(getTypeName<Image>());
    setFocusAllowed(false);
    initDefaults();
#ifndef FORCE_INMEDIATEMODE_RENDERING
    m_imgDB.stdInit();
    m_drawImmediate = false;
#endif
}

Image::Image(std::string&& styleClass) : Div(std::move(styleClass)) {
    setName(getTypeName<Image>());
    setFocusAllowed(false);
    initDefaults();
#ifndef FORCE_INMEDIATEMODE_RENDERING
    m_imgDB.stdInit();
    m_drawImmediate = false;
#endif
}

Image::Image(const std::string& file, int mipMapLevel, bool keep_aspect, align ax, valign ay) : Div() {
    setName(getTypeName<Image>());
    setFocusAllowed(false);
    initDefaults();
    m_imageFile   = file;
    m_mipMapLevel = mipMapLevel;
    setAlign(ax, ay);
#ifndef FORCE_INMEDIATEMODE_RENDERING
    m_imgDB.stdInit();
    m_drawImmediate = false;
#endif
}

void Image::initDefaults() {
    m_ImgFlags    = 0;
    m_ImgScale    = 1.f;
    m_ImgAlign[0] = m_ImgAlign[1] = 1;
    m_mipMapLevel                 = 8;

    setStyleInitVal("img-align", "center,vcenter");
}

void Image::loadStyleDefaults() {
    UINode::loadStyleDefaults();

    setStyleInitVal("img-align", "center,vcenter");

    m_setStyleFunc[state::none][styleInit::color]    = [this]() { setColor(1.f, 1.f, 1.f, 1.f); };
    m_setStyleFunc[state::none][styleInit::imgFlag]  = [this]() { m_ImgFlags = 0; };
    m_setStyleFunc[state::none][styleInit::imgScale] = [this]() { m_ImgScale = 1.f; };
}

void Image::init() {
    m_texShdr = m_sharedRes->shCol->getUIGridTexSimple();
    if (!m_imageFile.empty()) {
        loadImg();
    }
}

void Image::updateStyleIt(ResNode* node, state st, std::string& styleClass) {
    UINode::updateStyleIt(node, st, styleClass);

    if (!m_sharedRes) {
        LOGE << "Image::updateStyleIt Error m_sharedRes not set";
        return;
    }

    // is there a style defined?
    if (m_sharedRes->res->img(styleClass)) {
        setImgBase(m_sharedRes->res->img(styleClass));
    }

    m_ImgFlags    = 0;
    m_ImgAlign[0] = m_ImgAlign[1] = 1;
    m_ImgScale    = 1;

    if (typeid(*node) == typeid(ImgSrc) || typeid(*node) == typeid(ImgSection)) {
        if (m_sharedRes->res->img(m_baseStyleClass)) {
            setImgBase(m_sharedRes->res->img(m_baseStyleClass));
        }
    } else {
        auto* inode = node->findNode<ResNode>("image");
        if (inode) {
            std::string name                     = inode->m_Value;
            m_setStyleFunc[st][styleInit::image] = [name, this]() { setImgBase(m_sharedRes->res->img(name)); };
        }
    }

    // sh: changed to methods in order to be called also from Image()
    // (temporarily)
    if (node->hasValue("img-flags")) {
        setImgFlag(node, st);
    }
    if (node->hasValue("img-align")) {
        setImgAlign(node, st);
    }
    if (node->hasValue("img-scale")){
        setImgScale(node, st);
    }
}

void Image::setImgFlag(ResNode* node, state st) {
    ParVec   p      = node->splitNodeValue("img-flags");
    unsigned iflags = 0;

    for (std::string& par : p) {
        if (par == "fill") {
            iflags |= 1;
        }
        if (par == "scale") {
            iflags |= 2;
        }
        if (par == "hflip") {
            iflags |= 4;
        }
        if (par == "vflip") {
            iflags |= 8;
        }
        if (par == "int") {
            iflags |= 16;
        }
        if (par == "no-aspect") {
            iflags |= 32;
        }
    }

    m_setStyleFunc[st][styleInit::imgFlag] = [iflags, this]() { m_ImgFlags = iflags; };
}

void Image::setImgAlign(ResNode* node, state st) {
    unsigned a[2] = {1, 1};
    ParVec   p    = node->splitNodeValue("img-align");

    for (std::string& par : p) {
        if (par == "left") {
            a[0] = 0;
        }
        if (par == "center") {
            a[0] = 1;
        }
        if (par == "right") {
            a[0] = 2;
        }

        if (par == "top") {
            a[1] = 0;
        }
        if (par == "vcenter") {
            a[1] = 1;
        }
        if (par == "bottom") {
            a[1] = 2;
        }
    }

    m_setStyleFunc[st][styleInit::imgAlign] = [a, this]() {
        m_ImgAlign[0] = a[0];
        m_ImgAlign[1] = a[1];
    };
}

void Image::setImgScale(ResNode* node, state st) {
    float scale                             = node->value1f("img-scale", 1.f);
    m_ImgScale                              = scale;
    m_setStyleFunc[st][styleInit::imgScale] = [scale, this]() { m_ImgScale = scale; };
}

void Image::setImg(const std::string& file, int mipMapLevel) {
    m_imageFile   = file;
    m_mipMapLevel = mipMapLevel;
    if (m_drawImmediate) {
        Image::updateDrawData();
    }
}

unsigned Image::setImgFlags(unsigned flags) {
    m_ImgFlags                                  = flags;
    m_setStyleFunc[m_state][styleInit::imgFlag] = [flags, this]() { m_ImgFlags = flags; };

    if (flags & 1) {
        setStyleInitVal("img-flags", "fill");
    }
    if (flags & 2) {
        setStyleInitVal("img-flags", "scale");
    }
    if (flags & 4) {
        setStyleInitVal("img-flags", "hflip");
    }
    if (flags & 8) {
        setStyleInitVal("img-flags", "vflip");
    }
    if (flags & 16) {
        setStyleInitVal("img-flags", "int");
    }
    if (flags & 32) {
        setStyleInitVal("img-flags", "no-aspect");
    }

    return m_ImgFlags;
}

void Image::setImgScale(float scale) {
    m_setStyleFunc[m_state][styleInit::imgScale] = [this, scale]() { m_ImgScale = scale; };
    setStyleInitVal("img-scale", std::to_string(scale));
}

void Image::setImgBase(ImageBase* imgBase) {
    if ((m_imgBase = imgBase) == nullptr) return;

    if (m_sharedRes) {
        if (m_imgBase->getType() == ImageBase::Type::frame) {
            m_texShdr = m_sharedRes->shCol->getUIGridTexFrame();
        } else {
            m_texShdr = m_sharedRes->shCol->getUIGridTexSimple();
        }

        m_loaded = true;
    }
}

void Image::setFillToNodeSize(bool val, state st) {
    if (st == state::m_state || st == m_state) {
        m_ImgFlags |= 32;
    }
    setStyleInitVal("img-flags", "no-aspect", st);
}

void Image::setObjUsesTexAlpha(bool val) {
    m_objUsesTexAlpha = val;
    Image::updateDrawData();
}

void Image::updateDrawData() {
    Div::updateDrawData();

    if (m_useTexId) {
        memset(&m_secPos[0], 0, 8);
        m_secSize.x = m_extTexWidth;
        m_secSize.y = m_extTexHeight;
        m_texSize.x = m_extTexWidth;
        m_texSize.y = m_extTexHeight;
    } else if (m_imgBase) {
        for (int i=0; i<2; ++i) {
            m_secPos[i] = m_imgBase->getVerPos(m_sectIndex)[i];
            m_secSize[i] = m_imgBase->getSectionSize()[i];
            m_secSep[i] = m_imgBase->getSectionSep()[i];
        }

        if (m_imgBase->getTexture()) {
            m_texSize.x = static_cast<int>(m_imgBase->getTexture()->getWidth());
            m_texSize.y = static_cast<int>(m_imgBase->getTexture()->getHeight());
        }
    } else {
        if (tex && !m_useTexId) {
            if (m_secPos.x == 0 && m_secPos.y == 0) {
                memset(&m_secPos[0], 0, 8);
            }

            if (m_secSize.x == 0 && m_secSize.y == 0) {
                m_secSize.x = static_cast<int>(tex->getWidth());
                m_secSize.y = static_cast<int>(tex->getHeight());
            }

            if (m_texSize.x == 0 && m_texSize.y == 0) {
                m_texSize.x = static_cast<int>(tex->getWidth());
                m_texSize.y = static_cast<int>(tex->getHeight());
            }
        }
    }

    if (m_drawImmediate) {
        initUnitBlock();
    } else {
        m_nSize = getContentSize();
        m_nPos  = getContentOffset();

        tso = m_secSize.x <= 0 ? m_texSize : m_secSize;
        if (tso.x != 0.f && tso.y != 0.f) {
            ts = tso * ((m_ImgFlags & 1) != 0 ? std::max<float>(m_nSize.x / tso.x, m_nSize.y / tso.y)
                                              : std::min<float>(m_nSize.x / tso.x, m_nSize.y / tso.y));
            if ((m_ImgFlags & 2) != 0) {
                ts = tso * m_ImgScale;
            }
        } else {
            ts.x = 1.f;
            ts.y = 1.f;
        }

        auto dIt = m_imgDB.vaoData.begin();

        // pre calculate texture coordinates
        for (auto& it : stdQuadVertices) {
            if (m_imgBase && m_imgBase->getType() == ImageBase::Type::frame) {
                v = it * (m_size - static_cast<float>(m_borderWidth) * 2.f);
            } else {
                v  = it * m_nSize;
                uv = vec2(v.x - (m_ImgAlign[0] == 1   ? (m_nSize.x * 0.5f - ts.x * 0.5f)
                                 : m_ImgAlign[0] == 2 ? m_nSize.x - ts.x
                                                      : 0.f),
                          v.y - (m_ImgAlign[1] == 1   ? (m_nSize.y * 0.5f - ts.y * 0.5f)
                                 : m_ImgAlign[1] == 2 ? m_nSize.y - ts.y
                                                      : 0.f)) / ts;

                uv.x = ((m_ImgFlags & 4) != 0) ? 1.f - uv.x : uv.x;
                uv.y = ((m_ImgFlags & 8) != 0) ? 1.f - uv.y : uv.y;

                tuv = m_secSize.x <= 0 ? uv : (vec2(m_secPos) + vec2(m_secSize) * uv) / vec2(m_texSize);

                // no-aspect ratio
                if ((m_ImgFlags & 32) != 0) {
                    uv = tuv = it;
                    tuv.x    = ((m_ImgFlags & 4) != 0) ? 1.f - tuv.x : tuv.x;
                    tuv.y    = ((m_ImgFlags & 8) != 0) ? 1.f - tuv.y : tuv.y;
                }
            }

            // calculate position in normalized screen coordinates
            if (m_imgBase && m_imgBase->getType() == ImageBase::Type::frame) {
                dIt->pos = m_mvp * vec4(m_nPos - vec2(m_padding) + v, 0.f, 1.f);

                dIt->aux0.x = m_size.x - static_cast<float>(m_borderWidth) * 2.f;
                dIt->aux0.y = m_size.y - static_cast<float>(m_borderWidth) * 2.f;
                dIt->aux0.z = v.x;
                dIt->aux0.w = v.y;
            } else {
                //dIt->pos = m_mvp * vec4(m_nPos + v, 0.f, 1.f);
                dIt->pos = m_mvp * vec4(it * m_size, 0.f, 1.0);

                dIt->aux0.x = uv.x;
                dIt->aux0.y = uv.y;
                dIt->aux0.z = tuv.x;
                dIt->aux0.w = tuv.y;
            }

            dIt++;
        }

        dIt = m_imgDB.vaoData.begin();

        // calculate normalized texture view size
        m_uvSize  = vec2(m_imgDB.vaoData[3].aux0) - vec2(m_imgDB.vaoData[0].aux0);
        m_tuvSize = vec2{m_imgDB.vaoData[3].aux0.z, m_imgDB.vaoData[3].aux0.w} -
                    vec2{m_imgDB.vaoData[0].aux0.z, m_imgDB.vaoData[0].aux0.w};

        // calculate the reference size of the ui element (may differ from
        // m_size in case of content transformation of a parent element)
        for (int i = 0; i < 2; i++) {
            m_divRefSize[i] = ((dIt + 3)->pos[i] - dIt->pos[i]) * 0.5f * m_viewPort[i + 2];
        }
        m_divRefSize[1] *= -1.f;

        int j = 0;
        for (auto& it : stdQuadVertices) {
            // since scissoring can't be used in indirect draw mode, it has to be done at this point by adjusting
            // positions and texture coordinates
            limitDrawVaoToBounds(dIt, m_divRefSize, m_uvDiff, m_scIndDraw, m_viewPort);  // scissoring
            dIt->texCoord = it; // only for border calculation

            // move texture coordinates, according to geometry limit
            if (m_uvDiff.x != 0.f || m_uvDiff.y != 0.f) {
                limitTexCoordsToBounds(&dIt->texCoord[0], j, {1.f, 1.f}, m_uvDiff);
                limitTexCoordsToBounds(&dIt->aux0[0], j, m_uvSize, m_uvDiff);
                limitTexCoordsToBounds(&dIt->aux0[2], j, m_tuvSize, m_uvDiff);
            }

            //dIt->color = m_color; // color doesn't make any sense in the context of images
            dIt->color = m_borderColor;

            if (m_imgBase && m_imgBase->getType() == ImageBase::Type::frame) {
                dIt->aux1.x = static_cast<float>(m_secPos.x);
                dIt->aux1.y = static_cast<float>(m_secPos.y);
                dIt->aux1.z = static_cast<float>(m_secSize.x);
                dIt->aux1.w = static_cast<float>(m_secSize.y);

                dIt->aux2.x = static_cast<float>(m_secSep.x);
                dIt->aux2.y = static_cast<float>(m_secSep.y);
                dIt->aux2.z = m_texUnit;  // texture unit
            } else {
                dIt->aux1.x = getBorderWidthRel().x;
                dIt->aux1.y = getBorderWidthRel().y;
                dIt->aux1.z = getBorderRadiusRel().x;
                dIt->aux1.w = getBorderRadiusRel().y;

                dIt->aux2.x = getBorderAliasRel().x;
                dIt->aux2.y = getBorderAliasRel().y;
                dIt->aux2.z = m_texUnit;       // texture unit
                dIt->aux2.w = static_cast<float>(m_ImgFlags);
            }

            dIt->aux3.x = (m_imgBase && m_imgBase->getType() == ImageBase::Type::frame) ? 3.f : 2.f;          // type indicator (2=Image simple, 3=Image frame)
            dIt->aux3.y = static_cast<float>(m_extTexBitCount);  // bitcount
            dIt->aux3.z = m_lod;                    // mipmap lod
            dIt->aux3.w = m_absoluteAlpha;

            j++;
            dIt++;
        }
    }
}

void Image::initUnitBlock() {
    if (!m_texUniBlock.isInited()) {
        m_texUniBlock.addVarName("mvp", getMVPMatPtr(), GL_FLOAT_MAT4);
        m_texUniBlock.addVarName("pos", &getContentOffset()[0], GL_FLOAT_VEC2);
        m_texUniBlock.addVarName("zPos", &m_offsZPos, GL_FLOAT);
        m_texUniBlock.addVarName("size", &getContentSize()[0], GL_FLOAT_VEC2);
        m_texUniBlock.addVarName("scale", &m_ImgScale, GL_FLOAT);
        m_texUniBlock.addVarName("align", &m_ImgAlign[0], GL_INT_VEC2);
        m_texUniBlock.addVarName("color", &m_color[0], GL_FLOAT_VEC4);
        m_texUniBlock.addVarName("flags", &m_ImgFlags, GL_INT);
        m_texUniBlock.addVarName("section_pos", &m_secPos[0], GL_INT_VEC2);
        m_texUniBlock.addVarName("section_size", &m_secSize[0], GL_INT_VEC2);
        m_texUniBlock.addVarName("section_sep", &m_secSep[0], GL_INT_VEC2);
        m_texUniBlock.addVarName("bit_count", &m_extTexBitCount, GL_INT);
        m_texUniBlock.addVarName("objId", &m_objIdMin, GL_UNSIGNED_INT);
        m_texUniBlock.addVarName("alpha", &m_absoluteAlpha, GL_FLOAT);
        m_texUniBlock.addVarName("borderRadius", &getBorderRadiusRel()[0], GL_FLOAT_VEC2);
        //m_texUniBlock.addVarName("borderWidth", &getBorderWidthRel()[0], GL_FLOAT_VEC2);
        //m_texUniBlock.addVarName("borderColor", &m_borderColor[0], GL_FLOAT_VEC4);
    }

    m_texUniBlock.update();
    m_offsZPos = m_zPos - 0.00001f;
}

void Image::pushVaoUpdtOffsets() {
    Div::pushVaoUpdtOffsets();
    if (m_imgDB.drawSet) {
        m_imgDB.drawSet->updtNodes.emplace_back(m_imgDB.getUpdtPair());
    }
}

bool Image::draw(uint32_t* objId) {
    Div::draw(objId);

    m_objIdMax = ++(*objId);

    // auto sr = getSharedRes();

    if (m_sharedRes) {
        if (m_imgBase && m_imgBase->getType() == ImageBase::Type::frame) {
            m_texShdr = m_sharedRes->shCol->getUIGridTexFrame();
        } else {
            m_texShdr = m_sharedRes->shCol->getUIGridTexSimple();
        }
    }

    if (!m_texShdr || (!m_imgBase && !tex && !m_useTexId)) {
        return true;
    }

#ifdef ARA_USE_GLES31
    glBlendFunc(m_srcBlendFunc, m_dstBlendFunc);
#else
    if (!m_sepBlendFunc && (m_srcBlendFunc != GL_SRC_ALPHA || m_dstBlendFunc != GL_ONE_MINUS_SRC_ALPHA)) {
        glBlendFunci(0, m_srcBlendFunc, m_dstBlendFunc);
    } else if (m_sepBlendFunc) {
        glBlendFuncSeparatei(0, m_srcBlendFunc, m_dstBlendFunc, m_srcBlendAlphaFunc, m_dstBlendAlphaFunc);
    }
#endif

    m_texShdr->begin();

    if (!m_texUniBlock.isInited()) {
        m_texUniBlock.init(m_texShdr->getProgram(), "nodeData");
        m_texUniBlock.update();
    }

    m_texUniBlock.bind();

    m_texShdr->setUniform1i("stex", 0);
    m_texShdr->setUniform1i("depth", 1);
    m_texShdr->setUniform1i("objIdTex", 2);

    if (m_useTexId) {
        // in some rare cases (FBO with depth buffers) it is needed to render depth values from an external depth buffer
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_texId);

        if (m_depthTexId) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, m_depthTexId);
        }

        if (m_extObjTexId) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, m_extObjTexId);

            *objId += m_extObjTexObjIdOffs;
            m_objIdMax = *objId;
        }
    } else if (m_imgBase) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_imgBase->getTexID());
    } else {
        if (tex && !m_useTexId) {
            tex->bind(0);
        }
    }

    if (m_hasDepth || m_depthTexId) {
        glEnable(GL_DEPTH_TEST);
    }

    glBindVertexArray(*m_sharedRes->nullVao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    // glBindVertexArray(0);

    if (m_hasDepth || m_depthTexId) {
        glDisable(GL_DEPTH_TEST);
    }

    return true;  // count up objId
}

bool Image::drawIndirect(uint32_t* objId) {
    Div::drawIndirect(objId);

    m_objIdMax = ++(*objId);

    if (m_sharedRes && m_sharedRes->drawMan) {
        auto dm = m_sharedRes->drawMan;
        pushTexture(&dm->getWriteSet());
        updateDrawData();  // can this be optimized?
        m_imgDB.drawSet = &m_sharedRes->drawMan->push(m_imgDB, this);
    }

    return true;
}

void Image::pushTexture(DrawSet* ds) {
    auto dm = m_sharedRes->drawMan;

    // push texture
    if (m_useTexId) {
        // in some rare cases (FBO with depth buffers) it is needed to render depth values from an external depth buffer
        m_texUnit = dm->pushTexture(*ds, m_texId);
    } else if (m_imgBase) {
        m_texUnit = dm->pushTexture(*ds, m_imgBase->getTexID());
    } else if (tex/* && !m_useTexId*/) {
        m_texUnit = dm->pushTexture(*ds, tex->getId());
    }
}

void Image::loadImg() {
    m_useTexId    = false;
    auto filename = m_imageFile;

    auto& texCol = getSharedRes()->glbase->textureCollector();
    tex = texCol.addFromMem(filename, dataPath(), m_mipMapLevel);
    if (tex){
        m_loaded    = true;
        m_texAspect = tex->getWidthF() / tex->getHeightF();
        m_texSize   = { tex->getWidth(), tex->getHeight() };

        texCol.addRemoveCb(filename, [this]{
            tex = nullptr;
        });

        if (m_sizeToAspect && (getAspect() != m_texAspect)) {
            setFixAspect(m_texAspect);
        }

        if (m_sizeChangeCb) {
            m_sizeChangeCb(static_cast<int>(tex->getWidth()), static_cast<int>(tex->getHeight()));
        }

        if (m_drawImmediate) {
            Image::updateDrawData();
        }
    }
}

void Image::setLod(float val) {
    m_lod = val;
    m_drawParamChanged = true;
}

bool Image::setTexId(GLuint inTexId, int width, int height, int bitCount) {
    if (inTexId == m_texId && m_extTexWidth == width && m_extTexHeight == height && bitCount == m_extTexBitCount){
        return false;
    }

    m_useTexId       = inTexId != 0;
    m_texId          = inTexId;
    m_extTexWidth    = width;
    m_extTexHeight   = height;
    m_extTexBitCount = bitCount;
    m_texAspect      = (float)width / (float)height;
    m_loaded         = true;

    if (m_fixAspect != -1.f && (getAspect() != m_texAspect)) {
        setFixAspect(m_texAspect);
    }

    unsigned iflags = 0;
    iflags |= 8;
    m_ImgFlags = iflags;

    // change shader if necessary
    if (m_sharedRes && ((m_imgBase && m_imgBase->getType() == ImageBase::Type::frame) || !m_texShdr)) {
        m_texShdr = m_sharedRes->shCol->getUIGridTexSimple();
    }

    if (m_imgDB.drawSet) {
        pushTexture(m_imgDB.drawSet);
    }

    m_drawParamChanged = true;
    return true;
}

void Image::clearDs() {
    Div::clearDs();
    m_imgDB.drawSet = nullptr;
}

bool Image::isInBounds(glm::vec2& pos) {
    if (!m_visible) {
        return false;
    }

    // inBounds calculation must respect the parent content-transformation matrices and children bounds
    getWinPos();
    getWinRelSize();

    for (int i = 0; i < 2; i++) {
        m_objItLT[i] = m_winRelPos[i] + std::min(0.f, m_childBoundBox[i]);
        m_objItRB[i] = m_objItLT[i] + std::max(m_winRelSize[i], m_childBoundBox[i + 2] - m_childBoundBox[i]);
    }

    m_mpInBounds = glm::all(glm::greaterThanEqual(pos, m_objItLT)) && glm::all(glm::lessThanEqual(pos, m_objItRB));

    if (!m_mpInBounds) {
        return false;
    }

    if (m_objUsesTexAlpha && m_imgBase && m_imgBase->getTexture() && m_imgBase->getTexture()->getBits()) {
        // get mouse pos node relative
        m_hidMp = glm::min(glm::max((pos - m_winRelPos) / m_winRelSize, m_zeroVec), m_oneVec);

        // offset and scale by the uv coordinates for this section
        m_hidMp = static_cast<vec2>(m_secPos) + m_hidMp * (vec2)m_secSize;

        // check if alpha value at the mouse position is > 0 (convert from lt
        // origin to lb origin, freeimage is using lb origin)
        return static_cast<int>(*(m_imgBase->getTexture()->getBits() +
                                  ((m_imgBase->getTexture()->getHeight() - static_cast<int>(m_hidMp.y)) * m_imgBase->getTexture()->getWidth() +
                                   static_cast<int>(m_hidMp.x)) * m_imgBase->getTexture()->getNrChans() +
                                  3)) > 0;
    } else {
        return true;
    }
}

void Image::reload() {
    if (m_imageFile.empty()) {
        return;
    }
    loadImg();
}

PingPongFbo *Image::getUplFbo() {
    if (m_uplFbo) {
        return m_uplFbo.get();
    } else {
        return nullptr;
    }
}

void Image::initUplFbo(int width, int height, GLenum type, GLenum target, bool depthBuf, int nrAttachments,
                int mipMapLevels, int nrSamples, GLenum wrapMode, bool layered) {
    m_uplFbo = std::make_unique<PingPongFbo>(m_glbase, width, height, type, target, depthBuf, nrAttachments,
                                             mipMapLevels, nrSamples, wrapMode, layered);
}

void Image::rebuildUplFbo(int width, int height, GLenum type, GLenum target, bool depthBuf, int nrAttachments,
                   int mipMapLevels, int nrSamples, GLenum wrapMode, bool layered) {
    if (m_uplFbo) {
        m_uplFbo.reset();
    }
    m_uplFbo = std::make_unique<PingPongFbo>(m_glbase, width, height, type, target, depthBuf, nrAttachments,
                                             mipMapLevels, nrSamples, wrapMode, layered);
}

void Image::initUplPbo(int w, int h, GLenum format) {
    m_uplPbo.setSize(w, h);
    m_uplPbo.setFormat(format);
    m_uplPbo.init();
}


}  // namespace ara