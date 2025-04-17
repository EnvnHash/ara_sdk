//
// Created by user on 12.05.2021.
//

#pragma once

#include <Utils/MaskLayerRenderer.h>
#include <Utils/MaskStack.h>

namespace ara {

class MaskStackRenderer {
public:
    MaskStackRenderer() = default;
    MaskStackRenderer(MaskStack *stack, uint32_t width, uint32_t height, GLBase *glbase);
    virtual ~MaskStackRenderer() = default;

    void init(GLBase *glbase);
    void rebuildLayerRenderer();

    void resize(uint32_t width, uint32_t height) {
        m_width  = width;
        m_height = height;
        // resize();
    }

    void     resize(uint32_t width, uint32_t height, bool force = false);
    Shaders *initAlphaToBwShdr();
    bool     update();
    void     renderModified(uint32_t width, uint32_t height, bool alphaToBW, bool flipH);
    void    *download(uint32_t width, uint32_t height, GLenum format, bool alphaToBW = false, bool flipH = false);
    void download(void *ptr, uint32_t width, uint32_t height, GLenum format, bool alphaToBW = false, bool flipH = false,
                  GLenum extType = 0);
    void download(const std::string &filename, uint32_t width, uint32_t height, GLenum format, bool alphaToBW = false,
                  bool flipH = false);
    FBO *render(uint32_t width, uint32_t height, bool alphaToBW, bool flipH);
    void setDownloadCb(void *ptr, uint32_t width, uint32_t height, GLenum format, bool alphaToBW, bool flipH,
                       const std::function<void()> &cb);
    void freeGLResources();
    void setStack(MaskStack *stack) { m_stack = stack; }
    void setShaderCollector(ShaderCollector *shCol) { m_shCol = shCol; }

    void setWidth(uint32_t width) {
        m_width  = width;
        m_aspect = static_cast<float>(m_width) / static_cast<float>(m_height);
    }

    void setHeight(uint32_t height) {
        m_height = height;
        m_aspect = static_cast<float>(m_height) / static_cast<float>(m_height);
    }

    void setSize(uint32_t width, uint32_t height) {
        m_width  = width;
        m_height = height;
        m_aspect = static_cast<float>(m_width) / static_cast<float>(m_height);
    }

    void               addUpdtCb(std::function<void()> func) { m_updtCb.push_back(std::move(func)); }
    [[nodiscard]] bool isInited() const { return m_inited; }
    [[nodiscard]] FBO *getFbo() const { return m_fbo; }
    [[nodiscard]] FBO *getModFbo() const { return m_modFbo; }

    uint8_t *getCpuBuffer() {
        if (m_dataByte.empty())
            return nullptr;
        else
            return &m_dataByte[0];
    }

    void setAlwaysDownload(bool val) { m_alwaysDownload = val; }

    void setDownloadSize(uint32_t w, uint32_t h) {
        m_downloadWidth  = w;
        m_downloadHeight = h;
    }

    [[nodiscard]] uint32_t   getDownloadWidth() const { return m_downloadWidth; }
    [[nodiscard]] uint32_t   getDownloadHeight() const { return m_downloadHeight; }
    [[nodiscard]] uint32_t   getWidth() const { return m_width; }
    [[nodiscard]] uint32_t   getHeight() const { return m_height; }
    [[nodiscard]] float      getAspect() const { return m_aspect; }
    [[nodiscard]] uint32_t   getNrLayers() const { return static_cast<uint32_t>(m_layerRenderer.size()); }
    [[nodiscard]] MaskStack *getStack() const { return m_stack; }

    [[nodiscard]] MaskLayerRenderer *getLayer(size_t nr) const {
        if (m_layerRenderer.size() > nr)
            return m_layerRenderer[nr].get();
        else
            return nullptr;
    }

private:
    ShaderCollector *m_shCol  = nullptr;
    MaskStack       *m_stack  = nullptr;
    GLBase          *m_glbase = nullptr;

    Quad *m_quad      = nullptr;
    Quad *m_quadFlipH = nullptr;

    std::vector<std::unique_ptr<MaskLayerRenderer>> m_layerRenderer;

    bool m_inited         = false;
    bool m_alwaysDownload = false;
    bool m_downloadDone   = false;

    uint32_t m_downloadWidth  = 0;
    uint32_t m_downloadHeight = 0;
    uint32_t m_width          = 0;
    uint32_t m_height         = 0;

    float m_aspect = 1.f;

    FBO *m_fbo    = nullptr;
    FBO *m_modFbo = nullptr;

    Shaders *m_stdTexShdr    = nullptr;
    Shaders *m_alphaToBwShdr = nullptr;

    // std::unique_ptr<Texture>                           m_debugDownload;
    std::pair<void *, void *> m_initCtx;
    std::vector<uint8_t>      m_dataByte;

    std::function<void()>              m_downloadCb;
    std::vector<std::function<void()>> m_updtCb;
};

}  // namespace ara
