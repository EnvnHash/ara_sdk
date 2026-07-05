#pragma once

#include "UIElements/Image.h"
#include <Utils/FBO.h>
#include <Utils/UniformBlock.h>

namespace ara {

class PaintImage : public Image {
public:
    enum class mode : int32_t { add=0, subtract };
    struct Brush {
        float size = 10.0f;
        float hardness = 0.5f;
        glm::vec4 color = glm::vec4(1.0f);
        float opacity = 1.0f;
    };

    PaintImage();
    ~PaintImage() override = default;

    void init() override;
    
    void mouseDown(hidData& data) override;
    void mouseDrag(hidData& data) override;

    Brush& getBrush() { return m_brush; }

    void lock(const bool val) { m_locked = val; }
    void setBrush(const Brush& brush) { m_brush = brush; }
    void setMode(const mode& mode) { m_paintMode = mode; }
    void setBrushSize(float brushSize);
    void setBrushHardness(float hardness);
    void saveToFile(const std::filesystem::path &filename) const;

protected:
    void paint(const glm::vec2& mousePos);

    Brush                       m_brush{};
    std::unique_ptr<FBO>        m_fbo;
    std::shared_ptr<Texture>    m_ownedTex;
    Shaders*                    m_paintShader = nullptr;
    UniformBlock                m_brushBlock{};
    float                       m_scaledBrush{};
    bool                        m_locked{};
    mode                        m_paintMode = mode::add;
};

} // namespace ara
