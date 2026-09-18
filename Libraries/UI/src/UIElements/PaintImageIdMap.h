#pragma once

#include "UIElements/PaintImage.h"

namespace ara {

class PaintImageIdMap : public PaintImage {
public:
    PaintImageIdMap();
    ~PaintImageIdMap() override = default;

    void init() override;

    bool draw(uint32_t& objId) override;
    bool drawIndirect(uint32_t& objId) override;

    void setDrawId(const int id) { m_drawID = id; }
    void setVisualizationID(const int id) { m_visualizationID = id; }
    [[nodiscard]] int getVisualizationID() const { return m_visualizationID; }

protected:
    bool drawFunc(const uint32_t& objId);
    void paint(const glm::vec2& mousePos) override;

    int m_drawID = 0;
    int m_visualizationID = 0;
    Shaders* m_visShader = nullptr;
};

} // namespace ara
