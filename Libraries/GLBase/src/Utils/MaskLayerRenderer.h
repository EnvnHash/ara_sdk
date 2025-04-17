//
// Created by user on 12.05.2021.
//

#pragma once

#include "Utils/FBO.h"
#include "Utils/MaskLayer.h"

namespace ara {

class MaskLayerRenderer {
public:
    MaskLayerRenderer() = default;
    MaskLayerRenderer(MaskLayer *layer, uint32_t width, uint32_t height, ShaderCollector *shCol);

    void init();
    void initPolygonShdr();
    void initTesselationShader();
    void update();
    void render(uint32_t ind);
    void resize();

    void resize(uint32_t width, uint32_t height) {
        m_width  = width;
        m_height = height;
        resize();
    }

    void setLayer(MaskLayer *layer) { m_layer = layer; }
    void setShaderCollector(ShaderCollector *shCol) { m_shCol = shCol; }

    void setWidth(uint32_t width) {
        m_width = width;
        resize();
    }

    void setHeight(uint32_t height) {
        m_height = height;
        resize();
    }

    void setSize(uint32_t width, uint32_t height) {
        m_width  = width;
        m_height = height;
        resize();
    }

    void                   setObjId(uint32_t id) { m_id = id; }
    [[nodiscard]] uint32_t getWidth() const { return m_width; }
    [[nodiscard]] uint32_t getHeight() const { return m_height; }

    [[nodiscard]] Polygon *getPolygon() const {
        if (m_layer && m_layer->getPolygon())
            return m_layer->getPolygon();
        else
            return nullptr;
    }

    [[nodiscard]] Polygon *getPolygonInv() const {
        if (m_layer && m_layer->getPolygonInv())
            return m_layer->getPolygonInv();
        else
            return nullptr;
    }

    [[nodiscard]] uint32_t getObjId() const { return m_id; }

    [[nodiscard]] bool isVisible() const {
        if (m_layer)
            return m_layer->isVisible();
        else
            return false;
    }

    [[nodiscard]] bool isInverted() const {
        if (m_layer)
            return m_layer->isInverted();
        else
            return false;
    }

    [[nodiscard]] glm::mat4 *getTransformMat() const {
        if (m_layer)
            return &m_layer->getTransformMat();
        else
            return nullptr;
    }

    [[nodiscard]] MaskLayer *getMaskLayer() const { return m_layer; }

private:
    bool m_inited = false;

    uint32_t m_width  = 0;
    uint32_t m_height = 0;
    uint32_t m_id     = 0;

    MaskLayer *m_layer = nullptr;

    ShaderCollector *m_shCol      = nullptr;
    Shaders         *m_shdr       = nullptr;
    Shaders         *m_texShdr    = nullptr;
    Shaders         *m_tessShader = nullptr;
};

}  // namespace ara
