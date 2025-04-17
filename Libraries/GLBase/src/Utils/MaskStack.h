//
// Created by user on 10.07.2020.
//

#pragma once

#include "Utils/MaskLayer.h"

namespace ara {

class MaskStack {
public:
    enum class useCase : int { standard = 0, camera };
    MaskStack() {}
    explicit MaskStack(GLBase *glbase) : m_glbase(glbase), m_shCol(&glbase->shaderCollector()) {}

    /*
    MaskStack(const MaskStack& m) : m_inited(m.m_inited),
    m_hasBitmap(m.m_hasBitmap), m_hasPolygon(m.m_hasBitmap),
        m_updtCbs(m.m_updtCbs), m_shCol(m.m_shCol), m_fillColor(m.m_fillColor),
    m_fillColorInv(m.m_fillColorInv), m_loadFileName(m.m_loadFileName),
    m_stackSaveFileName(m.m_stackSaveFileName) { for (auto &it : m.m_layers){
            m_layers.emplace_back(new MaskLayer(*it));
        }
        LOG << "MaskStack copy ctor";
    }*/

    MaskLayer *addLayer();
    MaskLayer *addLayer(maskType mType);
    MaskLayer *addLayer(const std::filesystem::path &inPath, maskType mType);

    void clear();
    void removeLayer(size_t idx);
    void update();
    void checkLayers();
    void createBitmapLayer(uint8_t *data, int width, int height, int bpp, bool inverted, bool hFlip);
    void addUptCbs(void *name, std::function<void()> f) { m_updtCbs[name] = std::move(f); }

    void removeUptCbs(void *name) {
        auto it = m_updtCbs.find(name);
        if (it != m_updtCbs.end()) m_updtCbs.erase(it);
    }

    void callUptCbs() {
        for (auto &it : m_updtCbs) it.second();
    }

    size_t getNrLayers() { return m_layers.size(); }

    MaskLayer *getLayer(size_t nr) {
        if (m_layers.size() > nr)
            return m_layers[nr].get();
        else
            return nullptr;
    }

    std::vector<std::unique_ptr<MaskLayer>> *getLayers() { return &m_layers; }
    float                                   *getFillColorPtr() { return &m_fillColor[0]; }
    glm::vec4                               *getFillColorVecPtr() { return &m_fillColor; }
    float                                   *getFillColorInvPtr() { return &m_fillColorInv[0]; }
    glm::vec4                               *getFillColorInvVecPtr() { return &m_fillColorInv; }
    [[nodiscard]] bool                       hasBitmap() const { return m_hasBitmap; }
    [[nodiscard]] bool                       hasPolygon() const { return m_hasPolygon; }
    void                                     setShaderCollector(ShaderCollector *shCol) { m_shCol = shCol; }
    void                                     setSaveFileName(std::string &fn) { m_stackSaveFileName = fn; }
    void                                     setLoadFileName(std::string lfn) { m_loadFileName = std::move(lfn); }
    ShaderCollector                         *getShaderCollector() { return m_shCol; }

    void serializeToXml(pugi::xml_node &parent);
    void parseFromXml(pugi::xml_node &node);

    template <class B>
    void serialize(B &buf) const {
        buf << m_fillColor.r << m_fillColor.g << m_fillColor.b << m_fillColor.a;
        buf << m_fillColorInv.r << m_fillColorInv.g << m_fillColorInv.b << m_fillColorInv.a;
        buf << m_layers;
    }

    template <class B>
    void parse(B &buf) {
        buf >> m_fillColor.r >> m_fillColor.g >> m_fillColor.b >> m_fillColor.a;
        buf >> m_fillColorInv.r >> m_fillColorInv.g >> m_fillColorInv.b >> m_fillColorInv.a;
        buf >> m_layers;  // note: if this isn't parsed as last item, parsing of
                          // former variables fails

        if (m_shCol)
            for (auto &it : m_layers) {
                it->setShaderCollector(m_shCol);
                it->setInited(true);
            }

        checkLayers();
    }

private:
    bool                                              m_inited     = false;
    bool                                              m_hasBitmap  = false;
    bool                                              m_hasPolygon = false;
    std::vector<std::unique_ptr<MaskLayer>>           m_layers;
    std::unordered_map<void *, std::function<void()>> m_updtCbs;
    ShaderCollector                                  *m_shCol        = nullptr;
    GLBase                                           *m_glbase       = nullptr;
    glm::vec4                                         m_fillColor    = glm::vec4(0.f, 0.f, 0.f, 1.f);
    glm::vec4                                         m_fillColorInv = glm::vec4(0.f, 0.f, 0.f, 1.f);
    std::string                                       m_loadFileName;
    std::string                                       m_stackSaveFileName;
};
}  // namespace ara
