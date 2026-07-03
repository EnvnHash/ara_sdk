#pragma once

#include "UIElements/Image.h"
#include <Utils/FBO.h>
#include <Utils/UniformBlock.h>

namespace ara {

class PaintImage : public Image {
public:
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

    void setBrush(const Brush& brush) { m_brush = brush; }
    Brush& getBrush() { return m_brush; }
    void saveToFile(const std::filesystem::path &filename) const;

protected:
    void paint(const glm::vec2& mousePos);

    Brush                       m_brush{};
    std::unique_ptr<FBO>        m_fbo;
    std::shared_ptr<Texture>    m_ownedTex;
    Shaders*                    m_paintShader = nullptr;
    UniformBlock                m_brushBlock{};
    float                       m_scaledBrush{};
};

} // namespace ara
