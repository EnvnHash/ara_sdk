#include <Res/ResGlFont.h>
#include <Res/Font.h>
#include <Utils/Texture.h>

#include <RwBinFile.h>

using namespace std;
using namespace glm;

/**
 * OpenGL Font rendering
 * Note: all pixel arguments are meant in "virtual pixels", that is dpi
 * independent pixel values. all class member pixel values are meant in hardware
 * pixels, that is 1:1 in display (unscaled) resolution
 */

namespace ara {

Font *FontList::get(const std::string& font_path, int size, float pixRatio) {
    return add(font_path, size, pixRatio);
}

Font *FontList::find(const std::string &font_path, int size, float pixRatio) const {
    for (auto &f : m_FontList) {
        if (f->isFontType(font_path, size, pixRatio)) {
            return f.get();
        }
    }

    return nullptr;
}

Font *FontList::add(const std::string& font_path, int size, float pixRatio) {
    if (Font *font; (font = find(font_path, size, pixRatio)) != nullptr) {
        return font;
    }

    m_FontList.push_back(make_unique<Font>(font_path, size, pixRatio));
    update3DLayers();
    return m_FontList.back().get();
}

Font *FontList::addFromFilePath(const std::string& font_path, int size, float pixRatio) {
    if (Font *font; (font = find(font_path, size, pixRatio)) != nullptr) {
        return font;
    }

    std::vector<uint8_t> vp;
    if (!ReadBinFile(vp, font_path)) {
        return nullptr;
    }

    m_FontList.push_back(make_unique<Font>(vp, font_path, size, pixRatio));
    update3DLayers();
    return m_FontList.back().get();
}

Font *FontList::add(std::vector<uint8_t> &vp, const std::string& font_path, int size, float pixRatio) {
    if (Font *font; (font = find(font_path, size, pixRatio)) != nullptr) {
        return font;
    }

    m_FontList.emplace_back(make_unique<Font>(vp, font_path, size, pixRatio));
    update3DLayers();

    return m_FontList.back().get();
}

/** copy all glyph texture maps of the same size into one TEXTURE_3D for
 * DrawManager indirect drawing */
void FontList::update3DLayers() {
    m_layerCount.clear();

    // check how many glyph texture maps are there for each size
    for (auto &fnt : m_FontList) {
        m_layerCount[fnt->getGlyphTexSize()].emplace_back(fnt.get());
    }

    // check which of the layers changed and update those
    bool entrExists = false;
    int  layerCnt   = 0;
    int  sz         = 0;
    for (auto &lc : m_layerCount) {
        sz         = lc.first;
        entrExists = m_fontTexLayers.find(sz) != m_fontTexLayers.end();

        // 3d texture doesn't exist, or quantity changed
        if (!entrExists || (entrExists && m_fontTexLayers[sz]->getDepth() != static_cast<uint>(lc.second.size()))) {
            // unfortunately textures can't be resized, conserving the existing content, so we have to delete them
            if (entrExists) {
                m_fontTexLayers[sz]->releaseTexture();
            }

            // allocate a new texture
            m_fontTexLayers[sz] = make_unique<Texture>(m_glbase);
            m_fontTexLayers[sz]->allocate3D(sz, sz, static_cast<uint32_t>(m_layerCount[sz].size()), GL_R8, GL_RED, GL_TEXTURE_3D,
                                            GL_UNSIGNED_BYTE);

            layerCnt = 0;

#ifdef ARA_USE_GLES31
            std::vector<uint8_t> pixels(sz * sz);

            // on GLES the blitting trick doesn't work ... do this by copying to
            // the cpu and back copy the glyph textures into them, do this by
            for (auto &fnt : lc.second) {
                glesGetTexImage(fnt->getTexId(), GL_TEXTURE_2D, GL_RED, GL_UNSIGNED_BYTE, sz, sz, &pixels[0]);
                glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, layerCnt, sz, sz, 1, GL_RED, GL_UNSIGNED_BYTE, &pixels[0]);
                fnt->setTexLayer(m_fontTexLayers[sz]->getId(), layerCnt, (uint32_t)m_layerCount[sz].size());
                layerCnt++;
            }
#else
#ifdef __APPLE__
            if (!m_glbase->rendererIsIntel()) {
#endif
                m_fbo.setGlbase(m_glbase);
                m_fbo.getActStates();  // save last state
                m_fbo.genFbo();

                std::array<GLenum, 2> bufModes = {GL_NONE, GL_COLOR_ATTACHMENT1};
                glDrawBuffers(2, &bufModes[0]);

                // copy the glyph textures into them, do this by fbo blitting if
                // possible
                for (auto &fnt : lc.second) {
                    // attach the source texture to color attachment0
                    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fnt->getTexId(),
                                           0);

                    // attach the destination texture layer to color attachment1
                    glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, m_fontTexLayers[sz]->getId(),
                                              0, layerCnt);

                    FBO::checkFbo();

                    glBlitFramebuffer(0, 0, sz, sz, 0, 0, sz, sz, GL_COLOR_BUFFER_BIT, GL_NEAREST);

                    fnt->setTexLayer(m_fontTexLayers[sz]->getId(), layerCnt, static_cast<uint32_t>(m_layerCount[sz].size()));
                    ++layerCnt;
                }

                m_fbo.deleteFbo();
                m_fbo.restoreStates();  // restore states
#ifdef __APPLE__
            } else {
                // special treatment for macOS with intel gpu which don't supported mixed TEXTURE_2D and TEXTURE_3D
                // attachments copy to cpu and back
                std::vector<uint8_t> pixels(sz * sz);
                for (auto &fnt : lc.second) {
                    glBindTexture(GL_TEXTURE_2D, fnt->getTexId());
                    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, &pixels[0]);
                    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, layerCnt, sz, sz, 1, GL_RED, GL_UNSIGNED_BYTE, &pixels[0]);
                    fnt->setTexLayer(m_fontTexLayers[sz]->getId(), layerCnt, (uint32_t)m_layerCount[sz].size());
                    layerCnt++;
                }
            }
#endif
#endif
        }
    }
}

}  // namespace ara