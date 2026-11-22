#pragma once

#include "UIElements/Image.h"
#include <Utils/FBO.h>
#include <Utils/UniformBlock.h>

namespace ara {

class PaintImage : public Image {
public:
    enum class mode : int32_t { add=0, subtract, colorPick, mask };
    struct Brush {
        float size = 10.0f;
        float hardness = 0.5f;
        glm::vec4 color = glm::vec4(1.0f);
        float opacity = 1.0f;
    };

    PaintImage();
    ~PaintImage() override = default;

    void init() override;
    virtual void initPaintShader();
    void initColorMaskShader();

    void mouseDown(hidData& data) override;
    void mouseDrag(hidData& data) override;
    void mouseWheel(hidData& data) override;
    void mouseUp(hidData& data) override;
    void mouseIn(hidData& data) override;
    void mouseOut(hidData& data) override;

    void undo() override;
    void redo() override;
    void saveState() override;
    virtual void clear();
    void clearBackupPath() const;

    Brush& getBrush() { return m_brush; }

    void lock(const bool val) { m_locked = val; }
    void loadImg(bool forceReload) override;

    void setBrush(const Brush& brush) { m_brush = brush; }
    void setMode(const mode& mode) { m_paintMode = mode; }
    void setTolerance(const float& val) { m_selTolerance = val; }
    void setBackupPath(const std::filesystem::path& p);
    void setBrushSize(float brushSize);
    void setBrushHardness(float hardness);
    void setMasking(bool val);
    void setCursor();

    void                    colorSelect(const glm::vec2& mousePos, glm::vec4& col, float tolerance);
    glm::vec4               pickColor(const glm::vec2& mousePos);
    [[nodiscard]] glm::vec4 getPickColor() const { return m_pickColor; }
    void                    saveToFile(const std::filesystem::path &filename) const;
    virtual void            setSharedTexture(const std::shared_ptr<Texture>& tex);

    [[nodiscard]] const std::vector<std::filesystem::path>& getBackupCopies() const { return m_backupCopies; }
    void restoreFile(const std::filesystem::path &filename);

protected:
    std::optional<glm::ivec2> getMousePosInTexPixSpace(const Texture* tex, const glm::vec2& mousePos);
    virtual void initFbo();
    void initMaskTex();
    virtual void paint(const glm::vec2& mousePos);
    void setCircleCursor();
    void resetCursor();

    Brush                                           m_brush{};
    std::unique_ptr<FBO>                            m_fbo;
    std::shared_ptr<Texture>                        m_ownedTex;
    Shaders*                                        m_paintShader = nullptr;
    Shaders*                                        m_colMaskSeedShdr = nullptr;
    Shaders*                                        m_colMaskShdr = nullptr;
    UniformBlock                                    m_brushBlock{};
    float                                           m_scaledBrush{};
    float                                           m_selTolerance=0.15f;
    bool                                            m_locked{};
    int32_t                                         m_maskSeedReadIdx{};
    mode                                            m_paintMode = mode::add;
    std::vector<std::filesystem::path>              m_backupCopies;
    std::vector<std::filesystem::path>::iterator    m_backupCopiesIt;
    std::filesystem::path                           m_backupPath;
    glm::vec4                                       m_pickColor{0.f};
    std::array<std::shared_ptr<Texture>, 2>         m_maskTex;
    bool                                            m_drawMasked = false;
#ifdef ARA_USE_GLFW
    GLFWcursor*                                     m_cursor{};
#endif
};

} // namespace ara
