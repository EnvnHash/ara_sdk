//
// Created by user on 12.05.2021.
//

#include "MaskStackRenderer.h"

#include "GeoPrimitives/Quad.h"

using namespace std;
using namespace glm;

namespace ara {

MaskStackRenderer::MaskStackRenderer(MaskStack *stack, uint32_t width, uint32_t height, GLBase *glbase)
    : m_stack(stack), m_width(width), m_height(height), m_shCol(&glbase->shaderCollector()), m_glbase(glbase) {
    init(glbase);
}

void MaskStackRenderer::init(GLBase *glbase) {
    m_glbase = glbase;
    if (!m_inited && m_shCol && m_width && m_height && m_stack && m_glbase) {
        // create MaskStackLayers for each layer in the stack
        rebuildLayerRenderer();

        m_fbo           = new FBO(m_glbase, (int)m_width, (int)m_height, GL_RGBA8, GL_TEXTURE_2D, false, 2, 1, 1,
                                  GL_CLAMP_TO_EDGE, false);
        m_quad          = new Quad(-1.f, -1.f, 2.f, 2.f, vec3(0.f, 0.f, 1.f), 1.f, 1.f, 1.f, 1.f);
        m_quadFlipH     = new Quad(-1.f, -1.f, 2.f, 2.f, vec3(0.f, 0.f, 1.f), 1.f, 1.f, 1.f, 1.f, nullptr, 1, true);
        m_stdTexShdr    = m_shCol->getStdTexMulti();
        m_alphaToBwShdr = initAlphaToBwShdr();
        m_inited        = true;
    }
}

// must be called with a valid gl context bound
void MaskStackRenderer::rebuildLayerRenderer() {
    m_layerRenderer.clear();

    for (auto &it : *m_stack->getLayers()) {
        m_layerRenderer.push_back(make_unique<MaskLayerRenderer>(it.get(), m_width, m_height, m_shCol));
        m_layerRenderer.back()->init();
    }
}

Shaders *MaskStackRenderer::initAlphaToBwShdr() {
    std::string vert = STRINGIFY(layout(location = 0) in vec4 position; layout(location = 2) in vec2 texCoord;
                                 out vec2 tex_coord; void main() {
                                     tex_coord   = texCoord;
                                     gl_Position = position;
                                 });
    vert             = "// MaskStackAlphaBwShdr, vert\n" + m_shCol->getShaderHeader() + vert;

    std::string frag = STRINGIFY(in vec2 tex_coord; layout(location = 0) out vec4 fragColor; uniform sampler2D tex;
                                 uniform int invert; uniform vec4 alphaToColor; \n void main() {
                                     vec4  texCol  = texture(tex, tex_coord);
                                     float isWhite = dot(texCol.rgb, vec3(1.0));
                                     float alpha   = (1.0 - isWhite) * texCol.a;
                                     fragColor     = bool(invert)
                                                         ? mix(alphaToColor, vec4(1.0 - texCol.rgb, texCol.a), alpha)
                                                         : vec4(0.0, 0.0, 0.0, alpha);
                                     \n
                                 });

    frag = "// MaskStackAlphaBwShdr, frag\n" + m_shCol->getShaderHeader() + frag;
    return m_shCol->add("MaskStackAlphaBwShdr", vert, frag);
}

bool MaskStackRenderer::update() {
    if (!m_inited && !m_stack) return false;

    // sync number of layer renderers
    if (m_layerRenderer.size() != m_stack->getNrLayers()) rebuildLayerRenderer();

    if (m_fbo && (m_fbo->getWidth() != m_width || m_fbo->getHeight() != m_height)) resize(m_width, m_height, true);

    for (auto &it : m_layerRenderer)
        if (it->isVisible()) it->update();

    // render down all layers additive
    if (m_fbo && m_fbo->isInited() && m_quad) {
        m_fbo->bind();
        m_fbo->clearToColor(1.f, 1.f, 1.f, 0.f, 0);
        m_fbo->clearToColor(0.f, 0.f, 0.f, 0.f, 1);

        // only do this if there are valid layers, otherwise just clear the
        // s_fbo
#if !defined(__EMSCRIPTEN__) && !defined(ARA_USE_GLES31)
        glEnablei(GL_BLEND, 0);
        glDisablei(GL_BLEND, 1);  // no blending for objmap
        glBlendFuncSeparatei(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
#endif

        uint32_t ind = 1;
        for (auto &it : m_layerRenderer)
            if (it->isVisible()) it->render(ind++);

        m_fbo->unbind();
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (m_downloadCb) {
        m_downloadCb();
        m_downloadCb = nullptr;
    }

    if (m_alwaysDownload && m_downloadWidth && m_downloadHeight)
        download(m_downloadWidth, m_downloadHeight, GL_RED, true, true);

    if (!m_updtCb.empty())
        for (auto &it : m_updtCb) it();

    return true;
}

void MaskStackRenderer::renderModified(uint32_t width, uint32_t height, bool alphaToBW, bool flipH) {
    Shaders *shdr = m_stdTexShdr;

    if (!m_stack) return;

    if (!m_modFbo)
        m_modFbo = new FBO(m_glbase, (int)width, (int)height, m_fbo->getType(), m_fbo->getTarget(), false, 1, 1,
                           m_fbo->getNrSamples(), GL_CLAMP_TO_EDGE, false);

    if (m_modFbo->getWidth() != width || m_modFbo->getHeight() != height) m_modFbo->resize(width, height);

    m_modFbo->bind();

    if (alphaToBW) {
        shdr = m_alphaToBwShdr;
        glClearColor(1.f, 1.f, 1.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shdr->begin();
    shdr->setUniform1i("tex", 0);
    shdr->setUniform1i("invert", !alphaToBW);
    shdr->setUniform4fv("alphaToColor", m_stack->getFillColorPtr());
    glBindTexture(m_fbo->getTarget(), m_fbo->getColorImg());

    if (flipH)
        m_quadFlipH->draw();
    else
        m_quad->draw();

    m_modFbo->unbind();
}

void MaskStackRenderer::download(void *ptr, uint32_t width, uint32_t height, GLenum intFormat, bool alphaToBW,
                                 bool flipH, GLenum extType) {
    if (!width || !height) return;
    FBO *downloadFbo = m_fbo;
    glActiveTexture(GL_TEXTURE0);

    if (alphaToBW || flipH || (width != m_fbo->getWidth()) || (height != m_fbo->getHeight())) {
        renderModified(width, height, alphaToBW, flipH);
        downloadFbo = m_modFbo;
    }

    // bind the texture
    glBindTexture(downloadFbo->getTarget(), downloadFbo->getColorImg());

    // read back
    glPixelStorei(GL_PACK_ALIGNMENT, 4);  // should be 4

    // synchronous, blocking command, no swap() needed
#ifdef ARA_USE_GLES31
    glesGetTexImage(downloadFbo->getColorImg(), downloadFbo->getTarget(), getExtType(intFormat),
                    getPixelType(intFormat), width, height, (GLubyte *)ptr);
#else
    if (ptr)
        glGetTexImage(downloadFbo->getTarget(), 0, extType ? extType : getExtType(intFormat), getPixelType(intFormat),
                      ptr);
#endif

    // LOG << " saving mask to file " << (std::filesystem::current_path() /
    // "mask.png").string();
    // downloadFbo->saveToFile(std::filesystem::current_path() / "mask.png", 0);

    m_downloadDone = true;
}

void MaskStackRenderer::download(const std::string &filename, uint32_t width, uint32_t height, GLenum format,
                                 bool alphaToBW, bool flipH) {
    if (!width || !height) return;
    FBO *downloadFbo = m_fbo;
    glActiveTexture(GL_TEXTURE0);

    if (alphaToBW || flipH || (width != m_fbo->getWidth()) || (height != m_fbo->getHeight())) {
        renderModified(width, height, alphaToBW, flipH);
        downloadFbo = m_modFbo;
    }

    downloadFbo->saveToFile(std::filesystem::path(filename), 0, format);
}

void *MaskStackRenderer::download(uint32_t width, uint32_t height, GLenum extFormat, bool alphaToBW, bool flipH) {
    if (!width || !height) return nullptr;
    FBO *downloadFbo = m_fbo;
    glActiveTexture(GL_TEXTURE0);

    // if the requested download size is different from the requested one
    // resize a download FBO, update again and download

    if (alphaToBW || flipH || (width != m_fbo->getWidth()) || (height != m_fbo->getHeight())) {
        renderModified(width, height, alphaToBW, flipH);
        downloadFbo = m_modFbo;
    }

    // bind the texture
    glBindTexture(downloadFbo->getTarget(), downloadFbo->getColorImg());

    // read back
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // resize dataArray if necessary
    int nrChannels = (extFormat == GL_RGBA || extFormat == GL_BGRA)
                         ? 4
                         : ((extFormat == GL_RGB || extFormat == GL_BGR) ? 3 : (extFormat == GL_RG ? 2 : 1));

    size_t byteArSize = width * height * nrChannels;
    if (m_dataByte.size() != byteArSize) m_dataByte.resize(byteArSize);

    // synchronous, blocking command, no swap() needed
#ifdef ARA_USE_GLES31
    glesGetTexImage(downloadFbo->getColorImg(), downloadFbo->getTarget(), extFormat, GL_UNSIGNED_BYTE, width, height,
                    &m_dataByte[0]);
#else
    glGetTexImage(downloadFbo->getTarget(), 0, extFormat, GL_UNSIGNED_BYTE, &m_dataByte[0]);
#endif
    return (void *)&m_dataByte[0];
}

FBO *MaskStackRenderer::render(uint32_t width, uint32_t height, bool alphaToBW, bool flipH) {
    if (!width || !height) return nullptr;
    FBO *downloadFbo = m_fbo;
    glActiveTexture(GL_TEXTURE0);

    if (alphaToBW || flipH || (width != m_fbo->getWidth()) || (height != m_fbo->getHeight())) {
        renderModified(width, height, alphaToBW, flipH);
        downloadFbo = m_modFbo;
    }

    return downloadFbo;
}

void MaskStackRenderer::setDownloadCb(void *ptr, uint32_t width, uint32_t height, GLenum format, bool alphaToBW,
                                      bool flipH, const function<void()> &cb) {
    if (!width || !height) return;

    // request resizing, this will cause the size check in update() to call
    // resize()
    m_width      = width;
    m_height     = height;
    m_downloadCb = [this, ptr, width, height, format, alphaToBW, flipH, cb]() {
        download(ptr, width, height, format, alphaToBW, flipH);
        if (cb) cb();
    };
}

void MaskStackRenderer::resize(uint32_t width, uint32_t height, bool force) {
    if (m_shCol && (m_width != width || m_height != height || force)) {
        m_width  = width;
        m_height = height;
        m_aspect = (float)m_width / (float)m_height;

        if (m_fbo) m_fbo->resize(width, height);
        if (m_modFbo) m_modFbo->resize(width, height);

        for (auto &it : m_layerRenderer)
            if (it->getWidth() != m_width || it->getHeight() != m_height) it->resize(width, height);
    }
}

void MaskStackRenderer::freeGLResources() {
    if (m_fbo) {
        delete m_fbo;
        m_fbo = nullptr;
    }
    if (m_modFbo) {
        delete m_modFbo;
        m_modFbo = nullptr;
    }
    if (m_quad) {
        delete m_quad;
        m_quad = nullptr;
    }
    if (m_quadFlipH) {
        delete m_quadFlipH;
        m_quadFlipH = nullptr;
    }
    m_layerRenderer.clear();
}

}  // namespace ara