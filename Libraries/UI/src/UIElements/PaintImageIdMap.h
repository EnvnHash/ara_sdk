#pragma once

#include "UIElements/PaintImage.h"
#include <GLBase.h>

namespace ara {

class PaintImageIdMap : public PaintImage {
public:
    PaintImageIdMap();
    ~PaintImageIdMap() override = default;

    void init() override;
    void initPaintShader();
    void initVisualizationShader();
    void initFillShader();

    bool draw(uint32_t& objId) override;
    bool drawIndirect(uint32_t& objId) override;
    void clear();

    void fillByTexture(GLuint texId, glm::vec4 selectColor, uint32_t destBit) const;

    void setDrawId(const int id) { m_drawID = id; }
    void setPaintMode(const bool additive) { m_drawAdditive = additive; }
    void setVisualizationID(const int id) { m_visualizationID = id; }
    [[nodiscard]] int getVisualizationID() const { return m_visualizationID; }

protected:
    bool drawFunc(const uint32_t& objId);
    void paint(const glm::vec2& mousePos) override;

    bool m_drawAdditive = true;
    int m_drawID = 0;
    int m_visualizationID = 0;
    Shaders* m_visShader = nullptr;
    Shaders* m_fillByTextureShader = nullptr;
};

} // namespace ara
